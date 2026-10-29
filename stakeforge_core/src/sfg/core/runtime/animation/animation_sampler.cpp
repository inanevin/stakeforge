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

#include "core/runtime/animation/animation_sampler.hpp"
#include "core/runtime/animation/common_animation.hpp"

#include "core/io/assert.hpp"
#include "core/runtime/resources/animation.hpp"

namespace sfg
{
	void animation_sampler_t::sample_animation(const animation_sample_params_t& params)
	{
		SFG_ASSERT(params.animation != nullptr);
		SFG_ASSERT(params.bones != nullptr);

		for (u32 channel_index = 0; channel_index < params.animation->position_count; ++channel_index)
		{
			const animation_channel_v3_runtime_t& channel	 = params.animation->position_channels[channel_index];
			const u32							  node_index = static_cast<u32>(channel.node_index);

			if (is_masked(node_index, params.mask.masks))
				continue;

			params.bones[node_index].position = sample_channel(channel, params.sample_time);
			params.out_position_mask.masks[node_index / 64] |= 1llu << (node_index % 64);
		}

		for (u32 channel_index = 0; channel_index < params.animation->rotation_count; ++channel_index)
		{
			const animation_channel_q_runtime_t& channel	= params.animation->rotation_channels[channel_index];
			const u32							 node_index = static_cast<u32>(channel.node_index);

			if (is_masked(node_index, params.mask.masks))
				continue;

			params.bones[node_index].rotation = sample_channel(channel, params.sample_time);
			params.out_rotation_mask.masks[node_index / 64] |= 1llu << (node_index % 64);
		}

		for (u32 channel_index = 0; channel_index < params.animation->scale_count; ++channel_index)
		{
			const animation_channel_v3_runtime_t& channel	 = params.animation->scale_channels[channel_index];
			const u32							  node_index = static_cast<u32>(channel.node_index);

			if (is_masked(node_index, params.mask.masks))
				continue;

			params.bones[node_index].scale = sample_channel(channel, params.sample_time);
			params.out_scale_mask.masks[node_index / 64] |= 1llu << (node_index % 64);
		}
	}

	vec3f_t animation_sampler_t::sample_channel(const animation_channel_v3_runtime_t& channel, f32 sample_time)
	{
		if (channel.interpolation == animation_interpolation_e::cubic_spline)
		{
			if (channel.spline_count == 0)
				return vec3f_t::zero;

			const animation_keyframe_v3_spline_t& front = channel.keyframes_spline[0];
			const animation_keyframe_v3_spline_t& back	= channel.keyframes_spline[channel.spline_count - 1];

			if (sample_time <= front.time)
				return front.value;

			if (sample_time >= back.time)
				return back.value;

			size_t keyframe_index = 0;

			while (keyframe_index < channel.spline_count - 1 && sample_time > channel.keyframes_spline[keyframe_index + 1].time)
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

		if (channel.keyframe_count == 0)
			return vec3f_t::zero;

		const animation_keyframe_v3_t& front = channel.keyframes[0];
		const animation_keyframe_v3_t& back	 = channel.keyframes[channel.keyframe_count - 1];

		if (sample_time <= front.time)
			return front.value;

		if (sample_time >= back.time)
			return back.value;

		size_t keyframe_index = 0;

		while (keyframe_index < channel.keyframe_count - 1 && sample_time > channel.keyframes[keyframe_index + 1].time)
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

	quat_t animation_sampler_t::sample_channel(const animation_channel_q_runtime_t& channel, f32 sample_time)
	{
		if (channel.interpolation == animation_interpolation_e::cubic_spline)
		{
			if (channel.spline_count == 0)
				return quat_t::identity;

			const animation_keyframe_q_spline_t& front = channel.keyframes_spline[0];
			const animation_keyframe_q_spline_t& back  = channel.keyframes_spline[channel.spline_count - 1];

			if (sample_time <= front.time)
				return front.value;

			if (sample_time >= back.time)
				return back.value;

			size_t keyframe_index = 0;

			while (keyframe_index < channel.spline_count - 1 && sample_time > channel.keyframes_spline[keyframe_index + 1].time)
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

		if (channel.keyframe_count == 0)
			return quat_t::identity;

		const animation_keyframe_q_t& front = channel.keyframes[0];
		const animation_keyframe_q_t& back	= channel.keyframes[channel.keyframe_count - 1];

		if (sample_time <= front.time)
			return front.value;

		if (sample_time >= back.time)
			return back.value;

		size_t keyframe_index = 0;

		while (keyframe_index < channel.keyframe_count - 1 && sample_time > channel.keyframes[keyframe_index + 1].time)
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
