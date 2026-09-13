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

#include "ui/widgets/editor_widget_button.hpp"
#include <sfg/data/unique.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/animation/common_animation.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class vg_canvas_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_panel_animation_library_t;
	class editor_input_field_t;

	class editor_widget_animation_library_states_t final
	{
	public:
		editor_widget_animation_library_states_t();
		~editor_widget_animation_library_states_t();
		editor_widget_animation_library_states_t(const editor_widget_animation_library_states_t& other)			   = delete;
		editor_widget_animation_library_states_t& operator=(const editor_widget_animation_library_states_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, editor_panel_animation_library_t& panel);
		void uninit();
		void reset();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh();
		void refresh_values();
		void refresh_selection();

	private:
		struct state_controls_t;
		struct clip_controls_t;
		struct state_ui_t
		{
			bool expanded										  = false;
			bool clip_expanded[MAX_ANIMATION_LIBRARY_STATE_CLIPS] = {};
		};

		void			clear_controls();
		void			create_state(u32 index);
		void			create_clip(state_controls_t& controls, u32 index);
		ui::widget_id_t init_number(ui::widget_id_t parent, editor_input_field_t& field, const char* label, f32& value, f32 minimum, f32 maximum);
		void			init_preview(state_controls_t& controls);

		static void on_edit_begin(void* user_data);
		static void on_edited(void* user_data);
		static void on_edit_submitted(void* user_data);
		static void on_blend_edited(void* user_data);
		static void on_state_fold_changed(bool folded, void* user_data);
		static void on_clip_fold_changed(bool folded, void* user_data);
		static void on_clip_pressed(void* user_data);
		static void on_clip_animation_edited(void* user_data);
		static void on_states_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_clips_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_diamond_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_diamond_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_diamond_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void draw_diamond(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		vector_t<unique_t<state_controls_t>> _controls								 = {};
		vector_t<state_ui_t>				 _state_ui[MAX_ANIMATION_LIBRARY_LAYERS] = {};
		editor_widget_button_t				 _add_state								 = {};
		editor_widget_button_t				 _clear_states							 = {};
		editor_panel_animation_library_t*	 _panel									 = nullptr;
		ui::ui_context*						 _ui									 = nullptr;
		u32									 _layer									 = UINT32_MAX;
		ui::widget_id_t						 _root									 = NULL_WIDGET;
		ui::widget_id_t						 _list									 = NULL_WIDGET;
	};
}
