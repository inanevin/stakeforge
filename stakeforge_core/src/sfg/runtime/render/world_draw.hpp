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

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	enum class world_renderable_type_e : u8
	{
		mesh,
		sprite,
	};

	struct alignas(64) world_renderable_t
	{
		u64						sort_key	   = 0;
		aabb_t					aabb		   = {};
		u32						payload_index  = UINT32_MAX;
		u32						material_index = UINT32_MAX;
		u32						entity_index   = UINT32_MAX;
		u32						pass_mask	   = 0;
		world_renderable_type_e type		   = world_renderable_type_e::mesh;
	};

	struct world_sprite_draw_t
	{
		render_resource_handle_t texture  = {};
		vec2f_t					 uv_start = vec2f_t::zero;
		vec2f_t					 uv_size  = vec2f_t::zero;
		vec2f_t					 size	  = vec2f_t::zero;
	};

	struct world_draw_sprite_instance_gpu_t
	{
		vec2f_t		uv_start	  = vec2f_t::zero;
		vec2f_t		uv_size		  = vec2f_t::zero;
		vec2f_t		size		  = vec2f_t::zero;
		gpu_index_t texture_index = NULL_GPU_INDEX;
		u32			entity_index  = UINT32_MAX;
		u32			entity_id	  = UINT32_MAX;
		u32			pad			  = 0;
	};

	static_assert(sizeof(world_renderable_t) == 64);
	static_assert(sizeof(world_draw_sprite_instance_gpu_t) == 40);

	struct world_mesh_draw_t
	{
		render_resource_handle_t vertex_buffer	= {};
		render_resource_handle_t index_buffer	= {};
		render_resource_handle_t direct_pso		= {};
		u32						 draw_flags		= 0;
		u32						 skinning_index = UINT32_MAX;
		u32						 index_count	= 0;
		u32						 vertex_count	= 0;
		u32						 start_index	= 0;
		u32						 start_vertex	= 0;
		u32						 start_instance = 0;
		u16						 vertex_stride	= 0;
		u8						 index_stride	= 0;
	};
}
