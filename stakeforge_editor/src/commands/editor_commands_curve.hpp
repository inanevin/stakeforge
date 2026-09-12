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

#include <sfg/data/span.hpp>
#include <sfg/memory/chunk_allocator.hpp>

namespace sfg
{
	struct curve_def_t;

	struct editor_command_curve_edit_payload_t
	{
		chunk_handle32_t curve_ids		= {};
		chunk_handle32_t previous_jsons = {};
		chunk_handle32_t post_jsons		= {};
		u32				 count			= 0;
	};

	class editor_command_curve_edit_t final
	{
	public:
		editor_command_curve_edit_t() = delete;

		static bool edit(span_t<const sid_t> curves, span_t<const curve_def_t> previous, span_t<const curve_def_t> post);
	};
}
