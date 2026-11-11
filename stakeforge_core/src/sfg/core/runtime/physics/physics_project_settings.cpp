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

#include "core/runtime/physics/physics_project_settings.hpp"

#include "core/reflection/reflection_container_ops.hpp"
#include "core/reflection/reflection_registry.hpp"
#include "core/runtime/engine/engine_runtime.hpp"

namespace sfg
{
	physics_project_settings_reflection_t g_reflect_physics_project_settings = {};

	namespace
	{
		u32 get_collision_layer_bitmask_option_count(void* user_data)
		{
			return static_cast<u32>(engine_runtime_t::get().get_project_settings().physics.collision_layer_count);
		}

		bitmask_option_t get_collision_layer_bitmask_option(u32 index, void* user_data)
		{
			const physics_collision_layer_definition_t& layer = engine_runtime_t::get().get_project_settings().physics.collision_layers[index];

			return {
				.name  = layer.name[0] == '\0' ? "Unnamed Layer" : layer.name,
				.value = 1ull << layer.slot,
			};
		}

		const char* build_collision_layer_bitmask_title(u64 value, void* user_data)
		{
			if (value == 0)
				return "None";

			static thread_local string_t title = {};
			title.resize(0);

			const physics_project_settings_t& physics = engine_runtime_t::get().get_project_settings().physics;

			for (size_t i = 0; i < physics.collision_layer_count; ++i)
			{
				const physics_collision_layer_definition_t& layer = physics.collision_layers[i];

				if ((value & (1ull << layer.slot)) == 0)
					continue;

				if (!title.empty())
					title += " | ";

				if (layer.name[0] == '\0')
					title += "Unnamed Layer";
				else
					title += layer.name;
			}

			return title.empty() ? "Unknown" : title.c_str();
		}
	}

	bool physics_collision_layer_definition_t::operator==(const physics_collision_layer_definition_t& other) const
	{
		if (collides_with != other.collides_with || identifier != other.identifier || slot != other.slot)
			return false;

		return std::strcmp(name, other.name) == 0;
	}

	bool physics_project_settings_t::operator==(const physics_project_settings_t& other) const
	{
		if (collision_layer_count != other.collision_layer_count || next_collision_layer_identifier != other.next_collision_layer_identifier)
			return false;

		if (kinematic_sensors_collide_with_non_dynamic != other.kinematic_sensors_collide_with_non_dynamic)
			return false;

		return std::equal(collision_layers, collision_layers + collision_layer_count, other.collision_layers);
	}

	physics_project_settings_t::physics_project_settings_t()
	{
		collision_layers[0] = {.collides_with = 1, .identifier = 1, .name = "Default", .slot = 0};
	}

	void physics_project_settings_t::normalize(const physics_project_settings_t* previous)
	{
		if (collision_layer_count == 0)
		{
			collision_layers[0]	  = {.collides_with = 1, .identifier = next_collision_layer_identifier++, .name = "Default", .slot = 0};
			collision_layer_count = 1;
		}

		bool used_slots[PHYSICS_COLLISION_LAYER_MAX] = {};

		for (u32 i = 0; i < collision_layer_count; ++i)
		{
			physics_collision_layer_definition_t& layer	 = collision_layers[i];
			const bool							  is_new = layer.identifier == 0;

			if (layer.identifier == 0)
				layer.identifier = next_collision_layer_identifier++;

			if (layer.identifier >= next_collision_layer_identifier)
				next_collision_layer_identifier = layer.identifier + 1;

			if (layer.slot >= PHYSICS_COLLISION_LAYER_MAX || used_slots[layer.slot])
			{
				for (u32 slot = 0; slot < PHYSICS_COLLISION_LAYER_MAX; ++slot)
				{
					if (!used_slots[slot])
					{
						layer.slot = static_cast<u8>(slot);
						break;
					}
				}
			}

			used_slots[layer.slot] = true;

			if (is_new)
				layer.collides_with |= 1ull << layer.slot;
		}

		u64 active_layers = 0;

		for (size_t i = 0; i < collision_layer_count; ++i)
			active_layers |= 1ull << collision_layers[i].slot;

		for (size_t i = 0; i < collision_layer_count; ++i)
			collision_layers[i].collides_with &= active_layers;

		const physics_collision_layer_definition_t* previous_layers[PHYSICS_COLLISION_LAYER_MAX] = {};

		if (previous != nullptr)
		{
			for (u32 i = 0; i < collision_layer_count; ++i)
			{
				const auto it = std::find_if(previous->collision_layers, previous->collision_layers + previous->collision_layer_count, [&](const physics_collision_layer_definition_t& layer) { return layer.identifier == collision_layers[i].identifier; });

				if (it != previous->collision_layers + previous->collision_layer_count)
					previous_layers[i] = &*it;
			}
		}

		for (u32 i = 0; i < collision_layer_count; ++i)
		{
			for (u32 j = i; j < collision_layer_count; ++j)
			{
				const u64  bit_i	 = 1ull << collision_layers[i].slot;
				const u64  bit_j	 = 1ull << collision_layers[j].slot;
				const bool current_i = (collision_layers[i].collides_with & bit_j) != 0;
				const bool current_j = (collision_layers[j].collides_with & bit_i) != 0;
				bool	   collides	 = current_i || current_j;

				if (previous_layers[i] != nullptr && previous_layers[j] != nullptr)
				{
					const bool previous_i = (previous_layers[i]->collides_with & (1ull << previous_layers[j]->slot)) != 0;
					const bool previous_j = (previous_layers[j]->collides_with & (1ull << previous_layers[i]->slot)) != 0;
					const bool changed_i  = current_i != previous_i;
					const bool changed_j  = current_j != previous_j;

					if (changed_i != changed_j)
						collides = changed_i ? current_i : current_j;
				}

				if (collides)
				{
					collision_layers[i].collides_with |= bit_j;
					collision_layers[j].collides_with |= bit_i;
				}
				else
				{
					collision_layers[i].collides_with &= ~bit_j;
					collision_layers[j].collides_with &= ~bit_i;
				}
			}
		}
	}

