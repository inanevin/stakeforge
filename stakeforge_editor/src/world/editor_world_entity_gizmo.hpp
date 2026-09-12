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

#include "world/editor_world_gizmo.hpp"
#include <sfg/data/vector.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;
	class editor_world_edit_context_t;

	class editor_world_entity_gizmo_t final
	{
	public:
		editor_world_entity_gizmo_t()											   = default;
		~editor_world_entity_gizmo_t()											   = default;
		editor_world_entity_gizmo_t(const editor_world_entity_gizmo_t&)			   = delete;
		editor_world_entity_gizmo_t& operator=(const editor_world_entity_gizmo_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world, const editor_world_edit_context_t& context);
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		editor_gizmo_callbacks_t get_callbacks();

	private:
		bool get_target(editor_gizmo_target_t& target) const;
		bool begin();
		void update(const mat4x3_t& delta);
		void commit();
		void cancel();
		void clear_action();

	private:
		vector_t<mat4x3_t>				   _initial_absolute		= {};
		vector_t<mat4x3_t>				   _initial_parent_inverse	= {};
		vector_t<quat_t>				   _initial_local_rotations = {};
		vector_t<vec3f_t>				   _initial_local_positions = {};
		vector_t<vec3f_t>				   _initial_local_scales	= {};
		vector_t<entity_id_t>			   _entities				= {};
		world_t*						   _world					= nullptr;
		const editor_world_edit_context_t* _context					= nullptr;
	};
}
