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
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	enum class world_render_light_type_e : u32
	{
		directional,
		point,
		spot,
		area,
		count,
	};

	struct world_render_light_t
	{
		quat_t	prev_rot			 = {};
		quat_t	rot					 = {};
		vec3f_t prev_pos			 = vec3f_t::zero;
		f32		intensity			 = 1.0f;
		vec3f_t pos					 = vec3f_t::zero;
		f32		range				 = 10.0f;
		vec3f_t color				 = vec3f_t::one;
		f32		inner_cone_degrees	 = 30.0f;
		f32		outer_cone_degrees	 = 45.0f;
		f32		area_width			 = 1.0f;
		f32		area_height			 = 1.0f;
		f32		shadow_near_plane	 = 0.1f;
		f32		shadow_bias			 = 0.001f;
		f32		shadow_normal_bias	 = 0.01f;
		u32		stable_id			 = UINT32_MAX;
		u16		shadow_resolution	 = 1024;
		u8		type				 = static_cast<u8>(world_render_light_type_e::point);
		u8		flags				 = 0;
		u8		shadow_cascade_count = 4;
		u8		cast_shadows		 = 0;
	};
}
