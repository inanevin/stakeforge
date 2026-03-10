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

#include "renderer.hpp"
#include "app/engine_resources.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/common/render_target_definitions.hpp"

// game
#include "game/game_world_renderer.hpp"

// platform
#include "platform/window.hpp"
#include "platform/time.hpp"

// misc
#include "math/vector4.hpp"
#include "memory/memory_tracer.hpp"
#include "io/log.hpp"
#include "common/system_info.hpp"
#include "resources/common_resources.hpp"

#ifdef SFG_TOOLMODE
#include "editor/editor.hpp"
#endif

#include <tracy/Tracy.hpp>

namespace SFG
{
	gfx_id renderer::s_bind_group_global[BACK_BUFFER_COUNT] = {};
	gfx_id renderer::s_bind_layout_global					= 0;
	gfx_id renderer::s_bind_layout_global_compute			= 0;

	renderer::renderer(window& win, world& w, render_event_stream& event_stream, void* edt) : _world(w), _main_window(win), _event_stream(event_stream), _proxy_manager(_buffer_queue, _texture_queue)
	{
#ifdef SFG_TOOLMODE
		_editor = reinterpret_cast<editor*>(edt);
#endif
		_reuse_upload_barriers.reserve(256);
	}

	void renderer::create_bind_layout_global()
	{
		s_bind_layout_global		 = gfx_util::create_bind_layout_global(false);
		s_bind_layout_global_compute = gfx_util::create_bind_layout_global(true);
	}

	void renderer::destroy_bind_layout_global()
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_bind_layout(s_bind_layout_global);
		backend->destroy_bind_layout(s_bind_layout_global_compute);
		s_bind_layout_global		 = 0;
		s_bind_layout_global_compute = 0;
	}

	bool renderer::init(const vector2ui16& game_resolution)
	{
		gfx_backend* backend = gfx_backend::get();

		// swapchain
		_swapchain_flags = swapchain_flags::sf_vsync_every_v_blank;
		_base_size		 = _main_window.get_size();
		_world_size		 = game_resolution;

		_gfx_data.swapchain = backend->create_swapchain({
			.window	   = _main_window.get_window_handle(),
			.os_handle = _main_window.get_platform_handle(),
			.scaling   = 1.0f,
			.format	   = render_target_definitions::get_format_swapchain(),
			.pos	   = vector2ui16::zero,
			.size	   = _base_size,
			.flags	   = _swapchain_flags,
		});

		// debug & world
#ifdef SFG_USE_DEBUG_CONTROLLER
		_debug_controller.init(_main_window, &_texture_queue, s_bind_layout_global, _base_size);
#endif

		_world_renderer = new game_world_renderer(_proxy_manager, _world);
		_world_renderer->init(_world_size, &_texture_queue, &_buffer_queue, s_bind_layout_global, s_bind_layout_global_compute);

		// pfd
		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd		= _pfd[i];
			pfd.sem_frame.semaphore = backend->create_semaphore();
			pfd.sem_copy.semaphore	= backend->create_semaphore();
			pfd.cmd_gfx				= backend->create_command_buffer({
							.type		= command_type::graphics,
							.debug_name = "renderer_gfx",
			});

			pfd.cmd_copy = backend->create_command_buffer({
				.type		= command_type::transfer,
				.debug_name = "renderer_copy",
			});

			pfd.buf_engine_global.create({.size = sizeof(buf_engine_global), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "engine_cbv"});
			pfd.bind_group_global = backend->create_empty_bind_group();

			backend->bind_group_add_descriptor(pfd.bind_group_global, 0, binding_type::ubo);
			backend->bind_group_update_descriptor(pfd.bind_group_global, 0, pfd.buf_engine_global.get_gpu());

			_frame_allocator[i].init(1024 * 1024, 4);
			s_bind_group_global[i] = pfd.bind_group_global;
		}

		_buffer_queue.init();
		_texture_queue.init();
		_proxy_manager.init();

