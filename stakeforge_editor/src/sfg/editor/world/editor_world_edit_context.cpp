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
#include "editor/world/editor_world_edit_context.hpp"
#include "editor/editor_project.hpp"

namespace sfg
{
#define EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY 64
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_FOLDERS			  1024
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_SELECTION_LISTENERS 64

	void editor_world_edit_context_t::init(editor_world_edit_type_e edit_type)
	{
		_folders.init(EDITOR_WORLD_EDIT_CONTEXT_MAX_FOLDERS);
		_selection_listeners.init(EDITOR_WORLD_EDIT_CONTEXT_MAX_SELECTION_LISTENERS);
		_entity_metadata.reserve(EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY);
		_selected_entities.reserve(EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY);

		_selection_generation = 0;
		_edit_type			  = edit_type;
		_world_view_settings  = editor_project_t::get().settings.world_view_settings;
	}

	void editor_world_edit_context_t::uninit()
	{
		_entity_metadata.clear();
		_selected_entities.clear();
		_selection_listeners.uninit();
		_folders.uninit();

		_world				  = {};
		_entity_anchor		  = NULL_ENTITY_ID;
		_editor_camera_entity = NULL_ENTITY_ID;
		_next_guid			  = 1;
		_selection_generation = 0;

		_world_view_settings	= {};
		_transform_control_type = editor_transform_control_type_e::move;
		_transform_locality		= editor_transform_locality_e::local;
		_transform_snapping		= editor_transform_snapping_e::none;
		_world_view				= editor_world_view_e::lit;

		_grid_enabled			= false;
		_bounding_boxes_enabled = false;
		_physics_debug_enabled	= false;

		_edit_type = editor_world_edit_type_e::full_control;
	}

	void editor_world_edit_context_t::set_world(editor_world_handle_t world)
	{
		if (_world == world)
			return;

		_world = world;

		clear();
	}

	void editor_world_edit_context_t::clear()
	{
		_entity_metadata.resize(0);
		_folders.reset();
		_selected_entities.resize(0);

		_entity_anchor = NULL_ENTITY_ID;
		_next_guid	   = 1;
		++_selection_generation;

		notify_selection_listeners();
	}

	void editor_world_edit_context_t::set_entity_folded(entity_guid_t guid, bool folded)
	{
		get_or_create_entity_metadata(guid).folded = folded;
	}

	bool editor_world_edit_context_t::is_entity_expanded(entity_guid_t guid) const
	{
		const editor_world_entity_metadata_t* meta = find_entity_metadata(guid);

		return meta != nullptr && !meta->folded;
	}

	editor_world_entity_metadata_t& editor_world_edit_context_t::get_or_create_entity_metadata(entity_guid_t guid)
	{
		if (editor_world_entity_metadata_t* meta = find_entity_metadata(guid))
			return *meta;

		_entity_metadata.push_back({.guid = guid});

		return _entity_metadata.back();
	}

	editor_world_entity_metadata_t* editor_world_edit_context_t::find_entity_metadata(entity_guid_t guid)
	{
		for (editor_world_entity_metadata_t& meta : _entity_metadata)
		{
			if (meta.guid == guid)
				return &meta;
		}

		return nullptr;
	}

	const editor_world_entity_metadata_t* editor_world_edit_context_t::find_entity_metadata(entity_guid_t guid) const
	{
		for (const editor_world_entity_metadata_t& meta : _entity_metadata)
		{
			if (meta.guid == guid)
				return &meta;
		}

		return nullptr;
	}

}
