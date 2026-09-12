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

#include "editor_project_cook_options.hpp"
#include "assets/editor_asset_type.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	editor_project_cook_options_reflection_t::editor_project_cook_options_reflection_t()
	{
		reflection_registry_t::get().register_type({
			.name		  = "editor_project_cook_options_t",
			.display_name = "Project Cook Options",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops_with_default<resource_handle_t, NULL_RESOURCE_HANDLE>(reflected_value_type_e::u64, SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_WORLD),
					 .name			= "worlds",
					 .display_name	= "Worlds",
					 .offset		= offsetof(editor_project_cook_options_t, worlds),
					 .size			= sizeof(vector_t<resource_handle_t>),
					 .type			= reflected_value_type_e::container},
					{.name		   = "main_world",
					 .display_name = "Main World",
					 .sub_type_id  = SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_WORLD,
					 .offset	   = offsetof(editor_project_cook_options_t, main_world),
					 .size		   = sizeof(resource_handle_t),
					 .type		   = reflected_value_type_e::u64},
					{.container_ops = reflection_container_ops_t::vector_ops_with_default<resource_handle_t, NULL_RESOURCE_HANDLE>(reflected_value_type_e::u64, SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_ANY_RESOURCE),
					 .name			= "extra_resources",
					 .display_name	= "Extra Resources",
					 .offset		= offsetof(editor_project_cook_options_t, extra_resources),
					 .size			= sizeof(vector_t<resource_handle_t>),
					 .type			= reflected_value_type_e::container},
					{.name = "is_borderless", .display_name = "Borderless", .offset = offsetof(editor_project_cook_options_t, is_borderless), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "is_fullscreen", .display_name = "Fullscreen", .offset = offsetof(editor_project_cook_options_t, is_fullscreen), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.ui_definition = {.dependency_field = "is_fullscreen"_hs, .dependency_value = 0, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "resolution",
					 .display_name	= "Resolution",
					 .sub_type_id	= type_id_t<vec2u16_t>::value,
					 .offset		= offsetof(editor_project_cook_options_t, resolution),
					 .size			= sizeof(vec2u16_t),
					 .type			= reflected_value_type_e::object},
				},
			.type_id   = type_id_t<editor_project_cook_options_t>::value,
			.size	   = sizeof(editor_project_cook_options_t),
			.alignment = alignof(editor_project_cook_options_t),
		});
	}
}
