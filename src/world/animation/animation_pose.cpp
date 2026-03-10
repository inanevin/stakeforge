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

#include "game/game_max_defines.hpp"
#include "animation_pose.hpp"
#include "world/world.hpp"
#include "resources/animation.hpp"
#include "world/animation/animation_mask.hpp"
#include "math/math.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void animation_pose::sample_from_animation(world& w, resource_handle anim_handle, f32 time, const animation_mask* mask)
	{
		_joint_count = 0;

		const animation&   anim = w.get_resource_manager().get_resource<animation>(anim_handle);
		chunk_allocator32& aux	= w.get_resource_manager().get_aux();

		const chunk_handle32 positions		 = anim.get_position_channels();
		const chunk_handle32 rotations		 = anim.get_rotation_channels();
		const chunk_handle32 scales			 = anim.get_scale_channels();
		const u16			 positions_count = anim.get_position_channels_count();
		const u16			 rotations_count = anim.get_rotation_channels_count();
		const u16			 scales_count	 = anim.get_scale_channels_count();

		const animation_channel_v3* positions_ptr = positions_count == 0 ? nullptr : aux.get<animation_channel_v3>(positions);
		const animation_channel_q*	rotations_ptr = rotations_count == 0 ? nullptr : aux.get<animation_channel_q>(rotations);
		const animation_channel_v3* scales_ptr	  = scales_count == 0 ? nullptr : aux.get<animation_channel_v3>(scales);

		for (u16 i = 0; i < positions_count; i++)
		{
			const animation_channel_v3& ch		   = positions_ptr[i];
			const i16					node_index = ch.node_index;

			if (mask && mask->is_masked(node_index))
				continue;

			_joint_count = static_cast<u16>(math::max(static_cast<i16>(_joint_count), node_index));
			auto& jp	 = _joint_poses[node_index];
			jp.pos		 = ch.sample(time, aux);
			jp.flags.set(joint_pose_flags::has_position);
		}

		for (u16 i = 0; i < rotations_count; i++)
		{
			const animation_channel_q& ch		  = rotations_ptr[i];
			const i16				   node_index = ch.node_index;

			if (mask && mask->is_masked(node_index))
				continue;

			_joint_count = static_cast<u16>(math::max(static_cast<i16>(_joint_count), node_index));
			auto& jp	 = _joint_poses[node_index];
			jp.rot		 = ch.sample(time, aux);
			jp.flags.set(joint_pose_flags::has_rotation);
		}

		for (u16 i = 0; i < scales_count; i++)
		{
			const animation_channel_v3& ch		   = scales_ptr[i];
			const i16					node_index = ch.node_index;

			if (mask && mask->is_masked(node_index))
				continue;

			_joint_count = static_cast<u16>(math::max(static_cast<i16>(_joint_count), node_index));
			auto& jp	 = _joint_poses[node_index];
			jp.scale	 = ch.sample(time, aux);
			jp.flags.set(joint_pose_flags::has_scale);
		}
	}

	void animation_pose::blend_from(animation_pose& other, f32 other_ratio)
	{

		for (u16 i = 0; i < _joint_count; i++)
		{
			joint_pose&		   p	 = _joint_poses[i];
			const bitmask<u8>& flags = p.flags;
			if (flags == 0)
				continue;

			const joint_pose& other_pose = other._joint_poses[i];

			const bool has_pos		   = flags.is_set(joint_pose_flags::has_position);
			const bool has_rot		   = flags.is_set(joint_pose_flags::has_rotation);
			const bool has_scale	   = flags.is_set(joint_pose_flags::has_scale);
			const bool other_has_pos   = other_pose.flags.is_set(joint_pose_flags::has_position);
			const bool other_has_rot   = other_pose.flags.is_set(joint_pose_flags::has_rotation);
			const bool other_has_scale = other_pose.flags.is_set(joint_pose_flags::has_scale);

			if (has_pos && other_has_pos)
				p.pos = vector3::lerp(p.pos, other_pose.pos, other_ratio);
			else if (!has_pos && other_has_pos)
				p.pos = other_pose.pos;

			if (has_rot && other_has_rot)
				p.rot = quat::slerp(p.rot, other_pose.rot, other_ratio);
			else if (!has_rot && other_has_rot)
				p.rot = other_pose.rot;

			if (has_scale && other_has_scale)
				p.scale = vector3::lerp(p.scale, other_pose.scale, other_ratio);
			else if (!has_scale && other_has_scale)
				p.scale = other_pose.scale;
		}
	}

}