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

#include "script_api_canvas.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	canvas_widget_handle_t api_canvas_create_frame(world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, const ui::vg_rect_paint_t* style)
	{
		SFG_ASSERT(world != nullptr && layout != nullptr && style != nullptr);

		return world->get_canvas_controller().create_frame(canvas, parent, *layout, *style);
	}

	canvas_widget_handle_t api_canvas_create_text(world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, const char* text, const ui::vg_text_style_t* style)
	{
		SFG_ASSERT(world != nullptr && layout != nullptr && text != nullptr && style != nullptr);

		return world->get_canvas_controller().create_text(canvas, parent, *layout, text, *style);
	}

	canvas_widget_handle_t api_canvas_create_image(world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, resource_handle_t texture, const vec4f_t* tint)
	{
		SFG_ASSERT(world != nullptr && layout != nullptr && tint != nullptr);

		return world->get_canvas_controller().create_image(canvas, parent, *layout, texture, *tint);
	}

	canvas_widget_handle_t api_canvas_create_button(
		world_t* world, entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t* layout, const char* text, const ui::vg_rect_paint_t* frame_style, const ui::vg_text_style_t* text_style, const vec4f_t* hover_color, const vec4f_t* press_color)
	{
		SFG_ASSERT(world != nullptr && layout != nullptr && text != nullptr && frame_style != nullptr && text_style != nullptr && hover_color != nullptr && press_color != nullptr);

		return world->get_canvas_controller().create_button(canvas, parent, *layout, text, *frame_style, *text_style, *hover_color, *press_color);
	}

	u8 api_canvas_destroy_widget(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_canvas_controller().destroy_widget(canvas, widget) ? 1 : 0;
	}

	u8 api_canvas_clear_widgets(world_t* world, entity_id_t canvas)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_canvas_controller().clear_widgets(canvas) ? 1 : 0;
	}

	u8 api_canvas_set_layout(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const ui::layout_in_t* layout)
	{
		SFG_ASSERT(world != nullptr && layout != nullptr);

		return world->get_canvas_controller().set_layout(canvas, widget, *layout) ? 1 : 0;
	}

	u8 api_canvas_set_visible(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, u8 visible)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_canvas_controller().set_visible(canvas, widget, visible != 0) ? 1 : 0;
	}

	u8 api_canvas_set_enabled(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, u8 enabled)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_canvas_controller().set_enabled(canvas, widget, enabled != 0) ? 1 : 0;
	}

	u8 api_canvas_set_text(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const char* text)
	{
		SFG_ASSERT(world != nullptr && text != nullptr);

		return world->get_canvas_controller().set_text(canvas, widget, text) ? 1 : 0;
	}

	u8 api_canvas_set_frame_style(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_rect_paint_t* style)
	{
		SFG_ASSERT(world != nullptr && style != nullptr);

		return world->get_canvas_controller().set_frame_style(canvas, widget, *style) ? 1 : 0;
	}

	u8 api_canvas_set_text_style(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_text_style_t* style)
	{
		SFG_ASSERT(world != nullptr && style != nullptr);

		return world->get_canvas_controller().set_text_style(canvas, widget, *style) ? 1 : 0;
	}

	u8 api_canvas_set_image(world_t* world, entity_id_t canvas, canvas_widget_handle_t widget, resource_handle_t texture, const vec4f_t* tint)
	{
		SFG_ASSERT(world != nullptr && tint != nullptr);

		return world->get_canvas_controller().set_image(canvas, widget, texture, *tint) ? 1 : 0;
	}

	const script_api_canvas_t& get_script_api_canvas()
	{
		static const script_api_canvas_t api{
			.size			 = static_cast<u32>(sizeof(script_api_canvas_t)),
			.version		 = 1,
			.create_frame	 = api_canvas_create_frame,
			.create_text	 = api_canvas_create_text,
			.create_image	 = api_canvas_create_image,
			.create_button	 = api_canvas_create_button,
			.destroy_widget	 = api_canvas_destroy_widget,
			.clear_widgets	 = api_canvas_clear_widgets,
			.set_layout		 = api_canvas_set_layout,
			.set_visible	 = api_canvas_set_visible,
			.set_enabled	 = api_canvas_set_enabled,
			.set_text		 = api_canvas_set_text,
			.set_frame_style = api_canvas_set_frame_style,
			.set_text_style	 = api_canvas_set_text_style,
			.set_image		 = api_canvas_set_image,
		};

		return api;
	}
}
