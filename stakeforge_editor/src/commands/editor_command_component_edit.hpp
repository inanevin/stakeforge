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

#include "world/editor_world_handle.hpp"
#include <sfg/data/span.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class ostream_t;

	struct editor_command_component_edit_payload_t
	{
		chunk_handle32_t	  previous_streams = {};
		chunk_handle32_t	  post_streams	   = {};
		chunk_handle32_t	  entities		   = {};
		editor_world_handle_t world			   = {};
		sid_t				  component_type   = 0;
		u32					  count			   = 0;
	};

	class editor_command_component_edit_t final
	{
	public:
		editor_command_component_edit_t() = delete;

		static bool edit(editor_world_handle_t world, span_t<const entity_id_t> entities, sid_t component_type, span_t<const ostream_t> previous_streams, span_t<const ostream_t> post_streams);
	};
}
