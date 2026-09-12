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

#include "assets/editor_asset_spawn.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_command_prefab_spawn.hpp"
#include "ui/editor_payload_controller.hpp"
#include <sfg/runtime/resources/world_cook.hpp>

namespace sfg
{
	namespace
	{
		const editor_asset_t* get_asset_from_node(editor_asset_node_handle_t handle)
		{
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			if (handle.is_null() || !tree.is_valid(handle))
				return nullptr;

			const editor_asset_node_t& node = tree.value(handle);
			if (node.type != editor_asset_node_type_e::asset)
				return nullptr;

			return editor_asset_manager_t::get().find_asset(node.asset_id);
		}

	}

	bool editor_asset_spawn_t::spawn_from_payload(const editor_asset_spawn_desc_t& desc)
	{
		bool spawned = false;

		if (desc.payload->type == editor_payload_type_e::asset)
		{
			const editor_asset_node_handle_t payload_node = *static_cast<const editor_asset_node_handle_t*>(desc.payload->user_ptr);
			const editor_asset_t*			 asset		  = get_asset_from_node(payload_node);
			return asset != nullptr && editor_command_prefab_spawn_t::spawn(desc.world, asset->guid, desc.parent) != NULL_ENTITY_ID;
		}

		if (desc.payload->type == editor_payload_type_e::asset_multi)
		{
			const vector_t<editor_asset_node_handle_t>& payload_nodes = *static_cast<const vector_t<editor_asset_node_handle_t>*>(desc.payload->user_ptr);
			for (editor_asset_node_handle_t payload_node : payload_nodes)
			{
				const editor_asset_t* asset = get_asset_from_node(payload_node);
				if (asset != nullptr && asset->asset_type == editor_asset_type_e::prefab)
					spawned = editor_command_prefab_spawn_t::spawn(desc.world, asset->guid, desc.parent) != NULL_ENTITY_ID;
			}
		}

		return spawned;
	}
}
