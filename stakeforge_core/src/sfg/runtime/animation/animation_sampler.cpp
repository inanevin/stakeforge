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

#include "animation_sampler.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/resources/animation.hpp>

namespace sfg
{
	namespace
	{
		struct sampled_bone_t
		{
			vec3f_t position = vec3f_t::zero;
			quat_t	rotation = quat_t::identity;
			vec3f_t scale	 = vec3f_t::one;
			bool	active	 = false;
		};
	}

	void animation_sampler_t::sample_animation(const animation_runtime_t* animation, f32 sample_time, const u64* bitmasks, span_t<animation_graph_bone_t> pose_bones)
	{
		SFG_ASSERT(animation != nullptr);
		SFG_ASSERT(pose_bones.size <= MAX_SKELETON_BONES);

		sampled_bone_t sampled_bones[MAX_SKELETON_BONES] = {};

		for (size_t bone_index = 0; bone_index < pose_bones.size; ++bone_index)
			pose_bones.data[bone_index].active = 0;

		for (const animation_channel_v3_def_t& channel : animation->def.position_channels)
		{
			const u32 node_index = static_cast<u32>(channel.node_index);
			SFG_ASSERT(node_index < pose_bones.size);

			if (is_masked(node_index, bitmasks))
				continue;

			sampled_bones[node_index].position = sample_channel(channel, sample_time);
			sampled_bones[node_index].active   = true;
		}

		for (const animation_channel_q_def_t& channel : animation->def.rotation_channels)
		{
			const u32 node_index = static_cast<u32>(channel.node_index);
			SFG_ASSERT(node_index < pose_bones.size);

			if (is_masked(node_index, bitmasks))
				continue;

			sampled_bones[node_index].rotation = sample_channel(channel, sample_time);
			sampled_bones[node_index].active   = true;
		}

		for (const animation_channel_v3_def_t& channel : animation->def.scale_channels)
		{
			const u32 node_index = static_cast<u32>(channel.node_index);
			SFG_ASSERT(node_index < pose_bones.size);

			if (is_masked(node_index, bitmasks))
				continue;

			sampled_bones[node_index].scale	 = sample_channel(channel, sample_time);
			sampled_bones[node_index].active = true;
		}

		for (u32 node_index = 0; node_index < pose_bones.size; ++node_index)
		{
			const sampled_bone_t& sampled_bone = sampled_bones[node_index];

			if (!sampled_bone.active)
				continue;

			animation_graph_bone_t& pose_bone = pose_bones.data[node_index];
			pose_bone.local_matrix			  = mat4x3_t::transform(sampled_bone.position, sampled_bone.rotation, sampled_bone.scale);
			pose_bone.active				  = 1;
		}
	}

	vec3f_t animation_sampler_t::sample_channel(const animation_channel_v3_def_t& channel, f32 sample_time)
	{
		if (channel.interpolation == animation_interpolation_e::cubic_spline)
		{
			if (channel.keyframes_spline.empty())
				return vec3f_t::zero;

			const animation_keyframe_v3_spline_t& front = channel.keyframes_spline.front();
			const animation_keyframe_v3_spline_t& back	= channel.keyframes_spline.back();

			if (sample_time <= front.time)
				return front.value;

			if (sample_time >= back.time)
				return back.value;

			size_t keyframe_index = 0;

			while (keyframe_index < channel.keyframes_spline.size() - 1 && sample_time > channel.keyframes_spline[keyframe_index + 1].time)
				++keyframe_index;

			const animation_keyframe_v3_spline_t& keyframe0 = channel.keyframes_spline[keyframe_index];
			const animation_keyframe_v3_spline_t& keyframe1 = channel.keyframes_spline[keyframe_index + 1];
			const f32							  duration	= keyframe1.time - keyframe0.time;
			const f32							  time		= (sample_time - keyframe0.time) / duration;
			const f32							  time2		= time * time;
			const f32							  time3		= time2 * time;
			const f32							  h00		= 2.0f * time3 - 3.0f * time2 + 1.0f;
			const f32							  h10		= time3 - 2.0f * time2 + time;
			const f32							  h01		= -2.0f * time3 + 3.0f * time2;
			const f32							  h11		= time3 - time2;

			return h00 * keyframe0.value + h10 * keyframe0.out_tangent * duration + h01 * keyframe1.value + h11 * keyframe1.in_tangent * duration;
		}

		if (channel.keyframes.empty())
			return vec3f_t::zero;

		const animation_keyframe_v3_t& front = channel.keyframes.front();
		const animation_keyframe_v3_t& back	 = channel.keyframes.back();

		if (sample_time <= front.time)
			return front.value;

		if (sample_time >= back.time)
			return back.value;

		size_t keyframe_index = 0;

		while (keyframe_index < channel.keyframes.size() - 1 && sample_time > channel.keyframes[keyframe_index + 1].time)
			++keyframe_index;

		const animation_keyframe_v3_t& keyframe0  = channel.keyframes[keyframe_index];
		const animation_keyframe_v3_t& keyframe1  = channel.keyframes[keyframe_index + 1];
		const f32					   local_time = (sample_time - keyframe0.time) / (keyframe1.time - keyframe0.time);

		switch (channel.interpolation)
		{
		case animation_interpolation_e::linear:
			return vec3f_t::lerp(keyframe0.value, keyframe1.value, local_time);
		case animation_interpolation_e::step:
			return keyframe0.value;
		case animation_interpolation_e::cubic_spline:
			return vec3f_t::zero;
		}

		return vec3f_t::zero;
	}

