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