#ifdef SFG_TOOLMODE
		_shaders.swapchain = engine_resources::get().get_shader_direct(engine_resource_ident::shader_swapchain).get_hw(shader_variant_flags::variant_flag_console_and_editor);
		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_swapchain)
			{
				_shaders.swapchain = sh.get_hw(shader_variant_flags::variant_flag_console_and_editor);
				return;
			}
		});
#else

#ifdef SFG_DEBUG
		_shaders.swapchain = engine_resources::get().get_shader_direct(engine_resource_ident::shader_swapchain).get_hw(shader_variant_flags::variant_flag_console);
#else
		_shaders.swapchain = engine_resources::get().get_shader_direct(engine_resource_ident::shader_swapchain).get_hw();
#endif

#endif
		return true;
	}

	void renderer::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		// proxy
		_proxy_manager.fetch_render_events(_event_stream, 0);
		_proxy_manager.uninit();

		// debug & world
		_world_renderer->uninit();
		delete _world_renderer;
#ifdef SFG_USE_DEBUG_CONTROLLER
		_debug_controller.uninit();
#endif
		// utils
		_texture_queue.uninit();
		_buffer_queue.uninit();

		// pfd
		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_semaphore(pfd.sem_frame.semaphore);
			backend->destroy_semaphore(pfd.sem_copy.semaphore);
			backend->destroy_command_buffer(pfd.cmd_gfx);
			backend->destroy_command_buffer(pfd.cmd_copy);
			backend->destroy_bind_group(pfd.bind_group_global);
			pfd.buf_engine_global.destroy();
			_frame_allocator[i].uninit();
		}

		// swp
		backend->destroy_swapchain(_gfx_data.swapchain);
	}

	void renderer::wait_backend()
	{
		SFG_INFO("frame: {0}", frame_info::s_render_frame);

		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->wait_semaphore(pfd.sem_frame.semaphore, pfd.sem_frame.value);
		}

		_proxy_manager.flush_destroys(true);
		_gfx_data.frame_index = backend->get_back_buffer_index(_gfx_data.swapchain);
	}

	void renderer::tick()
	{
		ZoneScoped;

#ifdef SFG_USE_DEBUG_CONTROLLER
		_debug_controller.tick();
#endif

		_world_renderer->tick();
	}

	void renderer::render()
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		const gfx_id	  queue_gfx		 = backend->get_queue_gfx();
		const gfx_id	  queue_transfer = backend->get_queue_transfer();
		const vector2ui16 size			 = _base_size;

		{
			ZoneScopedN("wait latency");
			// Gate frame start to DXGI frame latency waitable for stable pacing
			backend->wait_for_swapchain_latency(_gfx_data.swapchain);
		}

		_gfx_data.frame_index = backend->get_back_buffer_index(_gfx_data.swapchain);
		const u8 frame_index  = _gfx_data.frame_index;

		_proxy_manager.fetch_render_events(_event_stream, frame_index);
		_proxy_manager.flush_destroys(false);

		/* access frame data */
		const gfx_id layout_global		   = s_bind_layout_global;
		const gfx_id layout_global_compute = s_bind_layout_global_compute;
		const gfx_id swapchain_rt		   = _gfx_data.swapchain;

		per_frame_data& pfd = _pfd[frame_index];

		bump_allocator& alloc = _frame_allocator[frame_index];
		alloc.reset();

		{
			ZoneScopedN("wait semaphore");
			// Wait for frame's fence, then send any uploads needed.
			backend->wait_semaphore(pfd.sem_frame.semaphore, pfd.sem_frame.value);
		}

		/* access pfd */
		const gfx_id cmd_list		  = pfd.cmd_gfx;
		const gfx_id cmd_list_copy	  = pfd.cmd_copy;
		const gfx_id bg_global		  = pfd.bind_group_global;
		const gfx_id shader_swp		  = _shaders.swapchain;
		const gfx_id sem_frame		  = pfd.sem_frame.semaphore;
		const gfx_id sem_copy		  = pfd.sem_copy.semaphore;
		const u64	 prev_copy_value  = pfd.sem_copy.value;
		const u64	 next_frame_value = ++pfd.sem_frame.value;

		// Handle uploads
		if (!_buffer_queue.empty(frame_index) || !_texture_queue.empty())
		{
			pfd.sem_copy.value++;
			backend->reset_command_buffer_transfer(cmd_list_copy);
			_buffer_queue.flush_all(cmd_list_copy, frame_index, _reuse_upload_barriers);
			_texture_queue.flush_all(cmd_list_copy, _reuse_upload_barriers);
			backend->close_command_buffer(cmd_list_copy);
			backend->submit_commands(queue_transfer, &cmd_list_copy, 1);
			backend->queue_signal(queue_transfer, &sem_copy, &pfd.sem_copy.value, 1);
		}

		// Begin frame cmd list
		backend->reset_command_buffer(cmd_list);
		backend->cmd_bind_layout(cmd_list, {.layout = layout_global});
		backend->cmd_bind_group(cmd_list, {.group = bg_global});

		root_constants rc = {};
		backend->cmd_bind_constants(cmd_list,
									{
										.data		 = (u8*)&rc,
										.offset		 = 0,
										.count		 = constant_index_max,
										.param_index = rpi_constants,
									});

		// Uploaded resources will have post-transitions.
		if (!_reuse_upload_barriers.empty())
		{
			backend->cmd_barrier(cmd_list,
								 {
									 .barriers		= _reuse_upload_barriers.data(),
									 .barrier_count = static_cast<u16>(_reuse_upload_barriers.size()),
								 });
			_reuse_upload_barriers.resize(0);
		}

		const u64 next_copy_value = pfd.sem_copy.value;

		const buf_engine_global globals = {
			.delta	 = static_cast<f32>(frame_info::get_render_thread_time_milli() * 0.001),
			.elapsed = static_cast<f32>(frame_info::get_render_thread_elapsed_seconds()),
		};
		pfd.buf_engine_global.buffer_data(0, (void*)&globals, sizeof(buf_engine_global));

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_render_target,
			.resource	 = swapchain_rt,
			.flags		 = barrier_flags::baf_is_swapchain,
		});

		backend->cmd_barrier(cmd_list,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });
		barriers.resize(0);
		const gpu_index rt_world_index = _world_renderer->get_output_gpu_index(frame_index);
		_world_renderer->prepare(frame_index);
		_world_renderer->render(frame_index, layout_global, layout_global_compute, bg_global, prev_copy_value, next_copy_value, sem_copy);

