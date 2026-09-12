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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	enum class world_render_reflection_probe_capture_type_e : u8
	{
		skybox,
		scene,
	};

	enum class world_render_reflection_probe_capture_mode_e : u8
	{
		manual,
		realtime,
	};

	struct world_render_reflection_probe_t
	{
		quat_t										 prev_rot				= quat_t::identity;
		quat_t										 rot					= quat_t::identity;
		vec3f_t										 prev_pos				= vec3f_t::zero;
		f32											 diffuse_intensity		= 1.0f;
		vec3f_t										 pos					= vec3f_t::zero;
		f32											 specular_intensity		= 1.0f;
		vec3f_t										 prev_scale				= vec3f_t::one;
		f32											 blend_distance			= 1.0f;
		vec3f_t										 scale					= vec3f_t::one;
		f32											 resolution				= 256.0f;
		vec3f_t										 extents				= {5.0f, 5.0f, 5.0f};
		f32											 max_distance			= 0.0f;
		f32											 near_plane				= 0.1f;
		u32											 stable_id				= UINT32_MAX;
		u32											 generation				= 0;
		u32											 realtime_tick_interval = 30;
		world_render_reflection_probe_capture_type_e capture_type			= world_render_reflection_probe_capture_type_e::scene;
		world_render_reflection_probe_capture_mode_e capture_mode			= world_render_reflection_probe_capture_mode_e::manual;
		u8											 is_global				= 0;
		u8											 disabled				= 0;
	};
}
