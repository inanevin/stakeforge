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
