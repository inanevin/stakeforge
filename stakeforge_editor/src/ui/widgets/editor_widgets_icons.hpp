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

#define ICON_SETTINGS_WHEEL	 "\u0034"
#define ICON_FRAME			 "\u0027"
#define ICON_FRAME_DOTTED	 "\u0029"
#define ICON_MAGNET			 "\u0024"
#define ICON_MAGNET_OFF		 "\u0028"
#define ICON_SCROLL			 "\u0036"
#define ICON_CUBE			 "\u0039"
#define ICON_PEN			 "\u0065"
#define ICON_TRIANGLE		 "\u0032"
#define ICON_GLOBE			 "\u005A"
#define ICON_GLASSES		 "\u005E"
#define ICON_FOLDER			 "\u0061"
#define ICON_ARROW_RIGHT	 "\u0071"
#define ICON_DD_RIGHT		 "\u0046"
#define ICON_DD_DOWN		 "\u0048"
#define ICON_CHECK			 "\u0042"
#define ICON_CROSS			 "\u0057"
#define ICON_HAMMER			 "\u0035"
#define ICON_EYE			 "\u0025"
#define ICON_EYE_CROSS		 "\u0026"
#define ICON_FILE			 "\u0067"
#define ICON_PLAY			 "\u002C"
#define ICON_PLAY_STEP		 "\u002E"
#define ICON_PAUSE			 "\u002F"
#define ICON_INFO			 "\u004F"
#define ICON_WARN			 "\u004E"
#define ICON_ERROR			 "\u006E"
#define ICON_TRACE			 "\u004D"
#define ICON_AMBIENT		 "\u0032"
#define ICON_ANIMATION		 "\u0033"
#define ICON_AUDIO			 "\u0037"
#define ICON_AUDIO_MUTE		 "\u0077"
#define ICON_CAMERA			 "\u0038"
#define ICON_SPOT			 "\u003A"
#define ICON_LIGHT_BULB		 "\u003B"
#define ICON_SUN			 "\u003C"
#define ICON_MESH			 "\u003D"
#define ICON_EXPLOSION		 "\u003E"
#define ICON_CUBES			 "\u003F"
#define ICON_MOVE			 "\u0022"
#define ICON_SCALE			 "\u0023"
#define ICON_L				 "\u0075"
#define ICON_ROTATE			 "\u002B"
#define ICON_SETTINGS		 "\u002D"
#define ICON_RESET			 ICON_ROTATE
#define ICON_PLUS			 "\u0069"
#define ICON_MINUS			 "\u0058"
#define ICON_WORLD			 "\u005A"
#define ICON_FILTER			 "\u006D"
#define ICON_FILLED_CIRCLE	 "\u0043"
#define ICON_STAR			 "\u004B"
#define ICON_TRASH			 "\u006A"
#define ICON_IMPORT			 "\u006F"
#define ICON_GAUGE			 "\u0036"
#define ICON_WINDOW_MINIMIZE ICON_MINUS
#define ICON_WINDOW_MAXIMIZE "\u0054"
#define ICON_WINDOW_CLOSE	 ICON_CROSS

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_icon_widgets_t final
	{
	public:
		static ui::widget_id_t add_icon(ui::ui_context& ui, ui::widget_id_t parent, const char* icon, f32 point_size, const vec4f_t& color);
		static ui::widget_id_t add_sub_item_icon(ui::ui_context& ui, ui::widget_id_t parent);
		static ui::widget_id_t add_naked_icon_button(ui::ui_context& ui, ui::widget_id_t parent, const char* icon, f32 size, const vec4f_t& color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& disabled_color);
	};
}
