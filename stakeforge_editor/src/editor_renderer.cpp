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

#include "editor_renderer.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/gfx/common/texture_queue.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg
{
	namespace
	{
		struct global_buffer_data_t
		{
			f32 delta_time	 = 0.0f;
			f32 elapsed_time = 0.0f;
		};
	}

	bool editor_renderer_t::init()
	{
		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd	   = _pfd[i];
			pfd.semaphore_frame.sem	   = backend.create_semaphore();
			pfd.semaphore_transfer.sem = backend.create_semaphore();
			pfd.cmd_gfx				   = backend.create_command_buffer({
							   .type	   = command_type::graphics,
							   .debug_name = "editor_gfx",
			   });
			pfd.cmd_gfx_prepare		   = backend.create_command_buffer({
					   .type	   = command_type::graphics,
					   .debug_name = "editor_prep",
			   });
			pfd.cmd_transfer		   = backend.create_command_buffer({
						  .type		  = command_type::transfer,
						  .debug_name = "editor_xfer",
			  });

			resource_desc_t desc = {};
			desc.size			 = sizeof(global_buffer_data_t);
			desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			desc.set_name("editor_global");
			pfd.global_buffer = backend.create_resource(desc);
			backend.map_resource(pfd.global_buffer, pfd.mapped_global);
			pfd.global_index = backend.get_resource_gpu_index(pfd.global_buffer);
		}

		_render_targets.reserve(8);
		_ui_renderer.init();

		return true;
	}

	void editor_renderer_t::uninit()
	{
		gfx_backend& backend = gfx_backend::get();

		join();

		_ui_renderer.uninit();

		for (const surface_render_target_t& t : _render_targets)
			backend.destroy_swapchain(t.swapchain);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd = _pfd[i];
			backend.destroy_resource(pfd.global_buffer);
			backend.destroy_command_buffer(pfd.cmd_gfx);
			backend.destroy_command_buffer(pfd.cmd_gfx_prepare);
			backend.destroy_command_buffer(pfd.cmd_transfer);
			backend.destroy_semaphore(pfd.semaphore_frame.sem);
			backend.destroy_semaphore(pfd.semaphore_transfer.sem);
			pfd = {};
		}

		_render_targets.resize(0);
		_frame_counter = 0;
		_frame_index   = 0;
	}

	void editor_renderer_t::join()
	{
		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			const per_frame_data_t& pfd = _pfd[i];
			backend.wait_semaphore(pfd.semaphore_frame.sem, pfd.semaphore_frame.value);
		}
	}

	gfx_swapchain_handle editor_renderer_t::create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size, ui::ui_context* ui)
	{
		gfx_backend& backend = gfx_backend::get();
		SFG_ASSERT(window_handle != nullptr);

		const gfx_swapchain_handle swapchain = backend.create_swapchain({
			.window_t  = window_handle,
			.os_handle = platform_handle,
			.scaling   = dpi_scale == 0.0f ? 1.0f : dpi_scale,
			.format	   = format_e::b8g8r8a8_srgb,
			.pos	   = vec2u16_t::zero,
			.size	   = size,
			.flags	   = swapchain_flags::sf_vsync_every_v_blank,
		});

		_render_targets.push_back({
			.swapchain = swapchain,
			.ui		   = ui,
			.size	   = size,
			.minimized = false,
		});

		return swapchain;
	}

	void editor_renderer_t::resize_swapchain(gfx_swapchain_handle swapchain, vec2u16_t size, f32 dpi_scale)
	{
		gfx_backend& backend = gfx_backend::get();
		SFG_ASSERT(!swapchain.is_null());

		auto it = std::find_if(_render_targets.begin(), _render_targets.end(), [swapchain](const surface_render_target_t& t) -> bool { return t.swapchain == swapchain; });
		SFG_ASSERT(it != _render_targets.end());
		it->size = size;

		backend.recreate_swapchain({
			.size		 = (size.x == 0 || size.y == 0) ? vec2u16_t(4, 4) : size,
			.swapchain_t = swapchain,
			.scaling	 = dpi_scale == 0.0f ? 1.0f : dpi_scale,
			.flags		 = swapchain_flags::sf_vsync_every_v_blank,
		});
	}

	void editor_renderer_t::destroy_swapchain(gfx_swapchain_handle swapchain)
	{
		gfx_backend& backend = gfx_backend::get();
		if (swapchain.is_null())
			return;

		auto it = std::find_if(_render_targets.begin(), _render_targets.end(), [swapchain](const surface_render_target_t& t) -> bool { return t.swapchain == swapchain; });
		SFG_ASSERT(it != _render_targets.end());

		backend.destroy_swapchain(swapchain);
		_render_targets.erase(it);
	}

	void editor_renderer_t::set_swapchain_minimized(gfx_swapchain_handle handle, bool is_minimized)
	{
		auto it = std::find_if(_render_targets.begin(), _render_targets.end(), [handle](const surface_render_target_t& t) -> bool { return t.swapchain == handle; });
		SFG_ASSERT(it != _render_targets.end());
		it->minimized = is_minimized;
	}

	void editor_renderer_t::render()
	{
		render_resources_t::get().drain_requests();
		texture_queue_t& texture_queue = render_resources_t::get().get_texture_upload_queue();
		gfx_backend&	 backend	   = gfx_backend::get();

		/* figure out swapchain list */
		struct rt_t
		{
			gfx_swapchain_handle swapchain;
			ui::ui_context*		 ui;
			vec2u16_t			 size;
		};

		frame_vector_t<rt_t>				 render_targets;
		frame_vector_t<gfx_swapchain_handle> present_list;
		for (const surface_render_target_t& t : _render_targets)
		{
			if (t.minimized)
				continue;
			backend.wait_for_swapchain_latency(t.swapchain);
			backend.get_back_buffer_index(t.swapchain);
			render_targets.push_back({t.swapchain, t.ui, t.size});
			present_list.push_back(t.swapchain);
		}

		if (render_targets.empty())
			return;

		/* time calc, globals, frame index */

		const i64  now			= time_t::get_cpu_microseconds();
		static i64 prev_time	= now;
		const i64  delta		= now - prev_time;
		prev_time				= now;
		static f32 elapsed_time = 0.0f;
		elapsed_time += time_t::micro_to_s(delta);
		const f32 delta_time = time_t::micro_to_s(delta);

		_frame_index		  = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);
		per_frame_data_t& pfd = _pfd[_frame_index];

		backend.wait_semaphore(pfd.semaphore_frame.sem, pfd.semaphore_frame.value);

		const global_buffer_data_t global_data = {.delta_time = delta_time, .elapsed_time = elapsed_time};
		SFG_MEMCPY(pfd.mapped_global, &global_data, sizeof(global_buffer_data_t));

		/* graphics & transfer */

		const gfx_queue_handle			queue_gfx	   = backend.get_queue_gfx();
		const gfx_queue_handle			queue_transfer = backend.get_queue_transfer();
		const gfx_command_buffer_handle cmd			   = pfd.cmd_gfx;
		const gfx_command_buffer_handle cmd_prepare	   = pfd.cmd_gfx_prepare;
		const gfx_command_buffer_handle cmd_transfer   = pfd.cmd_transfer;

		/* flush uploads, begin graphics & transits */

		bool prepare_submitted	= false;
		bool transfer_submitted = false;
		if (texture_queue.has_uploads())
		{
			backend.reset_command_buffer(cmd_prepare);
			const bool prepare_emitted = texture_queue.prepare(cmd_prepare);
			backend.close_command_buffer(cmd_prepare);
			if (prepare_emitted)
			{
				backend.submit_commands(queue_gfx, &cmd_prepare, 1);
				pfd.semaphore_transfer.value++;
				backend.queue_signal(queue_gfx, &pfd.semaphore_transfer.sem, &pfd.semaphore_transfer.value, 1);
				prepare_submitted = true;
			}

			if (prepare_submitted)
				backend.queue_wait(queue_transfer, &pfd.semaphore_transfer.sem, &pfd.semaphore_transfer.value, 1);

			backend.reset_command_buffer_transfer(cmd_transfer);
			if (texture_queue.flush(cmd_transfer))
			{
				backend.close_command_buffer(cmd_transfer);
				backend.submit_commands(queue_transfer, &cmd_transfer, 1);
				pfd.semaphore_transfer.value++;
				backend.queue_signal(queue_transfer, &pfd.semaphore_transfer.sem, &pfd.semaphore_transfer.value, 1);
				transfer_submitted = true;
			}
		}

		backend.reset_command_buffer(cmd);
		backend.cmd_bind_layout(cmd, {.layout = render_globals_t::get_global_bind_layout()});
		backend.cmd_bind_constants(cmd, {.data = &pfd.global_index, .offset = constant_global0, .count = 1, .param_index = 0});
		texture_queue.transit(cmd);

		/* render pass per rt */

		for (const rt_t& t : render_targets)
		{
			barrier_t barrier = {
				.from_states = resource_state_common,
				.to_states	 = resource_state_render_target,
				.swapchain_t = t.swapchain,
				.flags		 = barrier_flags::baf_is_swapchain,
			};
			backend.cmd_barrier(cmd, {.barriers = &barrier, .barrier_count = 1});

			render_pass_swapchain_attachment_t attachment = {
				.clear_color = vec4f_t(0, 0, 0, 1.0f),
				.swapchain_t = t.swapchain,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			};

			backend.cmd_begin_render_pass_swapchain(cmd,
													{
														.color_attachments		= &attachment,
														.color_attachment_count = 1,
													});

			command_set_viewport_t vp = {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = t.size.x, .height = t.size.y};
			backend.cmd_set_viewport(cmd, vp);

			if (t.ui != nullptr)
				_ui_renderer.render(cmd, *t.ui, _frame_index, t.size);

			backend.cmd_end_render_pass(cmd, {});

			barrier = {
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_common,
				.swapchain_t = t.swapchain,
				.flags		 = barrier_flags::baf_is_swapchain,
			};
			backend.cmd_barrier(cmd, {.barriers = &barrier, .barrier_count = 1});
		}

		/* submit & present */
		backend.close_command_buffer(cmd);
		if (transfer_submitted)
			backend.queue_wait(queue_gfx, &pfd.semaphore_transfer.sem, &pfd.semaphore_transfer.value, 1);

		backend.submit_commands(queue_gfx, &cmd, 1);
		backend.present(present_list.data(), static_cast<u8>(present_list.size()));
		pfd.semaphore_frame.value++;
		backend.queue_signal(queue_gfx, &pfd.semaphore_frame.sem, &pfd.semaphore_frame.value, 1);

		_frame_counter++;
	}

	void editor_renderer_t::ensure_render()
	{
		if (_render_thread_active.load())
			return;

		_render_thread_active = true;
		_render_thread		  = std::thread(&editor_renderer_t::render_loop, this);
	}

	void editor_renderer_t::end_render()
	{
		if (!_render_thread_active.load() && !_render_thread.joinable())
			return;

		_render_thread_active = false;

		if (_render_thread.joinable())
			_render_thread.join();

		join();
	}

	void editor_renderer_t::render_loop()
	{
		frame_allocator_tls_t::init(RENDER_FRAME_ALLOC_SIZE);
		g_engine_thread_ids.render_thread_id = SFG_THIS_THREAD_ID();

		while (_render_thread_active.load())
		{
			frame_allocator_tls_t::reset();
			render();
			time_t::yield_thread();
		}

		g_engine_thread_ids.render_thread_id = 0;
		frame_allocator_tls_t::uninit();
	}
}
