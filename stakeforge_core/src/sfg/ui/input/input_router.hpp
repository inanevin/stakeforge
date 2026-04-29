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

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "data/hash_map.hpp"
#include "math/vec2f.hpp"
#include "ui/ui_common.hpp"

namespace sfg::ui
{
	enum class mouse_button_e : u8
	{
		left   = 0,
		right  = 1,
		middle = 2,
		count
	};

	enum class key_action_e : u8
	{
		press,
		release,
		repeat
	};

	struct key_event_t
	{
		u16			 key	   = 0;
		u16			 scan_code = 0;
		key_action_e action	   = key_action_e::press;
	};

	struct input_router_t;
	class layout_tree_t;

	using on_mouse_fn = void (*)(input_router_t& router, widget_id_t id, const vec2f_t& pos, mouse_button_e btn, void* user_data);
	using on_move_fn  = void (*)(input_router_t& router, widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
	using on_focus_fn = void (*)(input_router_t& router, widget_id_t id, bool from_nav, void* user_data);
	using on_key_fn	  = void (*)(input_router_t& router, widget_id_t id, const key_event_t& ev, void* user_data);
	using on_wheel_fn = void (*)(input_router_t& router, widget_id_t id, f32 delta, void* user_data);

	struct listener_bundle_t
	{
		on_mouse_fn on_press		= nullptr;
		on_mouse_fn on_release		= nullptr;
		on_mouse_fn on_click		= nullptr;
		on_mouse_fn on_double_click = nullptr;
		on_move_fn	on_hover_enter	= nullptr;
		on_move_fn	on_hover_exit	= nullptr;
		on_move_fn	on_hover_move	= nullptr;
		on_move_fn	on_drag_begin	= nullptr;
		on_move_fn	on_drag			= nullptr;
		on_move_fn	on_drag_end		= nullptr;
		on_focus_fn on_focus_gain	= nullptr;
		on_focus_fn on_focus_lose	= nullptr;
		on_key_fn	on_key			= nullptr;
		on_wheel_fn on_wheel		= nullptr;
		void*		user_data		= nullptr;
	};

	struct input_config_t
	{
		f32 click_max_seconds		 = 0.25f;
		f32 double_click_max_seconds = 0.4f;
		f32 drag_threshold_pixels	 = 3.0f;
	};

	class input_router_t
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const input_config_t& cfg = {});
		void uninit();
		void tick(const layout_tree_t& tree, f32 dt_seconds);

		// -----------------------------------------------------------------------------
		// listener
		// -----------------------------------------------------------------------------

		void set_listener(widget_id_t id, const listener_bundle_t& b);
		void clear_listener(widget_id_t id);

		// -----------------------------------------------------------------------------
		// events
		// -----------------------------------------------------------------------------

		void on_mouse_move(const vec2f_t& pos);
		void on_mouse_button(mouse_button_e btn, bool pressed);
		void on_wheel(f32 delta);
		void on_key(const key_event_t& ev);
		void next_focus();
		void prev_focus();
		void set_focus(widget_id_t id, bool from_nav);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline widget_id_t get_hovered() const
		{
			return _hovered;
		}
		inline widget_id_t get_focused() const
		{
			return _focused;
		}
		inline widget_id_t is_pressed(mouse_button_e b) const
		{
			return _pressed[static_cast<u32>(b)];
		}
		inline const vec2f_t& get_mouse_position() const
		{
			return _mouse;
		}

	private:
		void		rebuild_hit_test(const layout_tree_t& tree);
		widget_id_t hit_test(const vec2f_t& pos) const;
		void		fire_hover_change(widget_id_t new_hover);

	private:
		struct press_state_t
		{
			widget_id_t target		 = INVALID_WIDGET;
			vec2f_t		press_pos	 = {0, 0};
			f32			held_seconds = 0.0f;
			bool		dragging	 = false;
		};

		struct click_record_t
		{
			widget_id_t target	  = INVALID_WIDGET;
			f32			t_seconds = 0.0f;
		};

	private:

		hash_map_t<widget_id_t, listener_bundle_t> _listeners;
		vector_t<widget_id_t>					   _focus_order;
		vector_t<widget_id_t>					   _hit_order;
		input_config_t							   _config = {};
		const layout_tree_t*					   _tree   = nullptr;

		press_state_t  _pressed_state[static_cast<u32>(mouse_button_e::count)] = {};
		widget_id_t	   _pressed[static_cast<u32>(mouse_button_e::count)]	   = {INVALID_WIDGET, INVALID_WIDGET, INVALID_WIDGET};
		click_record_t _last_click[static_cast<u32>(mouse_button_e::count)]	   = {};

		vec2f_t		_mouse		= {0, 0};
		vec2f_t		_mouse_prev = {0, 0};
		f32			_accum_time = 0.0f;
		widget_id_t _hovered	= INVALID_WIDGET;
		widget_id_t _focused	= INVALID_WIDGET;
	};
}
