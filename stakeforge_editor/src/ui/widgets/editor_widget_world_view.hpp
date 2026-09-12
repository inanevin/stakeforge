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

#include "ui/widgets/editor_widget_world_view_toolbars.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	struct editor_payload_t;
	struct window_event_t;
	struct window_runtime_t;
	namespace ui
	{
		enum class mouse_button_e : u8;
		struct input_router_t;
		struct key_event_t;
		class paint_layer_t;
		class ui_context;
		class vg_canvas_t;
	}

	class editor_widget_world_view_t final
	{
	public:
		editor_widget_world_view_t()											 = default;
		~editor_widget_world_view_t()											 = default;
		editor_widget_world_view_t(const editor_widget_world_view_t&)			 = delete;
		editor_widget_world_view_t& operator=(const editor_widget_world_view_t&) = delete;

		void		init(ui::ui_context& ui, ui::widget_id_t parent);
		void		uninit();
		void		set_edit_world(editor_world_handle_t world);
		static bool on_window_event(window_runtime_t& runtime, const window_event_t& ev);
		static void reset_camera_input(window_runtime_t& runtime);
		vec2i16_t	get_center() const;

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline ui::widget_id_t get_view_widget() const
		{
			return _world_view;
		}

	private:
		void	clear_world();
		void	refresh_world_texture();
		void	request_world_resize(bool force);
		vec2f_t calculate_relative_position(const vec2f_t& position) const;

		static void on_world_view_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_world_view_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_world_view_release(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_world_view_hover_move(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_world_view_hover_exit(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_world_view_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_world_view_focus_lost(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_world_view_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_world_view_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static void draw_world_axes(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);

	private:
		editor_widget_world_view_toolbars_t _toolbars;
		ui::ui_context*						_ui					 = nullptr;
		editor_world_handle_t				_edit_world			 = {};
		vec2u16_t							_last_resize_request = vec2u16_t::zero;
		ui::widget_id_t						_root				 = NULL_WIDGET;
		ui::widget_id_t						_world_view			 = NULL_WIDGET;
		ui::widget_id_t						_world_axes			 = NULL_WIDGET;
		ui::widget_id_t						_empty_label		 = NULL_WIDGET;
		u8									_resize_ticks		 = 0;
	};
}
