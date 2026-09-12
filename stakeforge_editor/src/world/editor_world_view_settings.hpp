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
#include <sfg/common/type_id.hpp>

namespace sfg
{
	struct editor_world_view_settings_t
	{
		f32 physics_ray_force = 1000.0f;
		f32 grid_scale		  = 1.0f;
		f32 snap_translate	  = 0.0f;
		f32 snap_rotate		  = 0.0f;
		f32 snap_scale		  = 0.0f;
	};

	SFG_DEFINE_TYPE_ID(editor_world_view_settings_t);

	struct editor_world_view_settings_reflection_t
	{
		editor_world_view_settings_reflection_t();
	};

	inline editor_world_view_settings_reflection_t g_reflect_editor_world_view_settings;
}
