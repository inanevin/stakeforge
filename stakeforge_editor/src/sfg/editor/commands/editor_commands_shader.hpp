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

#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/span.hpp>

namespace sfg
{
	struct shader_cook_config_t;

	class SFG_EDITOR_API editor_command_shader_options_edit_t final
	{
	public:
		editor_command_shader_options_edit_t() = delete;

		static bool edit(span_t<const sid_t> shaders, span_t<const shader_cook_config_t> previous, span_t<const shader_cook_config_t> post);
	};
}
