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

#include "core/gfx/common/gfx_constants.hpp"
#include "core/gfx/common/format.hpp"
#include "core/math/vec2u16.hpp"

namespace sfg
{
	struct world_render_snapshot_t;

	/// <summary>
	/// if you transition the texture, you need to restore to the given u32 states before returning.
	/// </summary>
	struct game_plugin_render_texture_t
	{
		gfx_handle_t texture			   = {};
		gpu_index_t	 shader_resource_index = NULL_GPU_INDEX;
		u32			 states				   = 0;
		u8			 attachment_view_index = 0;
		format_e	 format				   = format_e::undefined;
	};

	/// <summary>
	/// do not submit or close the command_buffer.
	/// lighting_data_index is only available in before_forward and before_post_process
	/// </summary>
	struct game_plugin_render_context_t
	{
		const void*					   plugin_data		   = nullptr;
		const world_render_snapshot_t* snapshot			   = nullptr;
		gfx_handle_t				   command_buffer	   = {};
		gfx_handle_t				   bind_layout		   = {};
		gpu_index_t					   global_data_index   = NULL_GPU_INDEX;
		gpu_index_t					   view_data_index	   = NULL_GPU_INDEX;
		gpu_index_t					   entity_buffer_index = NULL_GPU_INDEX;
		gpu_index_t					   bone_buffer_index   = NULL_GPU_INDEX;
		gpu_index_t					   lighting_data_index = NULL_GPU_INDEX;
		gpu_index_t					   fog_data_index	   = NULL_GPU_INDEX;
		f32							   interpolation_alpha = 0.0f;
		vec2u16_t					   render_size		   = vec2u16_t::zero;
		u8							   frame_index		   = 0;
	};

	struct game_plugin_gbuffer_pass_t
	{
		game_plugin_render_texture_t albedo	  = {};
		game_plugin_render_texture_t normal	  = {};
		game_plugin_render_texture_t orm	  = {};
		game_plugin_render_texture_t emissive = {};
		game_plugin_render_texture_t depth	  = {};
	};

	/// <summary>
	/// color is writable.
	/// scene is read only always.
	/// bloom idx is available before tonemapping & after post-processing, check for nullity.
	/// </summary>
	struct game_plugin_color_pass_t
	{
		game_plugin_render_texture_t color		 = {};
		game_plugin_gbuffer_pass_t	 scene		 = {};
		gpu_index_t					 bloom_index = NULL_GPU_INDEX;
	};
}
