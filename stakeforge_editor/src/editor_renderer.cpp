// Copyright (c) 2025 Inan Evin

#include "editor_renderer.hpp"

#include "gfx/backend/backend.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/common/descriptions.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"

namespace sfg
{
	bool editor_renderer_t::init()
	{
		gfx_backend* backend = gfx_backend::get();
		if (backend == nullptr)
		{
			SFG_ERR("editor renderer requires an initialized backend!");
			return false;
		}

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd			= _pfd[i];
			pfd.semaphore_frame.semaphore_t = backend->create_semaphore();
			pfd.command_buffer				= backend->create_command_buffer({
							 .type		 = command_type::graphics,
							 .debug_name = "editor_gfx",
			 });
		}

		return true;
	}

	void editor_renderer_t::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		join();

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
		_eligible_swapchains.resize(0);
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
			.format	   = format_t::b8g8r8a8_srgb,
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

	void editor_renderer_t::render()
	{
		gfx_backend* backend = gfx_backend::get();

		_eligible_swapchains.resize(0);

		for (gfx_swapchain_handle swapchain : _swapchains)
		{
			backend->wait_for_swapchain_latency(swapchain);
			backend->get_back_buffer_index(swapchain);
			_eligible_swapchains.push_back(swapchain);
		}

		if (_eligible_swapchains.empty())
			return;

		_frame_index		  = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);
		per_frame_data_t& pfd = _pfd[_frame_index];
		backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);

		const gfx_command_buffer_handle command_buffer = pfd.command_buffer;
		backend->reset_command_buffer(command_buffer);

		for (gfx_swapchain_handle swapchain : _eligible_swapchains)
		{
			barrier_t barrier = {
				.from_states = resource_state_common,
				.to_states	 = resource_state_render_target,
				.swapchain_t = swapchain,
				.flags		 = barrier_flags::baf_is_swapchain,
			};
			backend->cmd_barrier(command_buffer, {.barriers = &barrier, .barrier_count = 1});

			render_pass_swapchain_attachment_t attachment = {
				.clear_color = vec4f_t(0.04f, 0.04f, 0.04f, 1.0f),
				.swapchain_t = swapchain,
				.load_op	 = load_op::clear,
				.store_op	 = store_op::store,
				.view_index	 = 0,
			};

			backend->cmd_begin_render_pass_swapchain(command_buffer,
													 {
														 .color_attachments		 = &attachment,
														 .color_attachment_count = 1,
													 });
			backend->cmd_end_render_pass(command_buffer, {});

			barrier = {
				.from_states = resource_state_render_target,
				.to_states	 = resource_state_common,
				.swapchain_t = swapchain,
				.flags		 = barrier_flags::baf_is_swapchain,
			};
			backend->cmd_barrier(command_buffer, {.barriers = &barrier, .barrier_count = 1});
		}

		backend->close_command_buffer(command_buffer);

		const gfx_queue_handle queue_gfx = backend->get_queue_gfx();
		backend->submit_commands(queue_gfx, &command_buffer, 1);
		backend->present(_eligible_swapchains.data(), static_cast<u8>(_eligible_swapchains.size()));

		pfd.semaphore_frame.value++;
		backend->queue_signal(queue_gfx, &pfd.semaphore_frame.semaphore_t, &pfd.semaphore_frame.value, 1);

		_frame_counter++;
	}
}
