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
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"

namespace sfg
{
	bool renderer_t::init()
	{
		if (gfx_backend::s_instance)
		{
			SFG_ERR("renderer is already init!");
			return false;
		}

		gfx_backend::s_instance = new gfx_backend();

		gfx_backend* backend = gfx_backend::get();
		if (!backend->init())
		{
			delete gfx_backend::s_instance;
			gfx_backend::s_instance = nullptr;
			return false;
		}

		_global_bind_layout			= gfx_util_t::create_bind_layout_global(false);
		_global_compute_bind_layout = gfx_util_t::create_bind_layout_global(true);

		return true;
	}

	void renderer_t::shutdown()
	{
		if (gfx_backend::s_instance == nullptr)
		{
			SFG_ERR("renderer is not initialized!");
			return;
		}

		gfx_backend* backend = gfx_backend::get();

		backend->destroy_bind_layout(_global_bind_layout);
		backend->destroy_bind_layout(_global_compute_bind_layout);
		backend->uninit();

		delete gfx_backend::s_instance;
		gfx_backend::s_instance = nullptr;

		_global_bind_layout			= NULL_GFX_ID;
		_global_compute_bind_layout = NULL_GFX_ID;
	}

	surface_id_t renderer_t::create_surface(const vec2u16_t& initial_size)
	{
		const surface_id_t id = _surfaces.add();
		surface_t&		   sf = _surfaces.get(id);
		sf.size				  = initial_size;
		return id;
	}

	void renderer_t::destroy_surface(surface_id_t id)
	{
		_surfaces.remove(id);
	}

	void renderer_t::resize_surface(surface_id_t id, const vec2u16_t& size)
	{
		surface_t& sf = _surfaces.get(id);
		sf.size		  = size;
	}
}