	world_physics_config_t physics_project_settings_t::make_runtime_config(u32 physics_rate, u32 max_sub_steps) const
	{
		world_physics_config_t config  = {};
		config.active_collision_layers = 0;

		for (u32 i = 0; i < PHYSICS_COLLISION_LAYER_MAX; ++i)
			config.collision_masks[i] = 0;

		for (size_t i = 0; i < collision_layer_count; ++i)
		{
			const physics_collision_layer_definition_t& layer = collision_layers[i];

			config.collision_masks[layer.slot] = layer.collides_with;
			config.active_collision_layers |= 1ull << layer.slot;
		}

		config.physics_rate								  = physics_rate;
		config.max_sub_steps							  = max_sub_steps;
		config.kinematic_sensors_collide_with_non_dynamic = kinematic_sensors_collide_with_non_dynamic;

		return config;
	}

	physics_project_settings_reflection_t::physics_project_settings_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "physics_collision_layer_definition_t",
			.display_name = "Collision Layer",
			.fields =
				{
					{
						.name		  = "name",
						.display_name = "Name",
						.offset		  = offsetof(physics_collision_layer_definition_t, name),
						.size		  = sizeof(physics_collision_layer_definition_t::name),
						.type		  = reflected_value_type_e::char_array,
					},
					{.name = "collides_with", .display_name = "Collides With", .offset = offsetof(physics_collision_layer_definition_t, collides_with), .size = sizeof(u64), .type = reflected_value_type_e::bitmask},
					{.name = "identifier", .display_name = "Identifier", .offset = offsetof(physics_collision_layer_definition_t, identifier), .size = sizeof(u64), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{.name = "slot", .display_name = "Slot", .offset = offsetof(physics_collision_layer_definition_t, slot), .size = sizeof(u8), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u8},
				},
			.bitmask_opts = {.get_option_count_fn = get_collision_layer_bitmask_option_count, .get_option_fn = get_collision_layer_bitmask_option, .build_title_fn = build_collision_layer_bitmask_title},
			.type_id	  = type_id_t<physics_collision_layer_definition_t>::value,
			.size		  = sizeof(physics_collision_layer_definition_t),
			.alignment	  = alignof(physics_collision_layer_definition_t),
		});

		registry.register_type({
			.name		  = "physics_project_settings_t",
			.display_name = "Physics",
			.fields =
				{
					{
						.container_ops = reflection_container_ops_t::
							sized_array_ops<physics_project_settings_t, physics_collision_layer_definition_t, PHYSICS_COLLISION_LAYER_MAX, &physics_project_settings_t::collision_layers, &physics_project_settings_t::collision_layer_count>(
								reflected_value_type_e::object, type_id_t<physics_collision_layer_definition_t>::value),
						.name		  = "collision_layers",
						.display_name = "Collision Layers",
						.offset		  = 0,
						.size		  = sizeof(physics_project_settings_t),
						.type		  = reflected_value_type_e::container,
					},
					{.name		   = "next_collision_layer_identifier",
					 .display_name = "Next Collision Layer Identifier",
					 .offset	   = offsetof(physics_project_settings_t, next_collision_layer_identifier),
					 .size		   = sizeof(u64),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::u64},
					{.name		   = "kinematic_sensors_collide_with_non_dynamic",
					 .display_name = "Kinematic Sensors vs Non-Dynamic",
					 .tooltip	   = "Allows kinematic sensors to generate trigger contacts with static and kinematic bodies. Enable only when needed and use restrictive collision layers.",
					 .offset	   = offsetof(physics_project_settings_t, kinematic_sensors_collide_with_non_dynamic),
					 .size		   = sizeof(bool),
					 .type		   = reflected_value_type_e::boolean},
				},
			.type_id   = type_id_t<physics_project_settings_t>::value,
			.size	   = sizeof(physics_project_settings_t),
			.alignment = alignof(physics_project_settings_t),
		});
	}
}