	quat_t animation_sampler_t::sample_channel(const animation_channel_q_def_t& channel, f32 sample_time)
	{
		if (channel.interpolation == animation_interpolation_e::cubic_spline)
		{
			if (channel.keyframes_spline.empty())
				return quat_t::identity;

			const animation_keyframe_q_spline_t& front = channel.keyframes_spline.front();
			const animation_keyframe_q_spline_t& back  = channel.keyframes_spline.back();

			if (sample_time <= front.time)
				return front.value;

			if (sample_time >= back.time)
				return back.value;

			size_t keyframe_index = 0;

			while (keyframe_index < channel.keyframes_spline.size() - 1 && sample_time > channel.keyframes_spline[keyframe_index + 1].time)
				++keyframe_index;

			const animation_keyframe_q_spline_t& keyframe0 = channel.keyframes_spline[keyframe_index];
			const animation_keyframe_q_spline_t& keyframe1 = channel.keyframes_spline[keyframe_index + 1];
			const f32							 duration  = keyframe1.time - keyframe0.time;
			const f32							 time	   = (sample_time - keyframe0.time) / duration;
			const f32							 time2	   = time * time;
			const f32							 time3	   = time2 * time;
			const f32							 h00	   = 2.0f * time3 - 3.0f * time2 + 1.0f;
			const f32							 h10	   = time3 - 2.0f * time2 + time;
			const f32							 h01	   = -2.0f * time3 + 3.0f * time2;
			const f32							 h11	   = time3 - time2;

			return (h00 * keyframe0.value + h10 * keyframe0.out_tangent * duration + h01 * keyframe1.value + h11 * keyframe1.in_tangent * duration).normalized();
		}

		if (channel.keyframes.empty())
			return quat_t::identity;

		const animation_keyframe_q_t& front = channel.keyframes.front();
		const animation_keyframe_q_t& back	= channel.keyframes.back();

		if (sample_time <= front.time)
			return front.value;

		if (sample_time >= back.time)
			return back.value;

		size_t keyframe_index = 0;

		while (keyframe_index < channel.keyframes.size() - 1 && sample_time > channel.keyframes[keyframe_index + 1].time)
			++keyframe_index;

		const animation_keyframe_q_t& keyframe0	 = channel.keyframes[keyframe_index];
		const animation_keyframe_q_t& keyframe1	 = channel.keyframes[keyframe_index + 1];
		const f32					  local_time = (sample_time - keyframe0.time) / (keyframe1.time - keyframe0.time);

		switch (channel.interpolation)
		{
		case animation_interpolation_e::linear:
			return quat_t::slerp(keyframe0.value, keyframe1.value, local_time);
		case animation_interpolation_e::step:
			return keyframe0.value;
		case animation_interpolation_e::cubic_spline:
			return quat_t::identity;
		}

		return quat_t::identity;
	}

	bool animation_sampler_t::is_masked(u32 node_index, const u64* bitmasks)
	{
		SFG_ASSERT(node_index < MAX_SKELETON_BONES);

		const u32 mask_index = node_index / 64;
		const u32 bit_index	 = node_index % 64;

		return bitmasks != nullptr && ((bitmasks[mask_index] & (u64{1} << bit_index)) != 0);
	}
}
