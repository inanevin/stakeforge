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

#include "core/common/sfg_exports.hpp"

#include "core/runtime/resources/animation_common.hpp"

#include "core/common/type_id.hpp"

#include "core/common/size_definitions.hpp"
#include "core/data/string.hpp"
#include "core/data/vector.hpp"
#include "core/runtime/animation/common_animation.hpp"
#include "core/runtime/resources/resource_handle.hpp"

namespace sfg
{
	struct animation_channel_v3_def_t
	{
		vector_t<animation_keyframe_v3_t>		 keyframes		  = {};
		vector_t<animation_keyframe_v3_spline_t> keyframes_spline = {};
		animation_interpolation_e				 interpolation	  = animation_interpolation_e::linear;
		i32										 node_index		  = -1;
	};

	struct animation_channel_q_def_t
	{
		vector_t<animation_keyframe_q_t>		keyframes		 = {};
		vector_t<animation_keyframe_q_spline_t> keyframes_spline = {};
		animation_interpolation_e				interpolation	 = animation_interpolation_e::linear;
		i32										node_index		 = -1;
	};

	struct SFG_API animation_event_def_t
	{
		char name[256] = {};
		f32	 time	   = 0.0f;
	};

	struct SFG_API animation_def_t
	{
		vector_t<animation_channel_v3_def_t> position_channels		 = {};
		vector_t<animation_channel_q_def_t>	 rotation_channels		 = {};
		vector_t<animation_channel_v3_def_t> scale_channels			 = {};
		animation_event_def_t				 events[MAX_CLIP_EVENTS] = {};
		size_t								 event_count			 = 0;
		string_t							 name					 = {};
		resource_handle_t					 preview_mesh			 = NULL_RESOURCE_HANDLE;
		resource_handle_t					 preview_skeleton		 = NULL_RESOURCE_HANDLE;
		sid_t								 name_hash				 = NULL_SID;
		f32									 duration				 = 0.0f;
	};

	SFG_DEFINE_TYPE_ID(animation_channel_v3_def_t);
	SFG_DEFINE_TYPE_ID(animation_channel_q_def_t);
	SFG_DEFINE_TYPE_ID(animation_def_t);
	SFG_DEFINE_TYPE_ID(animation_event_def_t);

	struct animation_channel_v3_def_reflection_t
	{
		animation_channel_v3_def_reflection_t();
	};

	struct animation_channel_q_def_reflection_t
	{
		animation_channel_q_def_reflection_t();
	};

	struct animation_event_def_reflection_t
	{
		animation_event_def_reflection_t();
	};

	struct animation_def_reflection_t
	{
		animation_def_reflection_t();
	};

}