#ifdef SFG_TOOLMODE

		const gpu_index rt_editor_index = _editor->get_render_output(frame_index);

		_editor->render({
			.pm				   = _proxy_manager,
			.cmd_buffer		   = cmd_list,
			.frame_index	   = frame_index,
			.alloc			   = alloc,
			.size			   = size,
			.global_layout	   = layout_global,
			.global_group	   = bg_global,
			.world_rt_index	   = rt_world_index,
			.color_rt_index	   = _world_renderer->get_albedo_gpu_index(frame_index),
			.normals_rt_index  = _world_renderer->get_normal_gpu_index(frame_index),
			.orm_rt_index	   = _world_renderer->get_orm_gpu_index(frame_index),
			.emissive_rt_index = _world_renderer->get_emissive_gpu_index(frame_index),
			.lighting_rt_index = _world_renderer->get_lighting_gpu_index(frame_index),
			.ssao_rt_index	   = _world_renderer->get_ssao_gpu_index(frame_index),
			.bloom_rt_index	   = _world_renderer->get_bloom_gpu_index(frame_index),
			.depth_rt_index	   = _world_renderer->get_depth_gpu_index(frame_index),
		});

#endif

#ifdef SFG_USE_DEBUG_CONTROLLER
		const gpu_index rt_console_index = _debug_controller.get_output_gpu_index(frame_index);
		_debug_controller.prepare(frame_index);
		_debug_controller.render(cmd_list, frame_index, alloc);
