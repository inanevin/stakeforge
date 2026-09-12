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

#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/runtime/world/world_canvas_controller.hpp>

namespace sfg
{
	class world_t;

	static_assert(sizeof(ui::layout_in_t) == 56);
	static_assert(sizeof(ui::vg_rect_paint_t) == 64);
	static_assert(sizeof(ui::vg_text_style_t) == 32);
	static_assert(sizeof(canvas_event_t) == 36);

	canvas_widget_handle_t api_canvas_create_frame(world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, const ui::vg_rect_paint_t* style);
	canvas_widget_handle_t api_canvas_create_text(world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, const char* text, const ui::vg_text_style_t* style);
	canvas_widget_handle_t api_canvas_create_image(world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, resource_handle_t texture, const vec4f_t* tint);
	canvas_widget_handle_t api_canvas_create_button(
		world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, const char* text, const ui::vg_rect_paint_t* frame_style, const ui::vg_text_style_t* text_style, const vec4f_t* hover_color, const vec4f_t* press_color);
	u8 api_canvas_destroy_widget(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget);
	u8 api_canvas_clear_widgets(world_t* world, entity_id_t canvas);
	u8 api_canvas_set_layout(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const ui::layout_in_t* layout);
	u8 api_canvas_set_visible(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, u8 visible);
	u8 api_canvas_set_enabled(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, u8 enabled);
	u8 api_canvas_set_text(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const char* text);
	u8 api_canvas_set_frame_style(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_rect_paint_t* style);
	u8 api_canvas_set_text_style(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_text_style_t* style);
	u8 api_canvas_set_image(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, resource_handle_t texture, const vec4f_t* tint);

	struct script_api_canvas_t
	{
		u32									  size			  = 0;
		u32									  version		  = 0;
		decltype(&api_canvas_create_frame)	  create_frame	  = nullptr;
		decltype(&api_canvas_create_text)	  create_text	  = nullptr;
		decltype(&api_canvas_create_image)	  create_image	  = nullptr;
		decltype(&api_canvas_create_button)	  create_button	  = nullptr;
		decltype(&api_canvas_destroy_widget)  destroy_widget  = nullptr;
		decltype(&api_canvas_clear_widgets)	  clear_widgets	  = nullptr;
		decltype(&api_canvas_set_layout)	  set_layout	  = nullptr;
		decltype(&api_canvas_set_visible)	  set_visible	  = nullptr;
		decltype(&api_canvas_set_enabled)	  set_enabled	  = nullptr;
		decltype(&api_canvas_set_text)		  set_text		  = nullptr;
		decltype(&api_canvas_set_frame_style) set_frame_style = nullptr;
		decltype(&api_canvas_set_text_style)  set_text_style  = nullptr;
		decltype(&api_canvas_set_image)		  set_image		  = nullptr;
	};

	const script_api_canvas_t& get_script_api_canvas();
}
