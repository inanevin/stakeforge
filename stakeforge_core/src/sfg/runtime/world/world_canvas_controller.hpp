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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/unique.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;
	struct world_canvas_render_snapshot_t;

	namespace ui
	{
		struct layout_in_t;
		struct vg_rect_paint_t;
		struct vg_text_style_t;
	}

	using canvas_widget_handle_t = u32;
#define NULL_CANVAS_WIDGET_HANDLE 0

	enum class canvas_event_type_e : u8
	{
		press,
		release,
		click,
		double_click,
		hover_enter,
		hover_exit,
		hover_move,
		drag_begin,
		drag,
		drag_end,
		focus_gain,
		focus_lose,
		key,
		wheel,
	};

	struct canvas_event_t
	{
		vec2f_t				   position	   = vec2f_t::zero;
		vec2f_t				   delta	   = vec2f_t::zero;
		canvas_widget_handle_t widget	   = NULL_CANVAS_WIDGET_HANDLE;
		entity_id_t			   canvas	   = NULL_ENTITY_ID;
		f32					   wheel_delta = 0.0f;
		u16					   key		   = 0;
		u16					   scan_code   = 0;
		canvas_event_type_e	   type		   = canvas_event_type_e::click;
		u8					   button	   = UINT8_MAX;
		u8					   action	   = 0;
		u8					   from_nav	   = 0;
	};

	class world_canvas_controller_t final
	{
	public:
		world_canvas_controller_t();
		~world_canvas_controller_t();
		world_canvas_controller_t(const world_canvas_controller_t&)			   = delete;
		world_canvas_controller_t& operator=(const world_canvas_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world);
		void uninit();
		void clear();
		void clear_widgets();
		void destroy_entity(entity_id_t entity);
		void sync_create_destroy_canvases();
		void tick(f32 dt_seconds);

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void set_viewport(const vec4f_t& input_rect, vec2u16_t render_size, f32 dpi_scale);
		void write_render_snapshot(world_canvas_render_snapshot_t& snapshot) const;

		// -----------------------------------------------------------------------------
		// input
		// -----------------------------------------------------------------------------

		bool key_event(u16 key, u16 scan_code, u8 action);
		bool mouse_button_event(u8 button, u8 action, const vec2f_t& position);
		bool mouse_move_event(const vec2f_t& position);
		bool mouse_wheel_event(const vec2f_t& position, f32 delta);
		bool is_keyboard_focus_active() const;
		void dispatch_events(void* world_script_instance);

		// -----------------------------------------------------------------------------
		// widgets
		// -----------------------------------------------------------------------------

		canvas_widget_handle_t create_frame(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const ui::vg_rect_paint_t& style);
		canvas_widget_handle_t create_text(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_text_style_t& style);
		canvas_widget_handle_t create_image(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, resource_handle_t texture, const vec4f_t& tint);
		canvas_widget_handle_t create_button(
			entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_rect_paint_t& frame_style, const ui::vg_text_style_t& text_style, const vec4f_t& hover_color, const vec4f_t& press_color);
		bool destroy_widget(entity_id_t canvas, canvas_widget_handle_t widget);
		bool clear_widgets(entity_id_t canvas);
		bool set_layout(entity_id_t canvas, canvas_widget_handle_t widget, const ui::layout_in_t& layout);
		bool set_visible(entity_id_t canvas, canvas_widget_handle_t widget, bool visible);
		bool set_enabled(entity_id_t canvas, canvas_widget_handle_t widget, bool enabled);
		bool set_text(entity_id_t canvas, canvas_widget_handle_t widget, const char* text);
		bool set_frame_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_rect_paint_t& style);
		bool set_text_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_text_style_t& style);
		bool set_image(entity_id_t canvas, canvas_widget_handle_t widget, resource_handle_t texture, const vec4f_t& tint);

	private:
		struct impl_t;
		unique_t<impl_t> _impl;
	};
}
