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

#include "game_type_registry.hpp"
#include "game_components.hpp"

#include <sfg/core/reflection/reflection_container_ops.hpp>
#include <sfg/core/runtime/resources/resource_type.hpp>
#include <sfg/core/runtime/scripting/native_component_schema.hpp>

namespace game
{
	void game_type_registry_t::register_types(sfg::native_component_schema_t& schema)
	{
		using namespace sfg;

		schema.register_type<example_mode_e>({
			.name		  = "game::example_mode_e",
			.display_name = "Mode",
			.fields =
				{
					{
						.name		  = "idle",
						.display_name = "Idle",
					},
					{
						.name		  = "patrol",
						.display_name = "Patrol",
					},
					{
						.name		  = "chase",
						.display_name = "Chase",
					},
				},
			.flags = reflected_type_flag_enum,
		});

		schema.register_type<example_settings_t>({
			.name		  = "game::example_settings_t",
			.display_name = "Settings",
			.fields =
				{
					{
						.name		  = "strength",
						.display_name = "Strength",
						.offset		  = offsetof(example_settings_t, strength),
						.size		  = sizeof(example_settings_t::strength),
						.type		  = reflected_value_type_e::f32,
					},
					{
						.name		  = "enabled",
						.display_name = "Enabled",
						.offset		  = offsetof(example_settings_t, enabled),
						.size		  = sizeof(example_settings_t::enabled),
						.type		  = reflected_value_type_e::boolean,
					},
				},
		});

		schema
			.register_type<component_example_t>(
				{
					.name		  = "game::component_example_t",
					.display_name = "Example Component",
					.category	  = "Game",
					.fields =
						{
							{
								.name		  = "label",
								.display_name = "Label",
								.offset		  = offsetof(component_example_t, label),
								.size		  = sizeof(component_example_t::label),
								.type		  = reflected_value_type_e::char_array,
							},
							{
								.name		  = "enabled",
								.display_name = "Enabled",
								.offset		  = offsetof(component_example_t, enabled),
								.size		  = sizeof(component_example_t::enabled),
								.type		  = reflected_value_type_e::boolean,
							},
							{
								.name		  = "amount",
								.display_name = "Amount",
								.offset		  = offsetof(component_example_t, amount),
								.size		  = sizeof(component_example_t::amount),
								.type		  = reflected_value_type_e::f32,
							},
							{
								.name		  = "unsigned_8",
								.display_name = "Unsigned 8",
								.offset		  = offsetof(component_example_t, unsigned_8),
								.size		  = sizeof(component_example_t::unsigned_8),
								.type		  = reflected_value_type_e::u8,
							},
							{
								.name		  = "signed_8",
								.display_name = "Signed 8",
								.offset		  = offsetof(component_example_t, signed_8),
								.size		  = sizeof(component_example_t::signed_8),
								.type		  = reflected_value_type_e::i8,
							},
							{
								.name		  = "unsigned_16",
								.display_name = "Unsigned 16",
								.offset		  = offsetof(component_example_t, unsigned_16),
								.size		  = sizeof(component_example_t::unsigned_16),
								.type		  = reflected_value_type_e::u16,
							},
							{
								.name		  = "signed_16",
								.display_name = "Signed 16",
								.offset		  = offsetof(component_example_t, signed_16),
								.size		  = sizeof(component_example_t::signed_16),
								.type		  = reflected_value_type_e::i16,
							},
							{
								.name		  = "unsigned_32",
								.display_name = "Unsigned 32",
								.offset		  = offsetof(component_example_t, unsigned_32),
								.size		  = sizeof(component_example_t::unsigned_32),
								.type		  = reflected_value_type_e::u32,
							},
							{
								.name		  = "signed_32",
								.display_name = "Signed 32",
								.offset		  = offsetof(component_example_t, signed_32),
								.size		  = sizeof(component_example_t::signed_32),
								.type		  = reflected_value_type_e::i32,
							},
							{
								.name		  = "unsigned_64",
								.display_name = "Unsigned 64",
								.offset		  = offsetof(component_example_t, unsigned_64),
								.size		  = sizeof(component_example_t::unsigned_64),
								.type		  = reflected_value_type_e::u64,
							},
							{
								.name		  = "signed_64",
								.display_name = "Signed 64",
								.offset		  = offsetof(component_example_t, signed_64),
								.size		  = sizeof(component_example_t::signed_64),
								.type		  = reflected_value_type_e::i64,
							},
							{
								.name		  = "mode",
								.display_name = "Mode",
								.sub_type_id  = type_id_t<example_mode_e>::value,
								.offset		  = offsetof(component_example_t, mode),
								.size		  = sizeof(component_example_t::mode),
								.type		  = reflected_value_type_e::u32,
							},
							{
								.name		  = "abilities",
								.display_name = "Abilities",
								.offset		  = offsetof(component_example_t, abilities),
								.size		  = sizeof(component_example_t::abilities),
								.type		  = reflected_value_type_e::bitmask,
							},
							{
								.name		  = "vector_2",
								.display_name = "Vector 2",
								.sub_type_id  = type_id_t<vec2f_t>::value,
								.offset		  = offsetof(component_example_t, vector_2),
								.size		  = sizeof(component_example_t::vector_2),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "grid",
								.display_name = "Grid",
								.sub_type_id  = type_id_t<vec2u16_t>::value,
								.offset		  = offsetof(component_example_t, grid),
								.size		  = sizeof(component_example_t::grid),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "vector_3",
								.display_name = "Vector 3",
								.sub_type_id  = type_id_t<vec3f_t>::value,
								.offset		  = offsetof(component_example_t, vector_3),
								.size		  = sizeof(component_example_t::vector_3),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "vector_4",
								.display_name = "Vector 4",
								.sub_type_id  = type_id_t<vec4f_t>::value,
								.offset		  = offsetof(component_example_t, vector_4),
								.size		  = sizeof(component_example_t::vector_4),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "rotation",
								.display_name = "Rotation",
								.sub_type_id  = type_id_t<quat_t>::value,
								.offset		  = offsetof(component_example_t, rotation),
								.size		  = sizeof(component_example_t::rotation),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "tint",
								.display_name = "Tint",
								.sub_type_id  = type_id_t<color_t>::value,
								.offset		  = offsetof(component_example_t, tint),
								.size		  = sizeof(component_example_t::tint),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "settings",
								.display_name = "Settings",
								.sub_type_id  = type_id_t<example_settings_t>::value,
								.offset		  = offsetof(component_example_t, settings),
								.size		  = sizeof(component_example_t::settings),
								.type		  = reflected_value_type_e::object,
							},
							{
								.name		  = "target",
								.display_name = "Target",
								.sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID,
								.offset		  = offsetof(component_example_t, target),
								.size		  = sizeof(component_example_t::target),
								.type		  = reflected_value_type_e::u64,
							},
							{
								.name		  = "mesh",
								.display_name = "Mesh",
								.sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH,
								.offset		  = offsetof(component_example_t, mesh),
								.size		  = sizeof(component_example_t::mesh),
								.type		  = reflected_value_type_e::u64,
							},
							{
								.container_ops = reflection_container_ops_t::array_ops<i32, component_example_t::SAMPLE_COUNT>(reflected_value_type_e::i32),
								.name		   = "samples",
								.display_name  = "Samples",
								.offset		   = offsetof(component_example_t, samples),
								.size		   = sizeof(component_example_t::samples),
								.type		   = reflected_value_type_e::container,
							},
							{
								.container_ops = reflection_container_ops_t::sized_array_ops<example_integer_list_t, i32, example_integer_list_t::CAPACITY, &example_integer_list_t::values, &example_integer_list_t::count>(reflected_value_type_e::i32),
								.name		   = "integers",
								.display_name  = "Integers",
								.offset		   = offsetof(component_example_t, integers),
								.size		   = sizeof(component_example_t::integers),
								.type		   = reflected_value_type_e::container,
							},
							{
								.container_ops = reflection_container_ops_t::array_ops<example_settings_t, component_example_t::PRESET_COUNT>(reflected_value_type_e::object, type_id_t<example_settings_t>::value),
								.name		   = "presets",
								.display_name  = "Presets",
								.offset		   = offsetof(component_example_t, presets),
								.size		   = sizeof(component_example_t::presets),
								.type		   = reflected_value_type_e::container,
							},
						},
					.bitmask_opts = {
						.get_option_count_fn = [](void* user_data) -> u32 { return 2; },
						.get_option_fn		 = [](u32 index, void* user_data) -> bitmask_option_t {
							switch (index)
							{
							case 0:
								return {.name = "Move", .value = static_cast<u64>(example_ability_e::move)};
							case 1:
								return {.name = "Jump", .value = static_cast<u64>(example_ability_e::jump)};
							default:
								SFG_ASSERT(false);
								return {};
							}
						},
						.build_title_fn = [](u64 value, void* user_data) -> const char* {
							switch (value)
							{
							case 0:
								return "None";
							case static_cast<u64>(example_ability_e::move):
								return "Move";
							case static_cast<u64>(example_ability_e::jump):
								return "Jump";
							default:
								return "Move, Jump";
							}
						},
					},
					.flags = reflected_type_flag_component,
				});
	}
}
