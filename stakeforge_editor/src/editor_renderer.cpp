// Copyright (c) 2025 Inan Evin

#include "editor_renderer.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/ui/vg/vg_atlas.hpp>
#include <sfg/ui/vg/vg_canvas.hpp>

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
		gfx_backend* backend = gfx_backend::get();
		if (backend == nullptr)
		{
			SFG_ERR("editor renderer requires an initialized backend!");
			return false;
		}

		const resource_pack_t& resources = editor_app_t::get().get_resources();

		const string_t shaders_dir	   = editor_directories_t::get_editor_assets() + "shaders/";
		const string_t ui_default_path = shaders_dir + "ui_default.hlsl";
		const string_t ui_text_path	   = shaders_dir + "ui_text.hlsl";
		const string_t ui_sdf_path	   = shaders_dir + "ui_sdf.hlsl";

		// const editor_shader_t& ui_default = resources.get_resource<editor_shader_t>(TO_SID(ui_default_path.c_str()), editor_resource_type_e::shader);
		// const editor_shader_t& ui_text	  = resources.get_resource<editor_shader_t>(TO_SID(ui_text_path.c_str()), editor_resource_type_e::shader);
		// const editor_shader_t& ui_sdf	  = resources.get_resource<editor_shader_t>(TO_SID(ui_sdf_path.c_str()), editor_resource_type_e::shader);
		//_ui_renderer.init(ui_default.handle, ui_text.handle, ui_sdf.handle);

		_global_layout = gfx_util_t::create_bind_layout_global(false);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd			= _pfd[i];
			pfd.semaphore_frame.semaphore_t = backend->create_semaphore();
			pfd.command_buffer				= backend->create_command_buffer({
							 .type		 = command_type::graphics,
							 .debug_name = "editor_gfx",
			 });

			resource_desc_t g_desc = {};
			g_desc.size			   = sizeof(global_buffer_data_t);
			g_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			g_desc.debug_name	   = "editor_global";
			pfd.global_buffer	   = backend->create_resource(g_desc);
			backend->map_resource(pfd.global_buffer, pfd.mapped_global);
			pfd.global_index = backend->get_resource_gpu_index(pfd.global_buffer);
		}

		_swapchains.reserve(8);
		_texture_queue.init();

		return true;
	}

	void editor_renderer_t::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		join();

		// _ui_renderer.uninit();
		_texture_queue.uninit();

		backend->destroy_bind_layout(_global_layout);
		_global_layout = {};

		for (gfx_swapchain_handle swapchain : _swapchains)
			backend->destroy_swapchain(swapchain);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd = _pfd[i];
			backend->destroy_resource(pfd.global_buffer);
			backend->destroy_command_buffer(pfd.command_buffer);
			backend->destroy_semaphore(pfd.semaphore_frame.semaphore_t);
			pfd = {};
		}

		_swapchains.resize(0);
		_elapsed_time  = 0.0f;
		_frame_counter = 0;
		_frame_index   = 0;
	}

	void editor_renderer_t::join()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			const per_frame_data_t& pfd = _pfd[i];
			if (!pfd.semaphore_frame.semaphore_t.is_null())
				backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);
		}
	}

	gfx_swapchain_handle editor_renderer_t::create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size)
	{
		gfx_backend* backend = gfx_backend::get();
		SFG_ASSERT(window_handle != nullptr);

		const gfx_swapchain_handle swapchain = backend->create_swapchain({
			.window_t  = window_handle,
			.os_handle = platform_handle,
			.scaling   = dpi_scale == 0.0f ? 1.0f : dpi_scale,
			.format	   = format_e::b8g8r8a8_srgb,
			.pos	   = vec2u16_t::zero,
			.size	   = size,
			.flags	   = swapchain_flags::sf_vsync_every_v_blank,
		});

		_swapchains.push_back(swapchain);
		return swapchain;
	}

	void editor_renderer_t::resize_swapchain(gfx_swapchain_handle swapchain, vec2u16_t size, f32 dpi_scale)
	{
		gfx_backend* backend = gfx_backend::get();
		SFG_ASSERT(!swapchain.is_null());

		backend->recreate_swapchain({
			.size		 = size,
			.swapchain_t = swapchain,
			.scaling	 = dpi_scale == 0.0f ? 1.0f : dpi_scale,
			.flags		 = swapchain_flags::sf_vsync_every_v_blank,
		});
	}

	void editor_renderer_t::destroy_swapchain(gfx_swapchain_handle swapchain)
	{
		gfx_backend* backend = gfx_backend::get();

		if (swapchain.is_null())
			return;

		backend->destroy_swapchain(swapchain);

		for (size_t i = 0; i < _swapchains.size(); i++)
		{
			if (_swapchains[i] == swapchain)
			{
				_swapchains[i] = _swapchains.back();
				_swapchains.pop_back();
				return;
			}
		}
	}

	void editor_renderer_t::render(span_t<const surface_render_target_t> targets, f32 delta_time)
	{
		gfx_backend* backend = gfx_backend::get();

		if (targets.size == 0)
			return;

		for (size_t i = 0; i < targets.size; i++)
		{
			const surface_render_target_t& t = targets.data[i];
			backend->wait_for_swapchain_latency(t.swapchain);
			backend->get_back_buffer_index(t.swapchain);
		}

		_elapsed_time += delta_time;
		_frame_index		  = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);
		per_frame_data_t& pfd = _pfd[_frame_index];
		backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);

		const global_buffer_data_t global_data = {.delta_time = delta_time, .elapsed_time = _elapsed_time};
		SFG_MEMCPY(pfd.mapped_global, &global_data, sizeof(global_buffer_data_t));

		const gfx_command_buffer_handle command_buffer = pfd.command_buffer;
		backend->reset_command_buffer(command_buffer);

		backend->cmd_bind_layout(command_buffer, {.layout = _global_layout});
		backend->cmd_bind_constants(command_buffer, {.data = &pfd.global_index, .offset = constant_global0, .count = 1, .param_index = 0});

		_texture_queue.flush(command_buffer);
		_texture_queue.transit(command_buffer);

		for (size_t i = 0; i < targets.size; i++)
		{
			const surface_render_target_t& t = targets.data[i];

			barrier_t barrier = {
				.from_states = resource_state_common,
				.to_states	 = resource_state_render_target,
				.swapchain_t = t.swapchain,
				.flags		 = barrier_flags::baf_is_swapchain,
			};
			backend->cmd_barrier(command_buffer, {.barriers = &barrier, .barrier_count = 1});

			render_pass_swapchain_attachment_t attachment = {
				.clear_color = vec4f_t(0.04f, 0.04f, 0.04f, 1.0f),
				.swapchain_t = t.swapchain,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			};

			backend->cmd_begin_render_pass_swapchain(command_buffer,
													 {
														 .color_attachments		 = &attachment,
														 .color_attachment_count = 1,
													 });

			command_set_viewport_t vp = {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = t.size.x, .height = t.size.y};
			backend->cmd_set_viewport(command_buffer, vp);
			backend->cmd_end_render_pass(command_buffer, {});

			barrier = {
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_common,
				.swapchain_t = t.swapchain,
				.flags		 = barrier_flags::baf_is_swapchain,
			};
			backend->cmd_barrier(command_buffer, {.barriers = &barrier, .barrier_count = 1});
		}

		backend->close_command_buffer(command_buffer);

		const gfx_queue_handle queue_gfx = backend->get_queue_gfx();
		backend->submit_commands(queue_gfx, &command_buffer, 1);

		frame_vector_t<gfx_swapchain_handle> present_list;

		for (size_t i = 0; i < targets.size; i++)
			present_list.push_back(targets.data[i].swapchain);
		backend->present(present_list.data(), static_cast<u8>(targets.size));

		pfd.semaphore_frame.value++;
		backend->queue_signal(queue_gfx, &pfd.semaphore_frame.semaphore_t, &pfd.semaphore_frame.value, 1);

		_frame_counter++;
	}
}
