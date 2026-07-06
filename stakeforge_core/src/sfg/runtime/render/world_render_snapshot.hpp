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

#include "world_draw.hpp"
#include "world_render_entity.hpp"
#include "world_render_material.hpp"
#include "world_render_view.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	struct world_render_skybox_t
	{
		render_resource_handle_t radiance	= {};
		render_resource_handle_t irradiance = {};
		render_resource_handle_t prefilter	= {};
		render_resource_handle_t brdf_lut	= {};
		f32						 intensity	= 1.0f;
		f32						 exposure	= 1.0f;
	};

	struct world_render_prep_draw_cull_t
	{
		u64 cull_mask = 0;
	};

	struct world_render_prep_data_t
	{
		frame_vector_t<world_render_prep_draw_cull_t> draw_culls = {};
	};

	struct world_render_snapshot_t
	{
		world_render_view_t				  main_view = {};
		world_render_skybox_t			  skybox	= {};
		vector_t<world_render_material_t> materials = {};
		vector_t<world_render_entity_t>	  entities	= {};
		vector_t<world_draw_t>			  draws		= {};

		inline void reserve(size_t entity_count)
		{
			entities.reserve(entity_count);
		}
	};
}
