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
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "data/bitmask.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	class world;
	class animation;
	class skin;
	class animation_mask;

	enum joint_pose_flags : u8
	{
		has_position = 1 << 0,
		has_rotation = 1 << 1,
		has_scale	 = 1 << 2,
	};

	struct joint_pose
	{
		vector3		pos	  = vector3::zero;
		quat		rot	  = quat::identity;
		vector3		scale = vector3::zero;
		bitmask<u8> flags = 0;
	};

	class animation_pose
	{
	public:
		void sample_from_animation(world& w, resource_handle anim, f32 time, const animation_mask* mask);
		void blend_from(animation_pose& other, f32 other_ratio);

		inline void reset()
		{
			_joint_count = 0;
			for (u16 i = 0; i < MAX_WORLD_SKELETON_JOINTS; i++)
				_joint_poses[i].flags = 0;
		}

		inline const joint_pose* get_joint_poses() const
		{
			return _joint_poses;
		}

		inline u16 get_joint_count() const
		{
			return _joint_count;
		}

	private:
		joint_pose _joint_poses[MAX_WORLD_SKELETON_JOINTS];
		u16		   _joint_count = 0;
	};

}
