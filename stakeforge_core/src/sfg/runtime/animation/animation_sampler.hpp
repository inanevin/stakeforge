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

#include "animation_graph_types.hpp"

#include <sfg/data/span.hpp>

namespace sfg
{
	struct animation_runtime_t;
	struct animation_channel_v3_runtime_t;
	struct animation_channel_q_runtime_t;
	struct skeleton_mask_t;
	struct vec3f_t;
	struct decomposed_bone_t;
	class quat_t;

	class animation_sampler_t final
	{
	public:
		static void sample_animation(const animation_runtime_t* animation, f32 sample_time, const u64* bitmasks, span_t<animation_graph_bone_t> pose_bones);
		static void sample_animation(const animation_runtime_t* animation, f32 sample_time, const skeleton_mask_t& mask, skeleton_mask_t& out_written_bones, decomposed_bone_t* bones, f32 weight);

	private:
		static vec3f_t sample_channel(const animation_channel_v3_runtime_t& channel, f32 sample_time);
		static quat_t  sample_channel(const animation_channel_q_runtime_t& channel, f32 sample_time);
		static bool	   is_masked(u32 node_index, const u64* bitmasks);
	};
}
