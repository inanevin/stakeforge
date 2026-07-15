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
#include "world/editor_world_edit_context.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY 64
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_FOLDERS			  1024
#define EDITOR_WORLD_EDIT_CONTEXT_MAX_SELECTION_LISTENERS 64

	void editor_world_edit_context_t::init()
	{
		_folders.init(EDITOR_WORLD_EDIT_CONTEXT_MAX_FOLDERS);
		_selection_listeners.init(EDITOR_WORLD_EDIT_CONTEXT_MAX_SELECTION_LISTENERS);
		_entity_metadata.reserve(EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY);
		_outliner_items.reserve(EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY);
		_selected_entities.reserve(EDITOR_WORLD_EDIT_CONTEXT_INITIAL_ENTITY_CAPACITY);
		_selection_generation = 0;
	}

	void editor_world_edit_context_t::uninit()
	{
		_outliner_items.clear();
		_entity_metadata.clear();
		_selected_entities.clear();
		_selection_listeners.uninit();
		_folders.uninit();
		_world					= {};
		_entity_anchor			= NULL_ENTITY_ID;
		_next_guid				= 1;
		_selection_generation	= 0;
		_world_view_settings	= {};
		_transform_control_type = editor_transform_control_type_e::move;
		_transform_locality		= editor_transform_locality_e::local;
		_transform_snapping		= editor_transform_snapping_e::none;
		_grid_enabled			= false;
		_bounding_boxes_enabled = false;
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
		_outliner_items.resize(0);
		_entity_metadata.resize(0);
		_folders.reset();
		_selected_entities.resize(0);
		_entity_anchor = NULL_ENTITY_ID;
		_next_guid	   = 1;
		++_selection_generation;
		notify_selection_listeners();
	}

}
