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
#include "common/string_id.hpp"

namespace SFG
{
	class world;

	class animation_mask
	{
	public:
		struct joint_data
		{
			u8 is_masked = 0;
		};

		void mask_joints(world& w, resource_handle skin, string_id* name_hashes, u16 name_hashes_count);
		void mask_joint(world& w, resource_handle skin, string_id hash);

		inline const joint_data* get_mask() const
		{
			return joints;
		}

		inline void mask_joint(i16 j)
		{
			joints[j].is_masked = 1;
		}

		inline bool is_masked(i16 j) const
		{
			return joints[j].is_masked;
		}

		inline void reset()
		{
			for (u16 i = 0; i < MAX_WORLD_SKELETON_JOINTS; i++)
				joints[i].is_masked = 0;
		}

		string_id  sid = 0;
		joint_data joints[MAX_WORLD_SKELETON_JOINTS];
	};

}
