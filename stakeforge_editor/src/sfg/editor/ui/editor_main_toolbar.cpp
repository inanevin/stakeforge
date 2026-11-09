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

#include "editor/ui/editor_main_toolbar.hpp"
#include "editor/editor_world_controller.hpp"
#include "editor/ui/editor_text_rasterization.hpp"
#include "editor/ui/panels/editor_theme.hpp"
#include "editor/ui/widgets/editor_widgets_icons.hpp"
#include "editor/world/editor_world.hpp"

#include <sfg/core/runtime/ui/ui_context.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_main_toolbar_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui						= &ui;
		_pending_world_view		= editor_world_view_e::lit;
		_displayed_state		= editor_play_state_e::not_playing;
		_displayed_paused		= false;
		_has_pending_world_view = false;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_main_toolbar");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_value.y		 = 0.5f;
		root_in.child_margins	 = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};
		root_in.anchor_y		 = ui::anchor_e::center;

		_play_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_play_frame, "editor_main_toolbar_play_frame");
		tree.attach(_root, _play_frame);

		ui::layout_in_t& play_frame_in = tree.in(_play_frame);
		play_frame_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		play_frame_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		play_frame_in.pos_value		   = {0.5f, 0.5f};
		play_frame_in.anchor_x		   = ui::anchor_e::center;
		play_frame_in.anchor_y		   = ui::anchor_e::center;
		play_frame_in.size_mode_x	   = ui::axis_mode_e::sum_children;
		play_frame_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		play_frame_in.size_value.y	   = 1.0f;
		play_frame_in.child_spacing	   = theme.item_spacing;
		play_frame_in.child_margins	   = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		play_frame_in.flow			   = ui::flow_e::row;

		ui::vg_rect_paint_t play_rect = {
			.fill_color_a = theme.color_frame_dark,
			.fill_color_b = theme.color_frame_dark,
		};
		play_rect.rounding = theme.item_rounding * 2;
		paint.set_rect(_play_frame, play_rect);

		const float button_size = theme.item_area_height;

		_play_button.init(ui,
						  _play_frame,
						  {
							  .frame_color		   = {},
							  .toggled_frame_color = theme.color_accent2_dim,
							  .hover_color		   = theme.color_panel_light1,
							  .toggled_hover_color = theme.color_accent2_dim,
							  .press_color		   = theme.color_frame_light,
							  .icon_color		   = theme.color_accent2,
							  .disabled_color	   = theme.color_text_disabled,
							  .icon				   = ICON_PLAY,
							  .toggled_icon		   = ICON_PAUSE,
							  .tooltip			   = "Play",
							  .on_clicked		   = on_play_toggled,
							  .user_data		   = this,
							  .size				   = button_size,
							  .icon_size		   = theme.text_big_px_size,
							  .rounding			   = theme.item_rounding,
							  .toggle_enabled	   = true,
						  });

		_step_button.init(ui,
						  _play_frame,
						  {
							  .frame_color	  = {},
							  .hover_color	  = theme.color_panel_light1,
							  .press_color	  = theme.color_frame_light,
							  .icon_color	  = theme.color_text0,
							  .disabled_color = theme.color_text_disabled,
							  .icon			  = ICON_PLAY_STEP,
							  .tooltip		  = "Step Forward",
							  .on_clicked	  = on_step_pressed,
							  .user_data	  = this,
							  .size			  = button_size,
							  .icon_size	  = theme.text_big_px_size,
							  .rounding		  = theme.item_rounding,
						  });

		tree.in(_play_button.get_root()).size_mode_x = ui::axis_mode_e::fixed;
		tree.in(_step_button.get_root()).size_mode_x = ui::axis_mode_e::fixed;

		_edit_mode = editor_world_controller_t::get().get_play_session().is_edit_mode_enabled();
		init_play_option("Edit Mode", _edit_mode_checkbox, _edit_mode, {.edited = on_edit_mode_changed, .user_data = this});

		refresh_play_controls();
		ui.set_pre_layout_tick(_play_frame, on_play_controls_tick, this);
	}

	void editor_main_toolbar_t::init_play_option(const char* label, editor_checkbox_t& checkbox, bool& value, const editor_widget_callbacks_t& callbacks)
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();
		const ui::widget_id_t frame = ui.allocate_widget();

		ui.set_widget_debug_name(frame, label);
		tree.attach(_play_frame, frame);

		ui::layout_in_t& frame_in = tree.in(frame);
		frame_in.size_mode_x	  = ui::axis_mode_e::sum_children;
		frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		frame_in.size_value.y	  = 1.0f;
		frame_in.flow			  = ui::flow_e::row;
		frame_in.child_spacing	  = theme.item_spacing;
		frame_in.child_margins	  = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		const ui::widget_id_t text = ui.allocate_widget();
		tree.attach(frame, text);
		ui.set_widget_text(text, label);

		ui::layout_in_t& text_in = tree.in(text);
		text_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		text_in.pos_value.y		 = 0.5f;
		text_in.anchor_y		 = ui::anchor_e::center;
		text_in.size_mode_x		 = ui::axis_mode_e::fixed;
		text_in.size_mode_y		 = ui::axis_mode_e::fixed;
		text_in.size_value		 = {static_cast<f32>(ui.widget_text_len(text)) * theme.text_default_px_size * 0.7f, theme.text_default_px_size};
		ui.get_paint().set_text(
			text, ui.widget_text(text), ui.widget_text_len(text), {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		u8* field = reinterpret_cast<u8*>(&value);

		checkbox.init(ui,
					  frame,
					  {
						  .field	 = {.fields = {.data = &field, .size = 1}, .field_size = sizeof(value)},
						  .callbacks = callbacks,
					  });

		ui::layout_in_t& checkbox_in = tree.in(checkbox.get_root());
		checkbox_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		checkbox_in.pos_value.y		 = 0.5f;
		checkbox_in.anchor_y		 = ui::anchor_e::center;
	}

	void editor_main_toolbar_t::uninit()
	{
		_edit_mode_checkbox.uninit();
		_step_button.uninit();
		_play_button.uninit();
		_ui->deallocate_widget(_root);

		_ui						= nullptr;
		_root					= NULL_WIDGET;
		_play_frame				= NULL_WIDGET;
		_pending_world_view		= editor_world_view_e::lit;
		_displayed_state		= editor_play_state_e::not_playing;
		_displayed_paused		= false;
		_has_pending_world_view = false;
	}

	void editor_main_toolbar_t::serialize(nlohmann::json& j) const
	{
		j				= nlohmann::json::object();
		j["world_view"] = get_world_view();
	}

	void editor_main_toolbar_t::deserialize(const nlohmann::json& j)
	{
		_pending_world_view = j.value<editor_world_view_e>("world_view", editor_world_view_e::lit);

		if (_pending_world_view == editor_world_view_e::invalid)
			_pending_world_view = editor_world_view_e::lit;

		editor_world_controller_t&	world_controller = editor_world_controller_t::get();
		const editor_world_handle_t main_world		 = world_controller.get_main_world_handle();

		if (main_world.is_null())
			_has_pending_world_view = true;
		else
		{
			world_controller.get_editor_world(main_world)->get_edit_context().set_world_view(_pending_world_view);
			_has_pending_world_view = false;
		}
	}

	editor_world_view_e editor_main_toolbar_t::get_world_view() const
	{
		const editor_world_controller_t& world_controller = editor_world_controller_t::get();
		const editor_world_handle_t		 main_world		  = world_controller.get_main_world_handle();

		if (main_world.is_null())
			return _pending_world_view;

		return world_controller.get_editor_world(main_world)->get_edit_context().get_world_view();
	}

	bool editor_main_toolbar_t::is_window_drag_region(const vec2f_t& pos) const
	{
		const ui::layout_tree_t& tree		   = _ui->get_tree();
		const vec4f_t			 play_frame	   = tree.bounds(_play_frame);
		const bool				 in_play_frame = pos.x >= play_frame.x && pos.x <= play_frame.x + play_frame.z && pos.y >= play_frame.y && pos.y <= play_frame.y + play_frame.w;

		return !in_play_frame;
	}

	void editor_main_toolbar_t::refresh_play_controls()
	{
		const editor_world_controller_t& world_controller = editor_world_controller_t::get();
		const editor_world_handle_t		 main_world		  = world_controller.get_main_world_handle();
		const editor_play_session_t&	 session		  = world_controller.get_play_session();
		const editor_play_state_e		 state			  = session.get_requested_state();

		_play_button.set_toggled(state != editor_play_state_e::not_playing);
		_play_button.set_disabled(main_world.is_null());
		_step_button.set_disabled(!session.is_playing() || !session.is_paused());

		_displayed_state  = state;
		_displayed_paused = session.is_paused();
	}

	void editor_main_toolbar_t::on_play_toggled(bool toggled, void* user_data)
	{
		editor_main_toolbar_t&		toolbar			 = *static_cast<editor_main_toolbar_t*>(user_data);
		editor_world_controller_t&	world_controller = editor_world_controller_t::get();
		const editor_world_handle_t main_world		 = world_controller.get_main_world_handle();

		SFG_ASSERT(!main_world.is_null());
		world_controller.get_play_session().request_play(toggled);
		toolbar.refresh_play_controls();
	}

	void editor_main_toolbar_t::on_edit_mode_changed(void* user_data)
	{
		editor_main_toolbar_t& toolbar = *static_cast<editor_main_toolbar_t*>(user_data);

		editor_world_controller_t::get().get_play_session().set_edit_mode(toolbar._edit_mode);
		toolbar.refresh_play_controls();
	}

	void editor_main_toolbar_t::on_step_pressed(bool toggled, void* user_data)
	{
		editor_world_controller_t&	world_controller = editor_world_controller_t::get();
		const editor_world_handle_t main_world		 = world_controller.get_main_world_handle();

		SFG_ASSERT(!main_world.is_null());
		world_controller.get_play_session().request_step();
	}

	void editor_main_toolbar_t::on_play_controls_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_main_toolbar_t&		toolbar			 = *static_cast<editor_main_toolbar_t*>(user_data);
		editor_world_controller_t&	world_controller = editor_world_controller_t::get();
		const editor_world_handle_t main_world		 = world_controller.get_main_world_handle();

		const editor_play_session_t& session = world_controller.get_play_session();

		if (toolbar._edit_mode != session.is_edit_mode_enabled())
		{
			toolbar._edit_mode = session.is_edit_mode_enabled();
			toolbar._edit_mode_checkbox.refresh_field_data();
		}

		if (toolbar._displayed_state != session.get_requested_state() || toolbar._displayed_paused != session.is_paused() || ui.get_tree().is_disabled(toolbar._play_button.get_root()) != main_world.is_null())
			toolbar.refresh_play_controls();

		if (main_world.is_null())
			return;

		editor_world_edit_context_t& context = world_controller.get_editor_world(main_world)->get_edit_context();

		if (toolbar._has_pending_world_view)
		{
			context.set_world_view(toolbar._pending_world_view);
			toolbar._has_pending_world_view = false;
		}
	}

	void to_json(nlohmann::json& j, const editor_world_view_e& view)
	{
		switch (view)
		{
		case editor_world_view_e::lit:
			j = "lit";
			break;
		case editor_world_view_e::unlit:
			j = "unlit";
			break;
		default:
			j = "invalid";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_world_view_e& view)
	{
		const string_t s = j.get<string_t>();

		if (s == "lit" || s == "final")
			view = editor_world_view_e::lit;
		else if (s == "unlit" || s == "gbuffer_albedo")
			view = editor_world_view_e::unlit;
		else
			view = editor_world_view_e::invalid;
	}

	void to_json(nlohmann::json& j, const editor_main_toolbar_t& toolbar)
	{
		toolbar.serialize(j);
	}

	void from_json(const nlohmann::json& j, editor_main_toolbar_t& toolbar)
	{
		toolbar.deserialize(j);
	}
}
