/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include "core/common/size_definitions.hpp"
#include "core/gfx/common/gfx_constants.hpp"
#include "core/math/aabb.hpp"
#include "core/math/vec2f.hpp"
#include "core/math/vec3f.hpp"
#include "core/math/vec4f.hpp"
#include "core/runtime/render/render_resource_handle.hpp"

namespace sfg
{
	struct stock_world_renderer_draw_state_t
	{
		gfx_handle_t vertex_buffer	= {};
		gfx_handle_t index_buffer	= {};
		gfx_handle_t pipeline		= {};
		u32			 material_index = UINT32_MAX;
		u16			 vertex_stride	= 0;
		u8			 index_stride	= 0;
	};

	struct stock_world_renderer_draw_packet_t
	{
		gfx_handle_t pipeline			   = {};
		gfx_handle_t vertex_buffer		   = {};
		gfx_handle_t index_buffer		   = {};
		gpu_index_t	 instance_buffer_index = NULL_GPU_INDEX;
		u32			 object_constants[8]   = {};
		u32			 material_index		   = UINT32_MAX;
		u32			 element_count		   = 0;
		u32			 instance_count		   = 1;
		u32			 start_index		   = 0;
		u32			 start_vertex		   = 0;
		u16			 vertex_stride		   = 0;
		u8			 index_stride		   = 0;
		u8			 object_constant_count = 0;
	};

	enum class stock_world_renderer_renderable_type_e : u8
	{
		mesh,
		sprite,
		particle,
	};

	enum stock_world_renderer_renderable_flags_e : u8
	{
		stock_world_renderer_renderable_flag_none			  = 0,
		stock_world_renderer_renderable_flag_world_space_aabb = 1 << 0,
		stock_world_renderer_renderable_flag_view_model		  = 1 << 1,
	};

	struct alignas(64) stock_world_renderer_renderable_t
	{
		u64									   sort_key		  = 0;
		aabb_t								   aabb			  = {};
		u32									   payload_index  = UINT32_MAX;
		u32									   material_index = UINT32_MAX;
		u32									   entity_index	  = UINT32_MAX;
		u32									   pass_mask	  = 0;
		stock_world_renderer_renderable_type_e type			  = stock_world_renderer_renderable_type_e::mesh;
		u8									   flags		  = stock_world_renderer_renderable_flag_none;
	};

	struct stock_world_renderer_draw_mesh_instance_gpu_t
	{
		u32 entity_index   = UINT32_MAX;
		u32 skinning_index = UINT32_MAX;
		u32 entity_id	   = UINT32_MAX;
	};

	struct stock_world_renderer_sprite_draw_t
	{
		render_resource_handle_t texture		  = {};
		vec2f_t					 uv_start		  = vec2f_t::zero;
		vec2f_t					 uv_size		  = vec2f_t::zero;
		vec2f_t					 size			  = vec2f_t::zero;
		u8						 is_linear_sample = 0;
	};

	struct stock_world_renderer_draw_sprite_instance_gpu_t
	{
		vec2f_t		uv_start		 = vec2f_t::zero;
		vec2f_t		uv_size			 = vec2f_t::zero;
		vec2f_t		size			 = vec2f_t::zero;
		gpu_index_t texture_index	 = NULL_GPU_INDEX;
		u32			entity_index	 = UINT32_MAX;
		u32			entity_id		 = UINT32_MAX;
		u32			is_linear_sample = 0;
	};

	struct stock_world_renderer_debug_draw_texture_gpu_t
	{
		vec4f_t		color		  = vec4f_t::zero;
		vec3f_t		position	  = vec3f_t::zero;
		gpu_index_t texture_index = NULL_GPU_INDEX;
		vec2f_t		size_px		  = vec2f_t::zero;
		vec2f_t		screen_offset = vec2f_t::zero;
		u32			flags		  = 0;
	};

	struct stock_world_renderer_particle_t
	{
		vec3f_t position		  = vec3f_t::zero;
		f32		rotation		  = 0.0f;
		vec3f_t previous_position = vec3f_t::zero;
		f32		size			  = 0.0f;
		vec3f_t velocity		  = vec3f_t::zero;
		f32		pad				  = 0.0f;
		vec4f_t color			  = vec4f_t::zero;
	};

	struct stock_world_renderer_particle_draw_t
	{
		vec2f_t uv_start	   = vec2f_t::zero;
		vec2f_t uv_size		   = vec2f_t::zero;
		u32		particle_start = 0;
		u32		particle_count = 0;
		f32		aspect		   = 1.0f;
		u8		alignment	   = 0;
	};

	struct stock_world_renderer_draw_particle_instance_gpu_t
	{
		vec3f_t position = vec3f_t::zero;
		f32		rotation = 0.0f;
		vec3f_t velocity = vec3f_t::zero;
		f32		size	 = 0.0f;
		vec4f_t color	 = vec4f_t::zero;
	};

	struct stock_world_renderer_mesh_draw_t
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
