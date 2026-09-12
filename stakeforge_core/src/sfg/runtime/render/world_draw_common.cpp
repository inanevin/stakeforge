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

#include "world_draw_common.hpp"
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
}

namespace sfg
{
	world_pass_flags_reflection_t::world_pass_flags_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "world_pass_flags_e",
			.fields =
				{
					{.name = "none", .display_name = "None"},
					{.name = "gbuffer", .display_name = "GBuffer"},
					{.name = "forward", .display_name = "Forward"},
					{.name = "depth", .display_name = "Depth"},
					{.name = "shadow", .display_name = "Shadow"},
					{.name = "id", .display_name = "Id"},
					{.name = "reflections", .display_name = "Reflections"},
				},
			.type_id   = type_id_t<world_pass_flags_e>::value,
			.size	   = sizeof(world_pass_flags_e),
			.alignment = alignof(world_pass_flags_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

}
