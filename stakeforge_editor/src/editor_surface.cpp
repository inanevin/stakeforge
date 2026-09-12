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

#include "editor_surface.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_primary_base.hpp"
#include "ui/panels/editor_secondary_base.hpp"
#include "ui/widgets/editor_widget_project_creator.hpp"
#include "ui/widgets/editor_widget_window_frame.hpp"
#include "ui/widgets/editor_splash_screen.hpp"
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	editor_surface_t::editor_surface_t() = default;

	editor_surface_t::~editor_surface_t() = default;

	editor_surface_t::editor_surface_t(editor_surface_t&& other) noexcept = default;

	editor_surface_t& editor_surface_t::operator=(editor_surface_t&& other) noexcept = default;
}
