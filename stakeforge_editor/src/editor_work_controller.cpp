/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "editor_work_controller.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define EDITOR_WORK_BUCKET_CAPACITY 512

	editor_work_context_t::editor_work_context_t(editor_work_controller_t& controller, editor_work_handle_t handle, void* work) : _controller(&controller), _work(work), _handle(handle)
	{
	}

	void editor_work_context_t::set_progress(f32 progress, const char* text)
	{
		_controller->set_work_progress(_work, progress, text);
	}

	void editor_work_controller_t::init()
	{
		SFG_ASSERT(is_main_thread());

		_works.init(EDITOR_WORK_BUCKET_CAPACITY);
		_stop_requested.store(false, std::memory_order_relaxed);
		_last_submitted_work = {};
		_worker_thread		 = std::thread(&editor_work_controller_t::worker_loop, this);
	}

	void editor_work_controller_t::uninit()
	{
		SFG_ASSERT(is_main_thread());

		_stop_requested.store(true, std::memory_order_release);
		_worker_thread.join();

		tick();

		SFG_ASSERT(_works.begin() == _works.end());
		SFG_ASSERT(_last_submitted_work.is_null());

		_works.uninit();
		_stop_requested.store(false, std::memory_order_relaxed);
	}

	void editor_work_controller_t::tick()
	{
		SFG_ASSERT(is_main_thread());

		for (work_t& work : _works)
			drain_progress_text(work);

		completion_action_t action = {};

		while (_completion_actions.try_dequeue(action))
		{
			if (action.handle == _last_submitted_work)
				_last_submitted_work = {};

			if (action.fn != nullptr)
				action.fn(action.handle, action.state, action.user_data);

			_works.remove(action.handle);
		}
	}

	void editor_work_controller_t::wait_for_all()
	{
		SFG_ASSERT(is_main_thread());

		while (!_last_submitted_work.is_null())
		{
			const editor_work_handle_t work = _last_submitted_work;

			wait_for_work(work);
		}

		SFG_ASSERT(_works.begin() == _works.end());
	}

	editor_work_handle_t editor_work_controller_t::submit_work(const editor_work_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(desc.fn != nullptr);
		SFG_ASSERT(!_stop_requested.load(std::memory_order_acquire));

		const editor_work_handle_t handle = _works.emplace();

		SFG_ASSERT(!handle.is_null());

		work_t& work = _works.get(handle);

		if (desc.initial_status != nullptr)
			work.visible_progress_text = desc.initial_status;

		work.fn		   = desc.fn;
		work.completed = desc.completed;
		work.user_data = desc.user_data;
		work.progress.store(0.0f, std::memory_order_relaxed);
		work.state.store(editor_work_state_e::queued, std::memory_order_relaxed);
		work.completion_queued.store(false, std::memory_order_relaxed);

		const bool enqueued = _pending_work.enqueue({
			.work	= &work,
			.handle = handle,
		});

		SFG_ASSERT(enqueued);

		_last_submitted_work = handle;

		return handle;
	}

	void editor_work_controller_t::wait_for_work(editor_work_handle_t handle)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(std::this_thread::get_id() != _worker_thread.get_id());

		work_t& work			  = _works.get(handle);
		bool	completion_queued = work.completion_queued.load(std::memory_order_acquire);

		while (!completion_queued)
		{
			work.completion_queued.wait(false, std::memory_order_acquire);
			completion_queued = work.completion_queued.load(std::memory_order_acquire);
		}

		tick();
	}

	void editor_work_controller_t::get_work_status(editor_work_handle_t handle, editor_work_status_t& out_status) const
	{
		SFG_ASSERT(is_main_thread());

		const work_t& work = _works.get(handle);

		out_status.text		= work.visible_progress_text;
		out_status.progress = work.progress.load(std::memory_order_acquire);
		out_status.state	= work.state.load(std::memory_order_acquire);
	}

	void editor_work_controller_t::get_work_status(editor_work_handle_t handle, f32& out_progress, string_t& out_text) const
	{
		SFG_ASSERT(is_main_thread());

		const work_t& work = _works.get(handle);

		out_progress = work.progress.load(std::memory_order_acquire);
		out_text	 = work.visible_progress_text;
	}

	bool editor_work_controller_t::is_work_completed(editor_work_handle_t handle) const
	{
		SFG_ASSERT(is_main_thread());

		const editor_work_state_e state = _works.get(handle).state.load(std::memory_order_acquire);

		return state == editor_work_state_e::succeeded || state == editor_work_state_e::failed;
	}

	void editor_work_controller_t::worker_loop()
	{
#ifdef TRACY_ENABLE
		tracy::SetThreadName("editor work");
#endif

		while (true)
		{
			work_request_t request = {};

			if (!_pending_work.try_dequeue(request))
			{
				if (_stop_requested.load(std::memory_order_acquire))
					break;

				time_t::yield_thread();
				continue;
			}

			SFG_ASSERT(request.work);

			work_t& work = *request.work;

			work.state.store(editor_work_state_e::running, std::memory_order_release);

			editor_work_context_t	  context{*this, request.handle, &work};
			const bool				  succeeded = work.fn(context, work.user_data);
			const editor_work_state_e state		= succeeded ? editor_work_state_e::succeeded : editor_work_state_e::failed;

			if (succeeded)
				work.progress.store(1.0f, std::memory_order_relaxed);

			work.state.store(state, std::memory_order_release);
			work.state.notify_all();

			const bool enqueued = _completion_actions.enqueue({
				.fn		   = work.completed,
				.user_data = work.user_data,
				.handle	   = request.handle,
				.state	   = state,
			});

			SFG_ASSERT(enqueued);

			work.completion_queued.store(true, std::memory_order_release);
			work.completion_queued.notify_all();
		}
	}

	void editor_work_controller_t::set_work_progress(void* work_ptr, f32 progress, const char* text)
	{
		SFG_ASSERT(progress >= 0.0f && progress <= 1.0f);

		work_t& work = *static_cast<work_t*>(work_ptr);

		SFG_ASSERT(work.state.load(std::memory_order_acquire) == editor_work_state_e::running);

		work.progress.store(progress, std::memory_order_release);

		if (text != nullptr)
		{
			progress_text_slot_t* target_slot = nullptr;

			for (progress_text_slot_t& slot : work.progress_text_slots)
			{
				progress_text_slot_state_e expected = progress_text_slot_state_e::free;

				if (slot.state.compare_exchange_strong(expected, progress_text_slot_state_e::writing, std::memory_order_acq_rel, std::memory_order_relaxed))
				{
					target_slot = &slot;
					break;
				}
			}

			if (target_slot == nullptr)
			{
				for (progress_text_slot_t& slot : work.progress_text_slots)
				{
					progress_text_slot_state_e expected = progress_text_slot_state_e::ready;

					if (slot.state.compare_exchange_strong(expected, progress_text_slot_state_e::writing, std::memory_order_acq_rel, std::memory_order_relaxed))
					{
						target_slot = &slot;
						break;
					}
				}
			}

			if (target_slot != nullptr)
			{
				std::snprintf(target_slot->text, sizeof(target_slot->text), "%s", text);
				target_slot->sequence.store(work.next_progress_text_sequence.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);
				target_slot->state.store(progress_text_slot_state_e::ready, std::memory_order_release);
			}
		}
	}

	void editor_work_controller_t::drain_progress_text(work_t& work)
	{
		for (progress_text_slot_t& slot : work.progress_text_slots)
		{
			progress_text_slot_state_e expected = progress_text_slot_state_e::ready;

			if (!slot.state.compare_exchange_strong(expected, progress_text_slot_state_e::reading, std::memory_order_acquire, std::memory_order_relaxed))
				continue;

			const u64 sequence = slot.sequence.load(std::memory_order_relaxed);

			if (sequence > work.visible_progress_text_sequence)
			{
				work.visible_progress_text			= slot.text;
				work.visible_progress_text_sequence = sequence;
			}

			slot.state.store(progress_text_slot_state_e::free, std::memory_order_release);
		}
	}
}
