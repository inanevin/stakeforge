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
#include "ui/widgets/editor_widget_width.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>

namespace sfg
{
	void apply_editor_widget_width(ui::layout_in_t& in, const editor_widget_width_config_t& width)
	{
		if (width.mode == editor_widget_width_e::fixed)
		{
			in.size_mode_x	= ui::axis_mode_e::fixed;
			in.size_value.x = width.value;
			return;
		}

		in.size_mode_x	= ui::axis_mode_e::parent_relative;
		in.size_value.x = 1.0f;
	}
}
