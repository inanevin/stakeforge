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

#include "editor_command_entity_util.hpp"
#include "editor/editor_world_controller.hpp"
#include "editor/world/editor_world.hpp"
#include <sfg/core/data/istream.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/memory/chunk_allocator.hpp>
#include <sfg/core/runtime/resources/world_cook_entity_header.hpp>
#include <sfg/core/runtime/world/world.hpp>

namespace sfg
{
	chunk_handle32_t editor_command_entity_util_t::capture_entities(const world_t& world, chunk_allocator_t& aux, span_t<const entity_id_t> entities)
	{
		if (entities.size == 0)
			return {};

		entity_guid_t*		   guids  = nullptr;
		const chunk_handle32_t handle = aux.allocate<entity_guid_t>(entities.size, guids);

		for (size_t i = 0; i < entities.size; ++i)
			guids[i] = world.get_entity_guid(entities.data[i]);

		return handle;
	}

	bool editor_command_entity_util_t::resolve_entities(const world_t& world, const chunk_allocator_t& aux, chunk_handle32_t guids, frame_vector_t<entity_id_t>& entities, bool allow_null)
	{
		const size_t		 count = guids.size / sizeof(entity_guid_t);
		const entity_guid_t* data  = count != 0 ? aux.get<entity_guid_t>(guids) : nullptr;

		entities.resize(count);

		for (size_t i = 0; i < count; ++i)
		{
			entities[i] = world.find_by_guid(data[i]);

			if (allow_null && data[i] == NULL_ENTITY_GUID)
				continue;

			if (entities[i] == NULL_ENTITY_ID)
			{
				SFG_ERR("command entity no longer exists: {0}", data[i]);
				return false;
			}
		}

		return true;
	}

	void editor_command_entity_util_t::restore_selection(editor_world_handle_t world, const chunk_allocator_t& aux, chunk_handle32_t guids, entity_guid_t anchor)
	{
		editor_world_t&				editor_world  = *editor_world_controller_t::get().get_editor_world(world);
		const world_t&				runtime_world = editor_world.get_world();
		const size_t				count		  = guids.size / sizeof(entity_guid_t);
		const entity_guid_t*		data		  = count != 0 ? aux.get<entity_guid_t>(guids) : nullptr;
		frame_vector_t<entity_id_t> entities	  = {};

		entities.reserve(count);

		for (size_t i = 0; i < count; ++i)
		{
			const entity_id_t entity = runtime_world.find_by_guid(data[i]);

			if (entity != NULL_ENTITY_ID)
				entities.push_back(entity);
		}

		editor_world.get_edit_context().apply_entity_selection({.data = entities.data(), .size = entities.size()}, runtime_world.find_by_guid(anchor));
	}

	bool editor_command_entity_util_t::can_restore_tree(const world_t& world, const chunk_allocator_t& aux, chunk_handle32_t snapshot)
	{
		istream_t stream(aux.get<u8>(snapshot), snapshot.size);
		u32		  count = 0;

		stream >> count;

		for (u32 i = 0; i < count; ++i)
		{
			world_cook_entity_header_t header = {};

			stream >> header;

			if (world.find_by_guid(header.guid) != NULL_ENTITY_ID)
			{
				SFG_ERR("cannot restore command entity; GUID already exists: {0}", header.guid);
				return false;
			}
		}

		return count != 0;
	}
}
