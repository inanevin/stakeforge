/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "physics_project_settings.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	physics_project_settings_t::physics_project_settings_t()
	{
		collision_layers.push_back({.name = "Default", .collides_with = 1, .identifier = 1, .slot = 0});
	}

	void physics_project_settings_t::normalize(const physics_project_settings_t* previous)
	{
		if (collision_layers.size() > PHYSICS_COLLISION_LAYER_MAX)
			collision_layers.resize(PHYSICS_COLLISION_LAYER_MAX);
		if (collision_layers.empty())
			collision_layers.push_back({.name = "Default", .collides_with = 1, .identifier = next_collision_layer_identifier++, .slot = 0});

		bool used_slots[PHYSICS_COLLISION_LAYER_MAX] = {};
		for (u32 i = 0; i < collision_layers.size(); ++i)
		{
			physics_collision_layer_definition_t& layer	 = collision_layers[i];
			const bool							  is_new = layer.identifier == 0;
			if (layer.identifier == 0)
				layer.identifier = next_collision_layer_identifier++;
			if (layer.identifier >= next_collision_layer_identifier)
				next_collision_layer_identifier = layer.identifier + 1;
			if (used_slots[layer.slot])
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
		for (const physics_collision_layer_definition_t& layer : collision_layers)
			active_layers |= 1ull << layer.slot;
		for (physics_collision_layer_definition_t& layer : collision_layers)
			layer.collides_with &= active_layers;

		for (u32 i = 0; i < collision_layers.size(); ++i)
		{
			for (u32 j = i; j < collision_layers.size(); ++j)
			{
				const u64  bit_i	= 1ull << collision_layers[i].slot;
				const u64  bit_j	= 1ull << collision_layers[j].slot;
				const bool collides = (collision_layers[i].collides_with & bit_j) != 0 || (collision_layers[j].collides_with & bit_i) != 0;
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

	physics_runtime_config_t physics_project_settings_t::make_runtime_config(u32 physics_rate, u32 max_sub_steps) const
	{
		physics_runtime_config_t config;
		config.active_collision_layers = 0;
		for (u32 i = 0; i < PHYSICS_COLLISION_LAYER_MAX; ++i)
			config.collision_masks[i] = 0;

		for (const physics_collision_layer_definition_t& layer : collision_layers)
		{
			config.collision_masks[layer.slot] = layer.collides_with;
			config.active_collision_layers |= 1ull << layer.slot;
		}
		config.physics_rate	 = physics_rate;
		config.max_sub_steps = max_sub_steps;
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
					{.name = "name", .display_name = "Name", .offset = offsetof(physics_collision_layer_definition_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "collides_with", .display_name = "Collides With", .offset = offsetof(physics_collision_layer_definition_t, collides_with), .size = sizeof(u64), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{.name = "identifier", .display_name = "Identifier", .offset = offsetof(physics_collision_layer_definition_t, identifier), .size = sizeof(u64), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{.name = "slot", .display_name = "Slot", .offset = offsetof(physics_collision_layer_definition_t, slot), .size = sizeof(u8), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u8},
				},
			.type_id   = type_id_t<physics_collision_layer_definition_t>::value,
			.size	   = sizeof(physics_collision_layer_definition_t),
			.alignment = alignof(physics_collision_layer_definition_t),
		});

		registry.register_type({
			.name		  = "physics_project_settings_t",
			.display_name = "Physics",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<physics_collision_layer_definition_t>(reflected_value_type_e::object, type_id_t<physics_collision_layer_definition_t>::value),
					 .name			= "collision_layers",
					 .display_name	= "Collision Layers",
					 .offset		= offsetof(physics_project_settings_t, collision_layers),
					 .size			= sizeof(vector_t<physics_collision_layer_definition_t>),
					 .type			= reflected_value_type_e::container},
					{.name		   = "next_collision_layer_identifier",
					 .display_name = "Next Collision Layer Identifier",
					 .offset	   = offsetof(physics_project_settings_t, next_collision_layer_identifier),
					 .size		   = sizeof(u64),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::u64},
				},
			.type_id   = type_id_t<physics_project_settings_t>::value,
			.size	   = sizeof(physics_project_settings_t),
			.alignment = alignof(physics_project_settings_t),
		});
	}
}
