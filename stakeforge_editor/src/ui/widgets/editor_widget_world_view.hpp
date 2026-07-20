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

#include "ui/widgets/editor_widget_world_view_toolbars.hpp"
#include "world/editor_world_camera.hpp"
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
		class ui_context;
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

	private:
		void	clear_world();
		void	refresh_world_texture();
		void	request_world_resize(bool force);
		void	begin_camera_control(window_runtime_t& runtime);
		void	end_camera_control();
		void	pass_camera_input(const editor_world_camera_input_t& input);
		bool	pass_camera_key(const window_event_t& ev);
		vec2f_t calculate_relative_position(const vec2f_t& position) const;
		void	cancel_gizmo_action();

		static void on_world_view_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_world_view_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_world_view_release(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_world_view_hover_move(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_world_view_hover_exit(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_world_view_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_world_view_focus_lost(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_world_view_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_world_view_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);

	private:
		editor_widget_world_view_toolbars_t _toolbars;
		ui::ui_context*						_ui						  = nullptr;
		editor_world_handle_t				_edit_world				  = {};
		vec2u16_t							_last_resize_request	  = vec2u16_t::zero;
		ui::widget_id_t						_root					  = NULL_WIDGET;
		ui::widget_id_t						_world_view				  = NULL_WIDGET;
		ui::widget_id_t						_empty_label			  = NULL_WIDGET;
		window_runtime_t*					_camera_runtime			  = nullptr;
		u8									_resize_ticks			  = 0;
		bool								_camera_control			  = false;
		bool								_gizmo_press_consumed	  = false;
		bool								_shoot_ray_press_consumed = false;

		static inline editor_widget_world_view_t* s_active_camera_view = nullptr;
		static inline window_runtime_t*			  s_event_runtime	   = nullptr;
	};
}
