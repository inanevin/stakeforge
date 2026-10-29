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

#pragma once

#include "tools/tools_exports.hpp"
#include "tools/world/editor_world_handle.hpp"
#include <sfg/core/data/string.hpp>
#include <sfg/core/data/span.hpp>
#include <sfg/core/data/vector.hpp>
#include <sfg/core/runtime/world/ecs_defs.hpp>
#include <sfg/core/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class editor_command_system_t;
	class world_t;
	struct editor_command_t;
	struct editor_command_listener_tag_t;

	class SFG_TOOLS_API editor_play_edits_t final
	{
	public:
		editor_play_edits_t()									   = default;
		~editor_play_edits_t()									   = default;
		editor_play_edits_t(const editor_play_edits_t&)			   = delete;
		editor_play_edits_t& operator=(const editor_play_edits_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void begin(editor_world_handle_t world);
		void end();
		void clear();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool apply(world_t& world, bool& changed);
		bool capture_before(editor_world_handle_t world, span_t<const entity_id_t> entities, sid_t component_type);
		bool capture_after(editor_world_handle_t world, span_t<const entity_id_t> entities, sid_t component_type);
		bool apply_pending(editor_world_handle_t world, entity_id_t entity, sid_t component_type, void* component, bool& applied) const;
		void clear_pending(editor_world_handle_t world, sid_t component_type);

	private:
		enum class component_edit_e : u8
		{
			fields,
			replace,
			remove,
		};

		struct component_edit_t
		{
			string_t		 data			  = {};
			entity_guid_t	 entity			  = NULL_ENTITY_GUID;
			sid_t			 component_type	  = 0;
			u32				 sequence		  = 0;
			component_edit_e operation		  = component_edit_e::fields;
			bool			 initially_undone = false;
			bool			 active			  = true;
		};

		struct pending_edit_t
		{
			string_t	  original		 = {};
			string_t	  before		 = {};
			string_t	  authored		 = {};
			entity_guid_t entity		 = NULL_ENTITY_GUID;
			sid_t		  component_type = 0;
		};

		bool		record_component_command(editor_command_system_t& system, const editor_command_t& command);
		bool		record_entity_info_command(editor_command_system_t& system, const editor_command_t& command);
		bool		record_fields(const editor_command_t& command, entity_guid_t entity, sid_t component_type, const nlohmann::json& before, const nlohmann::json& after);
		static void on_command(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		vector_t<component_edit_t>						  _components = {};
		vector_t<pending_edit_t>						  _pending	  = {};
		pool_handle_t<u32, editor_command_listener_tag_t> _listener	  = {};
		editor_world_handle_t							  _world	  = {};
		bool											  _failed	  = false;
	};
}
