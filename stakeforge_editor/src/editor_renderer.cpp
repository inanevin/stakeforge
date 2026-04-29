// Copyright (c) 2025 Inan Evin

#include "editor_renderer.hpp"

#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_resources.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/shader_description.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"
#include "ui/vg/vg_canvas.hpp"

namespace sfg
{
	namespace
	{
		gfx_bind_layout_handle make_ui_default_layout(gfx_backend* backend)
		{
			const gfx_bind_layout_handle layout = backend->create_empty_bind_layout();
			backend->bind_layout_add_constant(layout, 16, 0, 0, shader_stage::vertex);
			backend->finalize_bind_layout(layout, false, false, "ui_default_layout");
			return layout;
		}

		gfx_bind_layout_handle make_ui_text_layout(gfx_backend* backend, const char* name)
		{
			const gfx_bind_layout_handle layout = backend->create_empty_bind_layout();
			backend->bind_layout_add_constant(layout, 16, 0, 0, shader_stage::vertex);
			backend->bind_layout_add_constant(layout, 4, 0, 1, shader_stage::fragment);

			sampler_desc_t samp = {};
			samp.flags			= sampler_flags::saf_min_linear | sampler_flags::saf_mag_linear | sampler_flags::saf_mip_linear | sampler_flags::saf_border_transparent;
			samp.address_u		= address_mode::clamp;
			samp.address_v		= address_mode::clamp;
			samp.address_w		= address_mode::clamp;
			samp.min_lod		= 0.0f;
			samp.max_lod		= 0.0f;
			backend->bind_layout_add_immutable_sampler(layout, 0, 0, samp, shader_stage::fragment);

			backend->finalize_bind_layout(layout, false, true, name);
			return layout;
		}
	}

	bool editor_renderer_t::init()
	{
		gfx_backend* backend = gfx_backend::get();
		if (backend == nullptr)
		{
			SFG_ERR("editor renderer requires an initialized backend!");
			return false;
		}

		const editor_resources_t& resources = editor_app_t::get().get_resources();

		const string_t shaders_dir	   = editor_directories_t::get_editor_assets() + "shaders/";
		const string_t ui_default_path = shaders_dir + "ui_default.hlsl";
		const string_t ui_text_path	   = shaders_dir + "ui_text.hlsl";
		const string_t ui_sdf_path	   = shaders_dir + "ui_sdf.hlsl";

		const editor_shader_t& ui_default = resources.get_resource<editor_shader_t>(TO_SID(ui_default_path.c_str()), editor_resource_type_e::shader);
		const editor_shader_t& ui_text	  = resources.get_resource<editor_shader_t>(TO_SID(ui_text_path.c_str()), editor_resource_type_e::shader);
		const editor_shader_t& ui_sdf	  = resources.get_resource<editor_shader_t>(TO_SID(ui_sdf_path.c_str()), editor_resource_type_e::shader);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd			= _pfd[i];
			pfd.semaphore_frame.semaphore_t = backend->create_semaphore();
			pfd.command_buffer				= backend->create_command_buffer({
							 .type		 = command_type::graphics,
							 .debug_name = "editor_gfx",
			 });
		}

		_swapchains.reserve(8);
		_eligible_targets.reserve(8);

		_ui_default_layout = make_ui_default_layout(backend);
		_ui_text_layout	   = make_ui_text_layout(backend, "ui_text_layout");
		_ui_sdf_layout	   = make_ui_text_layout(backend, "ui_sdf_layout");

		const ui::ui_render_group_t default_group = {.layout = _ui_default_layout, .group = {}, .pipeline = ui_default.handle};
		const ui::ui_render_group_t text_group	  = {.layout = _ui_text_layout, .group = {}, .pipeline = ui_text.handle};
		const ui::ui_render_group_t sdf_group	  = {.layout = _ui_sdf_layout, .group = {}, .pipeline = ui_sdf.handle};

		_ui_renderer.init(default_group, text_group, sdf_group);
		return true;
	}

	void editor_renderer_t::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		join();

		_ui_renderer.uninit();

		if (!_ui_default_layout.is_null())
			backend->destroy_bind_layout(_ui_default_layout);
		if (!_ui_text_layout.is_null())
			backend->destroy_bind_layout(_ui_text_layout);
		if (!_ui_sdf_layout.is_null())
			backend->destroy_bind_layout(_ui_sdf_layout);
		_ui_default_layout = {};
		_ui_text_layout	   = {};
		_ui_sdf_layout	   = {};

		for (gfx_swapchain_handle swapchain : _swapchains)
		{
			if (!swapchain.is_null())
				backend->destroy_swapchain(swapchain);
		}

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd = _pfd[i];

			if (!pfd.command_buffer.is_null())
			{
				backend->destroy_command_buffer(pfd.command_buffer);
				pfd.command_buffer = {};
			}

			if (!pfd.semaphore_frame.semaphore_t.is_null())
			{
				backend->destroy_semaphore(pfd.semaphore_frame.semaphore_t);
				pfd.semaphore_frame = {};
			}
		}

		_swapchains.resize(0);
		_eligible_targets.resize(0);
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

	void editor_renderer_t::render(span_t<const surface_render_target_t> targets)
	{
		gfx_backend* backend = gfx_backend::get();

		_eligible_targets.resize(0);
		for (size_t i = 0; i < targets.size; i++)
		{
			const surface_render_target_t& t = targets.data[i];
			backend->wait_for_swapchain_latency(t.swapchain);
			backend->get_back_buffer_index(t.swapchain);
			_eligible_targets.push_back(t);
		}

		if (_eligible_targets.empty())
			return;

		_frame_index		  = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);
		per_frame_data_t& pfd = _pfd[_frame_index];
		backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);

		const gfx_command_buffer_handle command_buffer = pfd.command_buffer;
		backend->reset_command_buffer(command_buffer);

		for (const surface_render_target_t& t : _eligible_targets)
		{
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

			if (t.canvas != nullptr)
				_ui_renderer.render(command_buffer, *t.canvas, _frame_index);

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

		const size_t				   target_count = _eligible_targets.size();
		vector_t<gfx_swapchain_handle> present_list;
		present_list.reserve(target_count);
		for (const surface_render_target_t& t : _eligible_targets)
			present_list.push_back(t.swapchain);
		backend->present(present_list.data(), static_cast<u8>(target_count));

		pfd.semaphore_frame.value++;
		backend->queue_signal(queue_gfx, &pfd.semaphore_frame.semaphore_t, &pfd.semaphore_frame.value, 1);

		_frame_counter++;
	}
}
