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

#include "assets/editor_asset_spawn.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_command_prefab_spawn.hpp"
#include "ui/editor_payload_controller.hpp"

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

		bool spawn_prefab_asset(world_handle_t world, const editor_asset_t& asset, entity_id_t parent)
		{
			if (asset.asset_type != editor_asset_type_e::prefab)
				return false;

			return editor_command_prefab_spawn_t::spawn(world, asset.guid, parent) != NULL_ENTITY_ID;
		}
	}

	bool editor_asset_spawn_t::spawn_from_payload(const editor_asset_spawn_desc_t& desc)
	{
		SFG_ASSERT(desc.payload != nullptr);
		SFG_ASSERT(desc.payload->user_ptr != nullptr);
		SFG_ASSERT(!desc.world.is_null());

		bool spawned = false;

		if (desc.payload->type == editor_payload_type_e::asset)
		{
			const editor_asset_node_handle_t payload_node = *static_cast<const editor_asset_node_handle_t*>(desc.payload->user_ptr);
			const editor_asset_t*			 asset		  = get_asset_from_node(payload_node);
			return asset != nullptr && spawn_prefab_asset(desc.world, *asset, desc.parent);
		}

		if (desc.payload->type == editor_payload_type_e::asset_multi)
		{
			const vector_t<editor_asset_node_handle_t>& payload_nodes = *static_cast<const vector_t<editor_asset_node_handle_t>*>(desc.payload->user_ptr);
			for (editor_asset_node_handle_t payload_node : payload_nodes)
			{
				const editor_asset_t* asset = get_asset_from_node(payload_node);
				if (asset != nullptr)
					spawned = spawn_prefab_asset(desc.world, *asset, desc.parent) || spawned;
			}
		}

		return spawned;
	}
}
