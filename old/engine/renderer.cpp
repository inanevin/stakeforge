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
#include "engine_config.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/util/gfx_util.hpp"
#include "common_engine.hpp"
#include "memory/frame_allocator.hpp"

namespace sfg
{

	u8 renderer_t::init()
	{
		if (gfx_backend::s_instance)
		{
			SFG_ERR("renderer is already init!");
			return static_cast<u8>(engine_error_code::renderer_already_init);
		}

		gfx_backend::s_instance = new gfx_backend();
		gfx_backend* backend	= gfx_backend::get();
		const u8	 result		= backend->init();
		if (result != static_cast<u8>(engine_error_code::none))
		{
			delete gfx_backend::s_instance;
			gfx_backend::s_instance = nullptr;
			return result;
		}

		_global_bind_layout			= gfx_util_t::create_bind_layout_global(false);
		_global_compute_bind_layout = gfx_util_t::create_bind_layout_global(true);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			_pfd[i].semaphore_frame.semaphore_t = backend->create_semaphore();
			_pfd[i].semaphore_frame.value		= 0;
		}

		return static_cast<u8>(engine_error_code::none);
	}

	void renderer_t::shutdown()
	{
		if (gfx_backend::s_instance == nullptr)
		{
			SFG_ERR("renderer is not initialized!");
			return;
		}

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd = _pfd[i];
			if (pfd.semaphore_frame.semaphore_t.is_null())
				continue;

			backend->destroy_semaphore(pfd.semaphore_frame.semaphore_t);
			pfd.semaphore_frame.semaphore_t = {};
			pfd.semaphore_frame.value		= 0;
		}

		backend->destroy_bind_layout(_global_bind_layout);
		backend->destroy_bind_layout(_global_compute_bind_layout);
		backend->uninit();

		delete gfx_backend::s_instance;
		gfx_backend::s_instance = nullptr;

		_global_bind_layout			= {};
		_global_compute_bind_layout = {};
		_frame_counter				= 0;
		_frame_index				= 0;
		_swapchains.clear();
	}

	void renderer_t::join()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data_t& pfd = _pfd[i];
			backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);
		}
	}

	void renderer_t::render()
	{
		gfx_backend*															backend	   = gfx_backend::get();
		const gfx_queue_handle													queue_gfx  = backend->get_queue_gfx();
		vector_t<gfx_swapchain_handle, frame_allocator_t<gfx_swapchain_handle>> swapchains = {};

		_frame_index = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);

		for (const renderer_swapchain_t& swp : _swapchains)
		{
			if (!swp.presentable)
				continue;

			backend->wait_for_swapchain_latency(swp.id);
			swapchains.push_back(swp.id);
		}

		per_frame_data_t& pfd = _pfd[_frame_index];
		backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);

		if (!swapchains.empty())
			backend->present(swapchains.data(), static_cast<u8>(swapchains.size()));

		pfd.semaphore_frame.value++;
		backend->queue_signal(queue_gfx, &pfd.semaphore_frame.semaphore_t, &pfd.semaphore_frame.value, 1);

		_frame_counter++;
	}

	gfx_swapchain_handle renderer_t::create_swapchain(const vec2u16_t& size, format_t format, void* window_handle, void* platform_handle)
	{
		SFG_ASSERT(size.x != 0 && size.y != 0);

		gfx_backend* backend = gfx_backend::get();

		const gfx_swapchain_handle id = backend->create_swapchain({
			.window_t  = window_handle,
			.os_handle = platform_handle,
			.scaling   = 1.0f,
			.format	   = format,
			.pos	   = vec2u16_t::zero,
			.size	   = size,
			.flags	   = swapchain_flags::sf_vsync_every_v_blank,
		});

		_swapchains.push_back({
			.id			 = id,
			.size		 = size,
			.format		 = format,
			.presentable = true,
		});

		return id;
	}

	void renderer_t::destroy_swapchain(gfx_swapchain_handle id)
	{
		renderer_swapchain_t* swp = nullptr;
		u32					  idx = 0;
		for (renderer_swapchain_t& current : _swapchains)
		{
			if (current.id == id)
			{
				swp = &current;
				break;
			}

			idx++;
		}

		SFG_ASSERT(swp != nullptr);

		gfx_backend* backend = gfx_backend::get();
		backend->destroy_swapchain(id);
		_swapchains.remove_index_swap(idx);
	}

	void renderer_t::resize_swapchain(gfx_swapchain_handle id, const vec2u16_t& size)
	{
		renderer_swapchain_t* swp = nullptr;
		for (renderer_swapchain_t& current : _swapchains)
		{
			if (current.id == id)
			{
				swp = &current;
				break;
			}
		}

		SFG_ASSERT(swp != nullptr);

		if (size.x == 0 || size.y == 0)
		{
			swp->presentable = false;
			swp->size		 = vec2u16_t::zero;
			return;
		}

		if (size == swp->size)
		{
			swp->presentable = true;
			return;
		}

		gfx_backend* backend = gfx_backend::get();

		backend->recreate_swapchain({
			.size		 = size,
			.swapchain_t = id,
			.scaling	 = 1.0f,
			.flags		 = swapchain_flags::sf_vsync_every_v_blank,
		});

		swp->size		 = size;
		swp->presentable = true;
	}

	gfx_texture_handle renderer_t::create_render_target(const vec2u16_t& size, format_t format)
	{
		gfx_backend* backend = gfx_backend::get();

		return backend->create_texture({
			.texture_format = format,
			.size			= size,
			.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
			.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
			.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
			.debug_name		= "surface_rt",
		});
	}

	void renderer_t::destroy_render_target(gfx_texture_handle id)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_texture(id);
	}

}
