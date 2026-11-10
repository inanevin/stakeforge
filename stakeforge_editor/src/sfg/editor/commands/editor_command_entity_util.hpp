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

#include "editor/editor_exports.hpp"
#include "editor/world/editor_world_handle.hpp"
#include <sfg/core/data/frame_vector.hpp>
#include <sfg/core/data/span.hpp>
#include <sfg/core/memory/chunk_handle.hpp>
#include <sfg/core/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;
	class chunk_allocator_t;

	class SFG_EDITOR_API editor_command_entity_util_t final
	{
	public:
		editor_command_entity_util_t() = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static chunk_handle32_t capture_entities(const world_t& world, chunk_allocator_t& aux, span_t<const entity_id_t> entities);
		static bool				resolve_entities(const world_t& world, const chunk_allocator_t& aux, chunk_handle32_t guids, frame_vector_t<entity_id_t>& entities, bool allow_null = false);
		static void				restore_selection(editor_world_handle_t world, const chunk_allocator_t& aux, chunk_handle32_t guids, entity_guid_t anchor);
		static bool				can_restore_tree(const world_t& world, const chunk_allocator_t& aux, chunk_handle32_t snapshot);
	};
}
