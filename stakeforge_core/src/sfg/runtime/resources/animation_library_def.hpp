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

#include "resource_handle.hpp"
#include <sfg/common/type_id.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
#define MAX_ANIMATION_LIBRARY_STATE_CLIPS 8

	enum class animation_library_blend_type_e : u8
	{
		no_blend,
		blend_1d,
		blend_2d,
	};

	struct animation_library_clip_def_t
	{
		resource_handle_t animation_clip = NULL_RESOURCE_HANDLE;
		vec2f_t			  weight_value	 = vec2f_t::zero;
	};

	struct animation_library_state_def_t
	{
		char						   name[256]								= {};
		animation_library_clip_def_t   clips[MAX_ANIMATION_LIBRARY_STATE_CLIPS] = {};
		vec2f_t						   blend_value								= vec2f_t::zero;
		size_t						   clip_count								= 0;
		animation_library_blend_type_e blend_type								= animation_library_blend_type_e::no_blend;
	};

	struct animation_library_layer_def_t
	{
		char									name[256]	   = {};
		char									mask_name[256] = {};
		vector_t<animation_library_state_def_t> states;
		u32										default_active_state = UINT32_MAX;
		float									weight				 = 1.0f;
		bool									use_mask			 = false;
	};

	struct animation_library_def_t
	{
		resource_handle_t						skeleton = NULL_RESOURCE_HANDLE;
		vector_t<animation_library_layer_def_t> layers	 = {};
	};

	SFG_DEFINE_TYPE_ID(animation_library_blend_type_e);
	SFG_DEFINE_TYPE_ID(animation_library_clip_def_t);
	SFG_DEFINE_TYPE_ID(animation_library_state_def_t);
	SFG_DEFINE_TYPE_ID(animation_library_layer_def_t);
	SFG_DEFINE_TYPE_ID(animation_library_def_t);

	struct animation_library_def_reflection_t
	{
		animation_library_def_reflection_t();
	};

	inline animation_library_def_reflection_t g_reflect_animation_library_def;
}
