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
#include "data/bitmask.hpp"
#include "common/string_id.hpp"
#include "data/static_vector.hpp"
#include "world/world_constants.hpp"

#undef max

namespace SFG
{
	enum entity_flags : u8
	{
		entity_flags_invisible					  = 1 << 0,
		entity_flags_transient_abs_transform_mark = 1 << 1,
		entity_flags_is_render_proxy			  = 1 << 2,
		entity_flags_template					  = 1 << 3,
		entity_flags_no_save					  = 1 << 4,
	};

	struct entity_meta
	{
		const char* name			   = "";
		const char* tag				   = "";
		u8			render_proxy_count = 0;
	};

	struct entity_family
	{
		world_handle parent		  = {};
		world_handle first_child  = {};
		world_handle prev_sibling = {};
		world_handle next_sibling = {};
	};

	struct entity_comp
	{
		string_id	 comp_type	 = 0;
		world_handle comp_handle = {};

		bool operator==(const entity_comp& other) const
		{
			return comp_type == other.comp_type && comp_handle == other.comp_handle;
		}
	};

#define MAX_COMPS_PER_ENTITY 8

	struct entity_comp_register
	{
		static_vector<entity_comp, MAX_COMPS_PER_ENTITY> comps;
	};
}
