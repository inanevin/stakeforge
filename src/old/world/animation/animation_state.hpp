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

#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "data/bitmask.hpp"
#include "math/vector2.hpp"

namespace SFG
{
	enum animation_state_flags : u8
	{
		animation_state_flags_is_looping = 1 << 0,
		animation_state_flags_is_1d		 = 1 << 1,
		animation_state_flags_is_2d		 = 1 << 2,
	};

	struct animation_state_sample
	{
		vector2			blend_point	 = vector2::zero;
		resource_handle animation	 = {};
		pool_handle16	_next_sample = {};
	};

	struct animation_state
	{
		string_id	  sid					= 0;
		pool_handle16 _first_sample			= {};
		pool_handle16 _first_out_transition = {};
		pool_handle16 _next_state			= {};
		pool_handle16 mask					= {};
		pool_handle16 blend_weight_param_x	= {};
		pool_handle16 blend_weight_param_y	= {};
		f32			  duration				= 0.0f;
		f32			  speed					= 1.0f;
		f32			  _current_time			= 0.0f;
		bitmask<u8>	  flags					= 0;
	};
}
