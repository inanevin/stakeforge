// Copyright (c) 2025 Inan Evin
#pragma once

#define ICON_ARROW_RIGHT	 "\u0071"
#define ICON_DD_RIGHT		 "\u0046"
#define ICON_DD_DOWN		 "\u0048"
#define ICON_CHECK			 "\u0042"
#define ICON_CROSS			 "\u0057"
#define ICON_HAMMER			 "\u0035"
#define ICON_EYE			 "\u0025"
#define ICON_FILE			 "\u0067"
#define ICON_PLAY			 "\u002C"
#define ICON_INFO			 "\u004D"
#define ICON_AMBIENT		 "\u0032"
#define ICON_ANIMATION		 "\u0033"
#define ICON_AUDIO			 "\u0037"
#define ICON_AUDIO_MUTE		 "\u0077"
#define ICON_CAMERA			 "\u0038"
#define ICON_GUI			 "\u0039"
#define ICON_SPOT			 "\u003A"
#define ICON_LIGHT_BULB		 "\u003B"
#define ICON_SUN			 "\u003C"
#define ICON_MESH			 "\u003D"
#define ICON_EXPLOSION		 "\u003E"
#define ICON_CUBES			 "\u003F"
#define ICON_MOVE			 "\u0022"
#define ICON_SCALE			 "\u0023"
#define ICON_ROTATE			 "\u002B"
#define ICON_WORLD			 "\u005A"
#define ICON_L				 "\u0075"
#define ICON_GLASSES		 "\u005E"
#define ICON_FILLED_CIRCLE	 "\u0043"
#define ICON_WINDOW_MINIMIZE "\u0058"
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
	};
}
