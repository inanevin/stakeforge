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

#include "physical_material_def.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{

}

namespace sfg
{
	physical_material_reflection_t::physical_material_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "physical_material_def_t",
			.fields =
				{
					{.name = "restitution", .display_name = "Restitution", .offset = offsetof(physical_material_def_t, restitution), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "friction", .display_name = "Friction", .offset = offsetof(physical_material_def_t, friction), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "angular_damping", .display_name = "Angular Damping", .offset = offsetof(physical_material_def_t, angular_damping), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "linear_damping", .display_name = "Linear Damping", .offset = offsetof(physical_material_def_t, linear_damping), .size = sizeof(f32), .type = reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<physical_material_def_t>::value,
			.size	   = sizeof(physical_material_def_t),
			.alignment = alignof(physical_material_def_t),
		});
	}
}
