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

#include "ui/widgets/editor_widgets_common.hpp"
#include <sfg/data/span.hpp>
#include <sfg/runtime/resources/curve_def.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct rectf_t;

	class editor_widget_curve_edit_t final
	{
	public:
		editor_widget_curve_edit_t()											 = default;
		~editor_widget_curve_edit_t()											 = default;
		editor_widget_curve_edit_t(const editor_widget_curve_edit_t&)			 = delete;
		editor_widget_curve_edit_t& operator=(const editor_widget_curve_edit_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, span_t<curve_def_t> curves, const editor_widget_callbacks_t& callbacks);
		void uninit();
		void set_curves(span_t<curve_def_t> curves);

	private:
		void	copy_primary_keys();
		u32		get_channel_count() const;
		rectf_t get_plot_rect() const;
		bool	find_key(const vec2f_t& position, f32 radius, u32& out_key, u32& out_channel) const;
		void	apply_position(const vec2f_t& position);

		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_double_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context*			  _ui				= nullptr;
		span_t<curve_def_t>		  _curves			= {};
		editor_widget_callbacks_t _callbacks		= {};
		ui::widget_id_t			  _root				= NULL_WIDGET;
		u32						  _selected_key		= UINT32_MAX;
		u32						  _selected_channel = 0;
		bool					  _dragging			= false;
	};
}
