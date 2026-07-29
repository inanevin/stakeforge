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
