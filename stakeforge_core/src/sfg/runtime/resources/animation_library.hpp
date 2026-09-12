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

namespace sfg
{
	enum class animation_library_blend_type_e : u8;

	struct animation_library_clip_runtime_t
	{
		resource_handle_t animation_clip = NULL_RESOURCE_HANDLE;
		vec2f_t			  weight_value	 = vec2f_t::zero;
		chunk_handle32_t  state			 = {};
	};

	struct animation_library_state_runtime_t
	{
		sid_t						   name_hash   = NULL_SID;
		vec2f_t						   blend_value = vec2f_t::zero;
		chunk_handle32_t			   layer	   = {};
		chunk_handle32_t			   clips	   = {};
		u32							   clip_count  = 0;
		animation_library_blend_type_e blend_type  = {};
	};

	struct animation_library_layer_runtime_t
	{
		sid_t			 name_hash			  = NULL_SID;
		sid_t			 mask_name_hash		  = NULL_SID;
		chunk_handle32_t states				  = {};
		u32				 state_count		  = 0;
		u32				 default_active_state = UINT32_MAX;
		f32				 weight				  = 1.0f;
		bool			 use_mask			  = false;
	};

	struct animation_library_runtime_t
	{
		resource_handle_t skeleton	  = NULL_RESOURCE_HANDLE;
		chunk_handle32_t  layers	  = {};
		chunk_handle32_t  states	  = {};
		chunk_handle32_t  clips		  = {};
		u32				  layer_count = 0;
		u32				  state_count = 0;
		u32				  clip_count  = 0;
	};

	struct animation_library_internals_t
	{
		u32 reserved = 0;
	};

	class animation_library_loader_t final
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('A', 'L', 'I', 'B');
		static constexpr u32 WIRE_VERSION = 3;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t animation_library_resource_desc;
}
