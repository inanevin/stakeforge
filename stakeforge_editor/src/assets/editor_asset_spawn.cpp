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

#include "editor_asset_spawn.hpp"
#include "editor_asset_io.hpp"
#include "editor_asset_manager.hpp"
#include "editor_asset_util.hpp"
#include "commands/editor_command_prefab_spawn.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_payload_controller.hpp"
#include "world/editor_world.hpp"
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation_def.hpp>
#include <sfg/runtime/resources/animation_library_def.hpp>
#include <sfg/runtime/resources/mesh_def.hpp>
#include <sfg/runtime/resources/ragdoll_def.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		struct asset_spawn_payload_t
		{
			resource_handle_t	  materials[decltype(component_mesh_renderer_t::materials)::capacity] = {};
			resource_handle_t	  mesh																  = NULL_RESOURCE_HANDLE;
			resource_handle_t	  skeleton															  = NULL_RESOURCE_HANDLE;
			resource_handle_t	  asset																  = NULL_RESOURCE_HANDLE;
			entity_guid_t		  guid																  = NULL_ENTITY_GUID;
			vec3f_t				  position															  = vec3f_t::zero;
			chunk_handle32_t	  name																  = {};
			chunk_handle32_t	  previous_selection												  = {};
			editor_world_handle_t world																  = {};
			entity_id_t			  parent															  = NULL_ENTITY_ID;
			entity_id_t			  entity															  = NULL_ENTITY_ID;
			entity_id_t			  previous_anchor													  = NULL_ENTITY_ID;
			u32					  previous_selection_count											  = 0;
			u32					  material_count													  = 0;
			editor_asset_type_e	  type																  = editor_asset_type_e::invalid;
			bool				  skinned															  = false;
			bool				  place																  = false;
		};

		static_assert(std::is_trivially_copyable_v<asset_spawn_payload_t>);
		static_assert(decltype(component_mesh_renderer_t::materials)::capacity == decltype(component_skinned_mesh_renderer_t::materials)::capacity);

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

		bool asset_spawn_undo(editor_command_system_t& system, editor_command_t& command)
		{
			asset_spawn_payload_t& payload		= system.get_payload_as<asset_spawn_payload_t>(command);
			editor_world_t&		   editor_world = *editor_world_controller_t::get().get_editor_world(payload.world);
			editor_world.get_world().destroy_entity_tree(payload.entity);
			payload.entity = NULL_ENTITY_ID;

			const entity_id_t* selection = payload.previous_selection_count == 0 ? nullptr : system.get_aux_data().get<entity_id_t>(payload.previous_selection);
			editor_world.get_edit_context().apply_entity_selection({.data = selection, .size = payload.previous_selection_count}, payload.previous_anchor);

			return true;
		}

		bool asset_spawn_redo(editor_command_system_t& system, editor_command_t& command)
		{
			asset_spawn_payload_t& payload		= system.get_payload_as<asset_spawn_payload_t>(command);
			editor_world_t&		   editor_world = *editor_world_controller_t::get().get_editor_world(payload.world);
			world_t&			   world		= editor_world.get_world();
			const entity_id_t	   entity		= world.create_entity(system.get_aux_data().get_text(payload.name), payload.guid);

			if (payload.parent != NULL_ENTITY_ID)
				world.attach_to(entity, payload.parent);

			if (payload.place)
				world.set_entity_pos_local(entity, world.abs_pos_to_local(entity, payload.position));

			if (payload.skinned)
			{
				component_skinned_mesh_renderer_t& renderer = world.get_component_table<component_skinned_mesh_renderer_t>().add_or_get_as<component_skinned_mesh_renderer_t>(entity);
				renderer.mesh								= payload.mesh;
				renderer.skeleton							= payload.skeleton;

				for (u32 i = 0; i < payload.material_count; ++i)
					renderer.materials.push_back(payload.materials[i]);
			}
			else
			{
				component_mesh_renderer_t& renderer = world.get_component_table<component_mesh_renderer_t>().add_or_get_as<component_mesh_renderer_t>(entity);
				renderer.mesh						= payload.mesh;

				for (u32 i = 0; i < payload.material_count; ++i)
					renderer.materials.push_back(payload.materials[i]);
			}

			switch (payload.type)
			{
			case editor_asset_type_e::animation_library:
				world.get_component_table<component_animation_library_t>().add_or_get_as<component_animation_library_t>(entity).animation_library = payload.asset;
				break;
			case editor_asset_type_e::animation:
				world.get_component_table<component_animation_player_t>().add_or_get_as<component_animation_player_t>(entity).animation = payload.asset;
				break;
			case editor_asset_type_e::ragdoll:
				world.get_component_table<component_ragdoll_t>().add_or_get_as<component_ragdoll_t>(entity).ragdoll = payload.asset;
				break;
			default:
				break;
			}

			world.scan_for_resources(entity, true);
			payload.entity = entity;
			payload.guid   = world.get_entity_guid(entity);
			editor_world.get_edit_context().apply_entity_selection({.data = &payload.entity, .size = 1}, payload.entity);

			return true;
		}

		bool asset_spawn_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			asset_spawn_payload_t& payload = system.get_payload_as<asset_spawn_payload_t>(command);
			system.get_aux_data().free(payload.name);

			if (payload.previous_selection)
				system.get_aux_data().free(payload.previous_selection);

			return true;
		}

		bool spawn_asset(const editor_asset_spawn_desc_t& desc, const editor_asset_t& asset)
		{
			asset_spawn_payload_t payload{.asset = asset.guid, .world = desc.world, .parent = desc.parent, .type = asset.asset_type};

			switch (asset.asset_type)
			{
			case editor_asset_type_e::prefab:
				return editor_command_prefab_spawn_t::spawn(desc.world, asset.guid, desc.parent, desc.world_position) != NULL_ENTITY_ID;
			case editor_asset_type_e::material:
				payload.mesh		   = DEFAULT_MESH_SPHERE_GUID;
				payload.materials[0]   = asset.guid;
				payload.material_count = 1;
				break;
			case editor_asset_type_e::mesh:
				payload.mesh = asset.guid;
				break;
			case editor_asset_type_e::animation: {
				animation_def_t animation = {};

				if (!editor_asset_util_t::load_animation_def(asset, animation))
					return false;

				payload.mesh	 = animation.preview_mesh;
				payload.skeleton = animation.preview_skeleton;
				break;
			}
			case editor_asset_type_e::animation_library: {
				animation_library_def_t library = {};

				if (!reflection_registry_t::get().type_from_json(type_id_t<animation_library_def_t>::value, &library, nullptr, editor_asset_io_t::get_embedded_source_json(asset)))
				{
					SFG_ERR("failed to read animation library {0} for spawning", asset.guid);
					return false;
				}

				payload.skeleton = library.skeleton;
				break;
			}
			case editor_asset_type_e::ragdoll: {
				ragdoll_def_t ragdoll = {};

				if (!reflection_registry_t::get().type_from_json(type_id_t<ragdoll_def_t>::value, &ragdoll, nullptr, editor_asset_io_t::get_embedded_source_json(asset)))
				{
					SFG_ERR("failed to read ragdoll {0} for spawning", asset.guid);
					return false;
				}

				payload.skeleton = ragdoll.target_skeleton;
				break;
			}
			default:
				return false;
			}

			if (asset.asset_type != editor_asset_type_e::material)
			{
				editor_asset_manager_t& assets = editor_asset_manager_t::get();

				if (asset.asset_type != editor_asset_type_e::mesh)
				{
					const editor_asset_t* skeleton_asset = assets.find_asset(payload.skeleton);
					skeleton_def_t		  skeleton		 = {};

					if (skeleton_asset == nullptr || skeleton_asset->asset_type != editor_asset_type_e::skeleton || !editor_asset_util_t::load_skeleton_def(*skeleton_asset, skeleton))
					{
						SFG_WARN("asset {0} needs a valid skeleton before it can be spawned", asset.guid);
						return false;
					}

					if (payload.mesh == NULL_RESOURCE_HANDLE)
						payload.mesh = skeleton.preview_mesh;
				}

				const editor_asset_t* mesh_asset = assets.find_asset(payload.mesh);
				mesh_def_t			  mesh		 = {};

				if (mesh_asset == nullptr || mesh_asset->asset_type != editor_asset_type_e::mesh)
				{
					SFG_WARN("asset {0} needs a valid preview mesh before it can be spawned", asset.guid);
					return false;
				}

				if (!editor_asset_util_t::load_mesh_def(*mesh_asset, mesh))
					return false;

				payload.skinned = !mesh.skinned_primitives.empty();

				if (payload.skinned && payload.skeleton == NULL_RESOURCE_HANDLE)
					payload.skeleton = editor_asset_util_t::find_skeleton_for_mesh(payload.mesh);

				if (payload.skinned && payload.skeleton == NULL_RESOURCE_HANDLE)
				{
					SFG_WARN("skinned mesh {0} needs a unique skeleton preview-mesh association before it can be spawned", payload.mesh);
					return false;
				}

				if (asset.asset_type == editor_asset_type_e::ragdoll && !payload.skinned)
				{
					SFG_WARN("ragdoll {0} needs a skinned preview mesh", asset.guid);
					return false;
				}

				payload.material_count = 1;

				for (const primitive_static_def_t& primitive : mesh.static_primitives)
				{
					if (primitive.material_index != UINT32_MAX)
						payload.material_count = std::max(payload.material_count, primitive.material_index + 1);
				}

				for (const primitive_skinned_def_t& primitive : mesh.skinned_primitives)
				{
					if (primitive.material_index != UINT32_MAX)
						payload.material_count = std::max(payload.material_count, primitive.material_index + 1);
				}

				if (payload.material_count > std::size(payload.materials))
				{
					SFG_WARN("mesh {0} uses more material slots than a renderer supports", payload.mesh);
					return false;
				}

				for (u32 i = 0; i < payload.material_count; ++i)
					payload.materials[i] = i < mesh.preview_materials.size() && mesh.preview_materials[i] != NULL_RESOURCE_HANDLE ? mesh.preview_materials[i] : DEFAULT_OPAQUE_MATERIAL_ASSET_GUID;
			}

			if (desc.world_position != nullptr)
			{
				payload.position = *desc.world_position;
				payload.place	 = true;
			}

			editor_world_edit_context_t&	context	  = editor_world_controller_t::get().get_editor_world(desc.world)->get_edit_context();
			editor_command_system_t&		system	  = editor_command_system_t::get();
			const span_t<const entity_id_t> selection = context.get_selected_entities();
			const char*						name	  = editor_asset_util_t::find_asset_display_name(asset.guid);
			payload.name							  = system.get_aux_data().allocate_text(name != nullptr ? name : "Asset");
			payload.previous_anchor					  = context.get_entity_anchor();
			payload.previous_selection_count		  = static_cast<u32>(selection.size);

			if (selection.size != 0)
			{
				entity_id_t* destination   = nullptr;
				payload.previous_selection = system.get_aux_data().allocate<entity_id_t>(selection.size, destination);
				SFG_MEMCPY(destination, selection.data, selection.size * sizeof(entity_id_t));
			}

			const editor_command_handle_t command = system.issue_command(
				{
					.undo			   = asset_spawn_undo,
					.redo			   = asset_spawn_redo,
					.cleanup		   = asset_spawn_cleanup,
					.debug_name		   = "Spawn Asset",
					.type			   = editor_command_type_e::entity_create,
					.world			   = desc.world,
					.entity_generation = true,
				},
				payload);

			return !command.is_null();
		}
	}

	bool editor_asset_spawn_t::spawn_from_payload(const editor_asset_spawn_desc_t& desc)
	{
		editor_world_controller_t& controller = editor_world_controller_t::get();

		if (desc.world.is_null() || desc.world != controller.get_main_world_handle())
			return false;

		editor_world_t& world = *controller.get_editor_world(desc.world);

		if (world.get_edit_context().get_edit_type() != editor_world_edit_type_e::full_control || world.get_play_mode() != editor_play_mode_e::none)
			return false;

		if (desc.parent != NULL_ENTITY_ID && !world.get_edit_context().is_entity_child_insertion_allowed(world.get_world(), desc.parent))
			return false;

		if (desc.payload->type == editor_payload_type_e::asset)
		{
			const editor_asset_node_handle_t node  = *static_cast<const editor_asset_node_handle_t*>(desc.payload->user_ptr);
			const editor_asset_t*			 asset = get_asset_from_node(node);

			return asset != nullptr && spawn_asset(desc, *asset);
		}

		bool spawned = false;

		if (desc.payload->type == editor_payload_type_e::asset_multi)
		{
			const vector_t<editor_asset_node_handle_t>& nodes = *static_cast<const vector_t<editor_asset_node_handle_t>*>(desc.payload->user_ptr);

			for (const editor_asset_node_handle_t node : nodes)
			{
				const editor_asset_t* asset = get_asset_from_node(node);

				if (asset != nullptr)
					spawned = spawn_asset(desc, *asset) || spawned;
			}
		}

		return spawned;
	}
}
