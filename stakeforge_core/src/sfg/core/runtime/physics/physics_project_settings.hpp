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

#include "core/common/sfg_exports.hpp"

#include "core/common/type_id.hpp"
#include "core/data/string.hpp"
#include "core/data/vector.hpp"
#include "core/runtime/physics/physics_config.hpp"

namespace sfg
{
	struct physics_collision_layer_definition_t
	{
		string_t name		   = {};
		u64		 collides_with = 0;
		u64		 identifier	   = 0;
		u8		 slot		   = 0;

		bool operator==(const physics_collision_layer_definition_t&) const = default;
	};

	struct SFG_API physics_project_settings_t
	{
		physics_project_settings_t();

		vector_t<physics_collision_layer_definition_t> collision_layers							  = {};
		u64											   next_collision_layer_identifier			  = 2;
		bool										   kinematic_sensors_collide_with_non_dynamic = false;

		void				   normalize(const physics_project_settings_t* previous = nullptr);
		world_physics_config_t make_runtime_config(u32 physics_rate, u32 max_sub_steps) const;

		bool operator==(const physics_project_settings_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(physics_collision_layer_definition_t);
	SFG_DEFINE_TYPE_ID(physics_project_settings_t);

	struct physics_project_settings_reflection_t
	{
		physics_project_settings_reflection_t();
	};

}
