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

#include "core/runtime/world/world_component_util.hpp"
#include "core/runtime/world/world.hpp"
#include "core/runtime/world/ecs.hpp"
#include "core/runtime/scripting/native_component_schema.hpp"

namespace sfg
{
	void world_component_util_t::apply_native_component_schema(world_t& world, const native_component_schema_t& previous, const native_component_schema_t& current, const native_component_schema_delta_t& delta)
	{
		SFG_ASSERT(!world.is_component_query_active());
		SFG_ASSERT(!world.is_playing());

		vector_t<ecs_component_table_t>& component_tables = world._component_tables;

		component_tables.reserve(component_tables.size() + delta.added.size());

		for (sid_t type_id : delta.removed)
		{
			const auto it = std::find_if(component_tables.begin(), component_tables.end(), [type_id](const ecs_component_table_t& table) { return table.get_type_id() == type_id; });
			SFG_ASSERT(it != component_tables.end());

			it->uninit();
			component_tables.erase(it);
		}

		for (sid_t type_id : delta.layout_changed)
		{
			const reflected_type_descriptor_t& type			   = *current.find_type(type_id);
			const bool						   tag			   = type.flags.is_set(reflected_type_flag_tag_component);
			ecs_component_table_t&			   previous_table  = world.get_component_table(type_id);
			ecs_component_table_t			   candidate_table = {};
			candidate_table.init(ecs_component_table_t::make_component_desc(type_id, tag ? 0 : type.size, tag ? 1 : type.alignment, tag ? ecs_component_type_flags_tag : ecs_component_type_flags_none, type.name));

			const ecs_component_table_ref_t refs[] = {previous_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				void* destination = candidate_table.add(row.id);

				if (tag)
					continue;

				type.default_init_fn(destination);

				if (previous_table.get_type_desc().size != 0)
					current.migrate_type(previous, type_id, row.components[0], destination);
			}

			previous_table.uninit();
			previous_table = std::move(candidate_table);
		}

		for (sid_t type_id : delta.added)
		{
			const reflected_type_descriptor_t& type = *current.find_type(type_id);
			const bool						   tag	= type.flags.is_set(reflected_type_flag_tag_component);

			world.add_component_table(ecs_component_table_t::make_component_desc(type_id, tag ? 0 : type.size, tag ? 1 : type.alignment, tag ? ecs_component_type_flags_tag : ecs_component_type_flags_none, type.name));
		}

		for (const reflected_type_descriptor_t& type : current.get_types())
		{
			if (!type.flags.is_set(reflected_type_flag_component | reflected_type_flag_system_component | reflected_type_flag_tag_component))
				continue;

			const bool tag = type.flags.is_set(reflected_type_flag_tag_component);

			world.get_component_table(type.type_id).set_type_desc(ecs_component_table_t::make_component_desc(type.type_id, tag ? 0 : type.size, tag ? 1 : type.alignment, tag ? ecs_component_type_flags_tag : ecs_component_type_flags_none, type.name));
		}

		world.refresh_component_table_cache();
	}
}
