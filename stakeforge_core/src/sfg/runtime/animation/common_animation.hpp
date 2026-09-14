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

#include <sfg/math/vec3f.hpp>
#include <sfg/math/quat.hpp>

namespace sfg
{
#define MAX_ANIMATION_LIBRARY_LAYERS	  6
#define MAX_ANIMATION_LIBRARY_STATE_CLIPS 8

	enum class animation_library_blend_type_e : u8
	{
		no_blend,
		blend_1d,
		blend_2d,
	};

	struct decomposed_bone_t
	{
		vec3f_t position = vec3f_t::zero;
		quat_t	rotation = quat_t::identity;
		vec3f_t scale	 = vec3f_t::one;
	};
}
