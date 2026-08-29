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

#pragma once

#include <sfg/data/atomic.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/bucketed_gen_pool.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

#include <thread>

namespace sfg
{
	class editor_work_controller_t;
	struct editor_work_handle_tag_t;

	using editor_work_handle_t = pool_handle_t<u32, editor_work_handle_tag_t>;

	enum class editor_work_state_e : u8
	{
		queued,
		running,
		succeeded,
		failed,
	};

	struct editor_work_status_t
	{
		string_t			text	 = {};
		f32					progress = 0.0f;
		editor_work_state_e state	 = editor_work_state_e::queued;
	};

	class editor_work_context_t final
	{
	public:
		editor_work_context_t()										   = delete;
		~editor_work_context_t()									   = default;
		editor_work_context_t(const editor_work_context_t&)			   = delete;
		editor_work_context_t& operator=(const editor_work_context_t&) = delete;

		void set_progress(f32 progress, const char* text = nullptr);

	private:
		friend class editor_work_controller_t;

		editor_work_context_t(editor_work_controller_t& controller, editor_work_handle_t handle, void* work);

		editor_work_controller_t* _controller = nullptr;
		void*					  _work		  = nullptr;
		editor_work_handle_t	  _handle	  = {};
	};

	using editor_work_fn		   = bool (*)(editor_work_context_t& context, void* user_data);
	using editor_work_completed_fn = void (*)(editor_work_handle_t handle, editor_work_state_e state, void* user_data);

	struct editor_work_desc_t
	{
		editor_work_fn			 fn				= nullptr;
		editor_work_completed_fn completed		= nullptr;
		void*					 user_data		= nullptr;
		const char*				 initial_status = nullptr;
	};

	class editor_work_controller_t final
	{
	public:
		editor_work_controller_t()											 = default;
		~editor_work_controller_t()											 = default;
		editor_work_controller_t(const editor_work_controller_t&)			 = delete;
		editor_work_controller_t& operator=(const editor_work_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void tick();
		void wait_for_all();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		editor_work_handle_t submit_work(const editor_work_desc_t& desc);
		void				 wait_for_work(editor_work_handle_t handle);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		void get_work_status(editor_work_handle_t handle, editor_work_status_t& out_status) const;
		void get_work_status(editor_work_handle_t handle, f32& out_progress, string_t& out_text) const;
		bool is_work_completed(editor_work_handle_t handle) const;

	private:
		friend class editor_work_context_t;

		static inline constexpr size_t PROGRESS_TEXT_CAPACITY = 256;

		enum class progress_text_slot_state_e : u8
		{
			free,
			writing,
			ready,
			reading,
		};

		struct progress_text_slot_t
		{
			atomic_t<u64>						 sequence					  = 0;
			atomic_t<progress_text_slot_state_e> state						  = progress_text_slot_state_e::free;
			char								 text[PROGRESS_TEXT_CAPACITY] = {};
		};

		struct work_t
		{
			string_t					  visible_progress_text			 = {};
			progress_text_slot_t		  progress_text_slots[2]		 = {};
			atomic_t<u64>				  next_progress_text_sequence	 = 1;
			editor_work_fn				  fn							 = nullptr;
			editor_work_completed_fn	  completed						 = nullptr;
			void*						  user_data						 = nullptr;
			atomic_t<f32>				  progress						 = 0.0f;
			atomic_t<editor_work_state_e> state							 = editor_work_state_e::queued;
			atomic_t<bool>				  completion_queued				 = false;
			u64							  visible_progress_text_sequence = 0;
		};

		struct work_request_t
		{
			work_t*				 work	= nullptr;
			editor_work_handle_t handle = {};
		};

		struct completion_action_t
		{
			editor_work_completed_fn fn		   = nullptr;
			void*					 user_data = nullptr;
			editor_work_handle_t	 handle	   = {};
			editor_work_state_e		 state	   = editor_work_state_e::queued;
		};

		void worker_loop();
		void set_work_progress(void* work, f32 progress, const char* text);
		void drain_progress_text(work_t& work);

	private:
		bucketed_gen_pool_t<work_t, editor_work_handle_tag_t> _works;
		moodycamel::ConcurrentQueue<work_request_t>			  _pending_work;
		moodycamel::ConcurrentQueue<completion_action_t>	  _completion_actions;
		std::thread											  _worker_thread;
		editor_work_handle_t								  _last_submitted_work = {};
		atomic_t<bool>										  _stop_requested	   = false;
	};
}
