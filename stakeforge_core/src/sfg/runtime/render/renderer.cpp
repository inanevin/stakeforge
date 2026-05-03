// Copyright (c) 2025 Inan Evin

#include "renderer.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool renderer_t::init()
	{
		gfx_backend* backend = gfx_backend::get();
		if (backend == nullptr)
		{
			SFG_ERR("renderer requires an initialized backend!");
			return false;
		}

		_global_bind_layout			= gfx_util_t::create_bind_layout_global(false);
		_global_compute_bind_layout = gfx_util_t::create_bind_layout_global(true);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			_pfd[i].semaphore_frame.semaphore_t = backend->create_semaphore();
			_pfd[i].semaphore_frame.value		= 0;
		}

		return true;
	}

	void renderer_t::uninit()
	{
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

		_global_bind_layout			= {};
		_global_compute_bind_layout = {};
		_frame_counter				= 0;
		_frame_index				= 0;
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
		gfx_backend* backend = gfx_backend::get();

		const gfx_queue_handle queue_gfx = backend->get_queue_gfx();
		_frame_index					 = static_cast<u8>(_frame_counter % BACK_BUFFER_COUNT);

		per_frame_data_t& pfd = _pfd[_frame_index];
		backend->wait_semaphore(pfd.semaphore_frame.semaphore_t, pfd.semaphore_frame.value);

		pfd.semaphore_frame.value++;
		backend->queue_signal(queue_gfx, &pfd.semaphore_frame.semaphore_t, &pfd.semaphore_frame.value, 1);

		_frame_counter++;
	}
}
