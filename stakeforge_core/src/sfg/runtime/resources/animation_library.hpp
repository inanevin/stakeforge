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

#include "common_resources.hpp"
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/animation/common_animation.hpp>

namespace sfg
{
	struct animation_library_clip_runtime_t
	{
		resource_handle_t animation_clip = NULL_RESOURCE_HANDLE;
		vec2f_t			  weight_value	 = vec2f_t::zero;
		chunk_handle32_t  state			 = {};
		f32				  start_time	 = 0.0f;
		f32				  playback_speed = 1.0f;
	};

	struct animation_library_state_delaunay_triangle_t
	{
		vec2f_t v0			= vec2f_t::zero;
		vec2f_t coeff1		= vec2f_t::zero;
		vec2f_t coeff2		= vec2f_t::zero;
		u32		clip_index0 = 0;
		u32		clip_index1 = 0;
		u32		clip_index2 = 0;
	};

	struct animation_library_state_runtime_t
	{
		animation_library_clip_runtime_t clips[MAX_ANIMATION_LIBRARY_STATE_CLIPS] = {};
		sid_t							 name_hash								  = NULL_SID;
		vec2f_t							 initial_blend_value					  = vec2f_t::zero;
		chunk_handle32_t				 layer									  = {};
		chunk_handle32_t				 delaunay_triangles						  = {};
		u32								 triangle_count							  = 0;
		u32								 clip_count								  = 0;
		f32								 speed									  = 1.0f;
		animation_library_blend_type_e	 blend_type								  = {};
		bool							 loop									  = true;
	};

	struct animation_library_layer_runtime_t
	{
		sid_t			 name_hash			  = NULL_SID;
		sid_t			 mask				  = NULL_SID;
		chunk_handle32_t states				  = {};
		u32				 state_count		  = 0;
		u32				 default_active_state = UINT32_MAX;
		f32				 weight				  = 1.0f;
	};

	struct animation_library_runtime_t
	{
		animation_library_layer_runtime_t layers[MAX_ANIMATION_LIBRARY_LAYERS] = {};
		resource_handle_t				  skeleton							   = NULL_RESOURCE_HANDLE;
		u32								  layer_count						   = 0;
	};

	struct animation_library_internals_t
	{
		u32 reserved = 0;
	};

	class animation_library_loader_t final
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('A', 'L', 'I', 'B');
		static constexpr u32 WIRE_VERSION = 8;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t animation_library_resource_desc;
}
