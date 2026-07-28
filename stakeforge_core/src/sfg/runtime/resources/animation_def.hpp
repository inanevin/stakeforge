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

#include "animation_common.hpp"

#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

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

	struct animation_def_t
	{
		vector_t<animation_channel_v3_def_t>	position_channels = {};
		vector_t<animation_channel_q_def_t>		rotation_channels = {};
		vector_t<animation_channel_v3_def_t>	scale_channels	  = {};
		string_t								name			  = {};
		inplace_vector_t<resource_handle_t, 16> preview_materials = {};
		resource_handle_t						preview_mesh	  = NULL_RESOURCE_HANDLE;
		resource_handle_t						preview_skeleton  = NULL_RESOURCE_HANDLE;
		sid_t									name_hash		  = NULL_SID;
		f32										duration		  = 0.0f;
	};

	SFG_DEFINE_TYPE_ID(animation_channel_v3_def_t);
	SFG_DEFINE_TYPE_ID(animation_channel_q_def_t);
	SFG_DEFINE_TYPE_ID(animation_def_t);

	struct animation_channel_v3_def_reflection_t
	{
		animation_channel_v3_def_reflection_t();
	};

	struct animation_channel_q_def_reflection_t
	{
		animation_channel_q_def_reflection_t();
	};

	struct animation_def_reflection_t
	{
		animation_def_reflection_t();
	};

	inline animation_channel_v3_def_reflection_t g_reflect_animation_channel_v3_def;
	inline animation_channel_q_def_reflection_t	 g_reflect_animation_channel_q_def;
	inline animation_def_reflection_t			 g_reflect_animation_def;
}