#endif

		// swapchain pass
		{
			render_pass_color_attachment* attachment = alloc.allocate<render_pass_color_attachment>(1);
			attachment->clear_color					 = vector4(0.8f, 0.7f, 0.7f, 1.0f);
			attachment->load_op						 = load_op::clear;
			attachment->store_op					 = store_op::store;
			attachment->texture						 = swapchain_rt;
			attachment->view_index					 = 0;

			BEGIN_DEBUG_EVENT(backend, cmd_list, "swapchain_pass");
			backend->cmd_begin_render_pass_swapchain(cmd_list, {.color_attachments = attachment, .color_attachment_count = 1});
			backend->cmd_set_scissors(cmd_list, {.width = static_cast<u16>(size.x), .height = static_cast<u16>(size.y)});
			backend->cmd_set_viewport(cmd_list, {.width = static_cast<u16>(size.x), .height = static_cast<u16>(size.y)});

			// gui pass bind group
			{

				static_vector<gpu_index, 4> constants;

				constants.push_back(rt_world_index);

#ifdef SFG_USE_DEBUG_CONTROLLER
				constants.push_back(rt_console_index);
#endif

#ifdef SFG_TOOLMODE
				constants.push_back(rt_editor_index);
#endif

				backend->cmd_bind_constants(cmd_list,
											{
												.data		 = (u8*)constants.data(),
												.offset		 = constant_index_rp_constant0,
												.count		 = static_cast<u8>(constants.size()),
												.param_index = rpi_constants,
											});
			}

			backend->cmd_bind_pipeline(cmd_list, {.pipeline = shader_swp});
			backend->cmd_draw_instanced(cmd_list, {.vertex_count_per_instance = 6, .instance_count = 1});

			backend->cmd_end_render_pass(cmd_list, {});
			END_DEBUG_EVENT(backend, cmd_list);
		}

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = swapchain_rt,
			.flags		 = barrier_flags::baf_is_swapchain,
		});

		backend->cmd_barrier(cmd_list,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		/*
			End the frame command list.
			Insert queue wait if there were any uploads.
			Submit & present, then insert queue signal for this frame's fence.
		*/
		backend->close_command_buffer(cmd_list);
		// backend->queue_wait(queue_gfx, &sem_world, &sem_world_value, 1);

		backend->submit_commands(queue_gfx, &cmd_list, 1);

#ifdef SFG_USE_DEBUG_CONTROLLER
		const i64 time_before_wait = time::get_cpu_microseconds();
#endif

		{
			ZoneScopedN("present");
			backend->present(&swapchain_rt, 1);
		}
		backend->queue_signal(queue_gfx, &sem_frame, &next_frame_value, 1);
	}

	bool renderer::on_window_event(const window_event& ev)
	{
#ifdef SFG_USE_DEBUG_CONTROLLER
		return _debug_controller.on_window_event(ev);
#endif
		return false;
	}

	void renderer::on_window_resize(const vector2ui16& size)
	{
		SFG_VERIFY_THREAD_MAIN();
		if (size == _base_size)
			return;

		_base_size = (size.x == 0 || size.y == 0) ? vector2ui16(32, 32) : size;

		gfx_backend* backend = gfx_backend::get();

		backend->recreate_swapchain({
			.size	   = _base_size,
			.swapchain = _gfx_data.swapchain,
			.flags	   = _swapchain_flags,
		});

#ifdef SFG_USE_DEBUG_CONTROLLER
		_debug_controller.on_window_resize(_base_size);
#endif
#ifdef SFG_TOOLMODE
		_editor->resize(_base_size);
#endif
	}

	void renderer::on_swapchain_flags(u8 flags)
	{
		SFG_VERIFY_THREAD_MAIN();
		gfx_backend* backend = gfx_backend::get();
		_swapchain_flags	 = flags;
		backend->recreate_swapchain({
			.size	   = _base_size,
			.swapchain = _gfx_data.swapchain,
			.flags	   = _swapchain_flags,
		});
	}

	void renderer::on_world_resolution(const vector2ui16& size)
	{
		_world_size = (size.x == 0 || size.y == 0) ? vector2ui16(32, 32) : size;
		_world_renderer->resize(_world_size);
	}
}
