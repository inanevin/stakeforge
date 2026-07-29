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

#include "game_renderer.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/texture_queue.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define GAME_BLIT_SHADER TO_SIDC("common/shaders/world/game_blit.hlsl")

	bool game_renderer_t::init(window_runtime_t& window, const game_renderer_config_t& config)
	{
		SFG_ASSERT(config.frame_budget_bytes != 0);
		SFG_ASSERT(window.window_handle != nullptr);
		SFG_ASSERT(window.platform_handle != nullptr);

		_frame_budget_bytes = config.frame_budget_bytes;
		_size				= window.size;
		_is_fullscreen		= config.is_fullscreen;
		_minimized			= window.has_flag(window_runtime_flags_e::minimized);

		render_resources_t&		  render_resources = render_resources_t::get();
		const shader_internals_t* blit_shader	   = resource_manager_t::get().find_internals<shader_internals_t>(GAME_BLIT_SHADER);

		if (blit_shader == nullptr)
			return false;

		_blit_shader = render_resources.get_shader_hw(blit_shader->find_pso(0));

		if (_blit_shader.is_null())
			return false;

		gfx_backend& backend = gfx_backend::get();

		for (u32 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
		{
			per_frame_data_t& pfd	   = _pfd[frame_index];
			pfd.semaphore_frame.sem	   = backend.create_semaphore();
			pfd.semaphore_transfer.sem = backend.create_semaphore();
			pfd.cmd_gfx				   = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "game_gfx",
			});
			pfd.cmd_gfx_prepare		   = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "game_prep",
			});
			pfd.cmd_gfx_transit		   = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "game_transit",
			});
			pfd.cmd_transfer		   = backend.create_command_buffer({
				.type		= command_type::transfer,
				.debug_name = "game_xfer",
			});
		}

		bitmask_t<u8> flags = static_cast<u8>(swapchain_flags::sf_vsync_every_v_blank);

		if (_is_fullscreen)
			flags.set(static_cast<u8>(swapchain_flags::sf_is_full_screen));

		_swapchain		 = backend.create_swapchain({
			.window_t  = window.window_handle,
			.os_handle = window.platform_handle,
			.scaling   = window.monitor_info.dpi_scale == 0.0f ? 1.0f : window.monitor_info.dpi_scale,
			.format	   = format_e::r8g8b8a8_srgb,
			.pos	   = vec2u16_t::zero,
			.size	   = _size,
			.flags	   = flags,
		});
		window.swapchain = _swapchain;

		if (!_swapchain.is_null())
			return true;

		for (u32 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
		{
			per_frame_data_t& pfd = _pfd[frame_index];
			backend.destroy_command_buffer(pfd.cmd_gfx);
			backend.destroy_command_buffer(pfd.cmd_gfx_prepare);
			backend.destroy_command_buffer(pfd.cmd_gfx_transit);
			backend.destroy_command_buffer(pfd.cmd_transfer);
			backend.destroy_semaphore(pfd.semaphore_frame.sem);
			backend.destroy_semaphore(pfd.semaphore_transfer.sem);
			pfd = {};
		}

		_frame_budget_bytes = 0;
		_size				= vec2u16_t::zero;
		_is_fullscreen		= false;
		_minimized			= false;

		return false;
	}

	void game_renderer_t::uninit()
	{
		end_render();

		gfx_backend& backend = gfx_backend::get();

		backend.destroy_swapchain(_swapchain);

		for (u32 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
		{
			per_frame_data_t& pfd = _pfd[frame_index];
			backend.destroy_command_buffer(pfd.cmd_gfx);
			backend.destroy_command_buffer(pfd.cmd_gfx_prepare);
			backend.destroy_command_buffer(pfd.cmd_gfx_transit);
			backend.destroy_command_buffer(pfd.cmd_transfer);
			backend.destroy_semaphore(pfd.semaphore_frame.sem);
			backend.destroy_semaphore(pfd.semaphore_transfer.sem);
			pfd = {};
		}

		_swapchain			= {};
		_blit_shader		= {};
		_size				= vec2u16_t::zero;
		_frame_budget_bytes = 0;
		_frame_counter		= 0;
		_is_fullscreen		= false;
		_minimized			= false;
	}

	void game_renderer_t::start()
	{
		SFG_ASSERT(!_render_thread_active.load());
		SFG_ASSERT(!_render_thread.joinable());

		_render_thread_active = true;
		_render_thread		  = std::thread(&game_renderer_t::render_loop, this);
	}

	void game_renderer_t::end_render()
	{
		if (!_render_thread_active.load() && !_render_thread.joinable())
			return;

		_render_thread_active = false;

		if (_render_thread.joinable())
			_render_thread.join();

		join();
	}

	void game_renderer_t::join()
	{
		gfx_backend& backend = gfx_backend::get();

		for (u32 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
		{
			const per_frame_data_t& pfd = _pfd[frame_index];
			backend.wait_semaphore(pfd.semaphore_frame.sem, pfd.semaphore_frame.value);
		}
	}

	void game_renderer_t::resize(vec2u16_t size, f32 dpi_scale, bool minimized)
	{
		const bool restart_render = _render_thread_active.load();

		end_render();

		_size	   = size;
		_minimized = minimized;

		bitmask_t<u8> flags = static_cast<u8>(swapchain_flags::sf_vsync_every_v_blank);

		if (_is_fullscreen)
			flags.set(static_cast<u8>(swapchain_flags::sf_is_full_screen));

		gfx_backend::get().recreate_swapchain({
			.size		 = minimized || size.x == 0 || size.y == 0 ? vec2u16_t{4, 4} : size,
			.swapchain_t = _swapchain,
			.scaling	 = dpi_scale == 0.0f ? 1.0f : dpi_scale,
			.flags		 = flags,
		});

		if (restart_render)
			start();
	}

	void game_renderer_t::render()
	{
		gfx_backend&		backend			 = gfx_backend::get();
		render_resources_t& render_resources = render_resources_t::get();

		render_resources.drain_requests();

		if (_minimized)
		{
			render_resources.drain_destroy_requests();
			return;
		}

		backend.wait_for_swapchain_latency(_swapchain);
		backend.get_back_buffer_index(_swapchain);

		const u8		  frame_index = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);
		per_frame_data_t& pfd		  = _pfd[frame_index];

		backend.wait_semaphore(pfd.semaphore_frame.sem, pfd.semaphore_frame.value);
		render_resources.flush_material_parameter_updates(frame_index);

		const gfx_handle_t queue_gfx	  = backend.get_queue_gfx();
		const gfx_handle_t queue_transfer = backend.get_queue_transfer();

		render_resources.get_texture_upload_queue().submit({
			.queue_gfx		= queue_gfx,
			.queue_transfer = queue_transfer,
			.cmd_prepare	= pfd.cmd_gfx_prepare,
			.cmd_transfer	= pfd.cmd_transfer,
			.cmd_transit	= pfd.cmd_gfx_transit,
			.semaphore		= &pfd.semaphore_transfer,
		});

		backend.reset_command_buffer(pfd.cmd_gfx);

		barrier_t barrier = {
			.from_states = resource_state_common,
			.to_states	 = resource_state_render_target,
			.swapchain_t = _swapchain,
			.flags		 = barrier_flags::baf_is_swapchain,
		};
		backend.cmd_barrier(pfd.cmd_gfx, {.barriers = &barrier, .barrier_count = 1});

		render_pass_swapchain_attachment_t attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.swapchain	 = _swapchain,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		backend.cmd_begin_render_pass_swapchain(pfd.cmd_gfx,
												{
													.color_attachments		= &attachment,
													.color_attachment_count = 1,
												});
		backend.cmd_set_viewport(pfd.cmd_gfx, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = _size.x, .height = _size.y});
		backend.cmd_set_scissors(pfd.cmd_gfx, {.x = 0, .y = 0, .width = _size.x, .height = _size.y});
		backend.cmd_bind_layout(pfd.cmd_gfx, {.layout = render_globals_t::get_global_bind_layout()});

		const gpu_index_t source_texture = render_resources.get_texture_gpu_index(render_resources.get_black_texture(), 0);

		backend.cmd_bind_constants(pfd.cmd_gfx, {.data = &source_texture, .offset = constant_rp0, .count = 1, .param_index = 0});
		backend.cmd_bind_pipeline(pfd.cmd_gfx, {.pipeline = _blit_shader});
		backend.cmd_draw_instanced(pfd.cmd_gfx, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});
		backend.cmd_end_render_pass(pfd.cmd_gfx, {});

		barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_common,
			.swapchain_t = _swapchain,
			.flags		 = barrier_flags::baf_is_swapchain,
		};
		backend.cmd_barrier(pfd.cmd_gfx, {.barriers = &barrier, .barrier_count = 1});
		backend.close_command_buffer(pfd.cmd_gfx);
		backend.submit_commands(queue_gfx, &pfd.cmd_gfx, 1);
		backend.present(&_swapchain, 1);

		pfd.semaphore_frame.value++;
		backend.queue_signal(queue_gfx, &pfd.semaphore_frame.sem, &pfd.semaphore_frame.value, 1);

		render_resources.drain_destroy_requests();
		_frame_counter++;
	}

	void game_renderer_t::render_loop()
	{
		frame_allocator_tls_t::init(_frame_budget_bytes);
		g_engine_thread_ids.render_thread_id = SFG_THIS_THREAD_ID();

#ifdef TRACY_ENABLE
		tracy::SetThreadName("render");
#endif

		while (_render_thread_active.load())
		{
			frame_allocator_tls_t::reset();
			render();
			FrameMarkNamed("render");
			time_t::yield_thread();
		}

		g_engine_thread_ids.render_thread_id = 0;
		frame_allocator_tls_t::uninit();
	}
}
