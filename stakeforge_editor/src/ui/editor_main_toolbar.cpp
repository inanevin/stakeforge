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

#include "ui/editor_main_toolbar.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		const editor_dropdown_item_t WORLD_VIEW_ITEMS[] = {
			{.text = "Final", .value = static_cast<u16>(editor_main_toolbar_world_view_e::final)},
			{.text = "GBuffer Albedo", .value = static_cast<u16>(editor_main_toolbar_world_view_e::gbuffer_albedo)},
			{.text = "GBuffer ORM", .value = static_cast<u16>(editor_main_toolbar_world_view_e::gbuffer_orm)},
			{.text = "GBuffer Normal", .value = static_cast<u16>(editor_main_toolbar_world_view_e::gbuffer_normal)},
			{.text = "GBuffer Emissive", .value = static_cast<u16>(editor_main_toolbar_world_view_e::gbuffer_emissive)},
			{.text = "Lighting", .value = static_cast<u16>(editor_main_toolbar_world_view_e::lighting)},
			{.text = "Post Process", .value = static_cast<u16>(editor_main_toolbar_world_view_e::post_process)},
		};
	}

	void editor_main_toolbar_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
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

		_world_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_world_frame, "editor_main_toolbar_world_frame");
		tree.attach(_root, _world_frame);

		ui::layout_in_t& world_frame_in = tree.in(_world_frame);
		world_frame_in.size_mode_x		= ui::axis_mode_e::sum_children;
		world_frame_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		world_frame_in.size_value.y		= 1.0f;
		world_frame_in.flow				= ui::flow_e::row;
		world_frame_in.child_spacing	= theme.item_spacing;
		world_frame_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		_world_label = ui.allocate_widget();
		ui.set_widget_debug_name(_world_label, "world_view_label");
		tree.attach(_world_frame, _world_label);

		ui::layout_in_t& label_in = tree.in(_world_label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::parent_relative;

		ui.set_widget_text(_world_label, "World View:");
		label_in.size_value.x = static_cast<f32>(ui.widget_text_len(_world_label)) * theme.text_default_px_size * 0.7f;
		paint.set_text(_world_label,
					   ui.widget_text(_world_label),
					   ui.widget_text_len(_world_label),
					   {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		editor_dropdown_config_t dropdown_config = {};
		dropdown_config.items					 = WORLD_VIEW_ITEMS;
		dropdown_config.item_count				 = static_cast<u16>(sizeof(WORLD_VIEW_ITEMS) / sizeof(WORLD_VIEW_ITEMS[0]));
		dropdown_config.selected				 = get_selected_world_view;
		dropdown_config.pressed					 = on_world_view_pressed;
		dropdown_config.user_data				 = this;
		dropdown_config.width					 = editor_dropdown_width_e::fixed;
		dropdown_config.pos_y					 = editor_dropdown_pos_y_e::center;
		dropdown_config.fixed_width				 = theme.item_width * 1.25f;
		_world_view_dropdown.init(ui, _world_frame, dropdown_config);

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
							  .toggled_icon		   = ICON_PLAY,
							  .tooltip			   = "Play",
							  .on_clicked		   = on_play_toggled,
							  .user_data		   = this,
							  .size				   = button_size,
							  .icon_size		   = theme.text_big_px_size,
							  .rounding			   = theme.item_rounding,
							  .toggle_enabled	   = true,
						  });

		_play_physics_button.init(ui,
								  _play_frame,
								  {
									  .frame_color		   = {},
									  .toggled_frame_color = theme.color_accent1_dim,
									  .hover_color		   = theme.color_panel_light1,
									  .toggled_hover_color = theme.color_accent1_dim,
									  .press_color		   = theme.color_frame_light,
									  .icon_color		   = theme.color_accent1,
									  .disabled_color	   = theme.color_text_disabled,
									  .icon				   = ICON_PLAY,
									  .toggled_icon		   = ICON_PLAY,
									  .tooltip			   = "Play Physics",
									  .on_clicked		   = on_play_physics_toggled,
									  .user_data		   = this,
									  .size				   = button_size,
									  .icon_size		   = theme.text_big_px_size,
									  .rounding			   = theme.item_rounding,
									  .toggle_enabled	   = true,
								  });

		_pause_button.init(ui,
						   _play_frame,
						   {
							   .frame_color			= {},
							   .toggled_frame_color = theme.color_panel_light1,
							   .hover_color			= theme.color_panel_light1,
							   .toggled_hover_color = theme.color_light,
							   .press_color			= theme.color_frame_light,
							   .icon_color			= theme.color_text0,
							   .disabled_color		= theme.color_text_disabled,
							   .icon				= ICON_PAUSE,
							   .toggled_icon		= ICON_PAUSE,
							   .tooltip				= "Pause",
							   .on_clicked			= on_pause_toggled,
							   .user_data			= this,
							   .size				= button_size,
							   .icon_size			= theme.text_big_px_size,
							   .rounding			= theme.item_rounding,
							   .toggle_enabled		= true,
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
							  .tooltip		  = "Step",
							  .on_clicked	  = on_step_pressed,
							  .user_data	  = this,
							  .size			  = button_size,
							  .icon_size	  = theme.text_big_px_size,
							  .rounding		  = theme.item_rounding,
						  });

		refresh_play_controls();
		ui.set_pre_layout_tick(_play_frame, on_play_controls_tick, this);
	}

	void editor_main_toolbar_t::uninit()
	{
		_step_button.uninit();
		_pause_button.uninit();
		_play_physics_button.uninit();
		_play_button.uninit();
		_world_view_dropdown.uninit();
		_ui->deallocate_widget(_root);

		_ui				= nullptr;
		_root			= NULL_WIDGET;
		_world_frame	= NULL_WIDGET;
		_world_label	= NULL_WIDGET;
		_play_frame		= NULL_WIDGET;
		_displayed_mode = editor_play_mode_e::none;
	}

	void editor_main_toolbar_t::serialize(nlohmann::json& j) const
	{
		j				= nlohmann::json::object();
		j["world_view"] = editor_global_toolbar_t::get().get_world_view();
	}

	void editor_main_toolbar_t::deserialize(const nlohmann::json& j)
	{
		editor_main_toolbar_world_view_e world_view = j.value<editor_main_toolbar_world_view_e>("world_view", editor_main_toolbar_world_view_e::final);
		if (world_view == editor_main_toolbar_world_view_e::invalid)
			world_view = editor_main_toolbar_world_view_e::final;
		editor_global_toolbar_t::get().set_world_view(world_view);
		if (_ui != nullptr)
			_world_view_dropdown.refresh_title();
	}

	bool editor_main_toolbar_t::is_window_drag_region(const vec2f_t& pos) const
	{
		const ui::layout_tree_t& tree			= _ui->get_tree();
		const vec4f_t			 world_frame	= tree.bounds(_world_frame);
		const vec4f_t			 play_frame		= tree.bounds(_play_frame);
		const bool				 in_world_frame = pos.x >= world_frame.x && pos.x <= world_frame.x + world_frame.z && pos.y >= world_frame.y && pos.y <= world_frame.y + world_frame.w;
		const bool				 in_play_frame	= pos.x >= play_frame.x && pos.x <= play_frame.x + play_frame.z && pos.y >= play_frame.y && pos.y <= play_frame.y + play_frame.w;
		return !in_world_frame && !in_play_frame;
	}

	void editor_main_toolbar_t::refresh_play_controls()
	{
		editor_global_toolbar_t& toolbar		 = editor_global_toolbar_t::get();
		const editor_play_mode_e mode			 = toolbar.get_play_mode();
		const bool				 is_play		 = mode == editor_play_mode_e::play || mode == editor_play_mode_e::play_paused;
		const bool				 is_play_physics = mode == editor_play_mode_e::play_physics || mode == editor_play_mode_e::play_physics_paused;
		const bool				 is_paused		 = mode == editor_play_mode_e::play_paused || mode == editor_play_mode_e::play_physics_paused;

		_play_button.set_toggled(is_play);
		_play_button.set_disabled(is_play_physics);
		_play_physics_button.set_toggled(is_play_physics);
		_play_physics_button.set_disabled(is_play);
		_pause_button.set_toggled(is_paused);
		_pause_button.set_disabled(mode == editor_play_mode_e::none);
		_step_button.set_disabled(!is_paused);

		_displayed_mode = mode;
	}

	u16 editor_main_toolbar_t::get_selected_world_view(void*)
	{
		return static_cast<u16>(editor_global_toolbar_t::get().get_world_view());
	}

	void editor_main_toolbar_t::on_world_view_pressed(u16 value, void*)
	{
		editor_global_toolbar_t::get().set_world_view(static_cast<editor_main_toolbar_world_view_e>(value));
	}

	void editor_main_toolbar_t::on_play_toggled(bool toggled, void* user_data)
	{
		editor_main_toolbar_t& toolbar = *static_cast<editor_main_toolbar_t*>(user_data);
		editor_global_toolbar_t::get().set_play_mode(toggled ? editor_play_mode_e::play : editor_play_mode_e::none);
		toolbar.refresh_play_controls();
	}

	void editor_main_toolbar_t::on_play_physics_toggled(bool toggled, void* user_data)
	{
		editor_main_toolbar_t& toolbar = *static_cast<editor_main_toolbar_t*>(user_data);
		editor_global_toolbar_t::get().set_play_mode(toggled ? editor_play_mode_e::play_physics : editor_play_mode_e::none);
		toolbar.refresh_play_controls();
	}

	void editor_main_toolbar_t::on_pause_toggled(bool toggled, void* user_data)
	{
		editor_main_toolbar_t&	 toolbar = *static_cast<editor_main_toolbar_t*>(user_data);
		editor_global_toolbar_t& global	 = editor_global_toolbar_t::get();
		const editor_play_mode_e mode	 = global.get_play_mode();

		if (toggled)
		{
			SFG_ASSERT(mode == editor_play_mode_e::play || mode == editor_play_mode_e::play_physics);
			global.set_play_mode(mode == editor_play_mode_e::play ? editor_play_mode_e::play_paused : editor_play_mode_e::play_physics_paused);
		}
		else
		{
			SFG_ASSERT(mode == editor_play_mode_e::play_paused || mode == editor_play_mode_e::play_physics_paused);
			global.set_play_mode(mode == editor_play_mode_e::play_paused ? editor_play_mode_e::play : editor_play_mode_e::play_physics);
		}

		toolbar.refresh_play_controls();
	}

	void editor_main_toolbar_t::on_step_pressed(bool toggled, void* user_data)
	{
		editor_global_toolbar_t::get().set_do_step(true);
	}

	void editor_main_toolbar_t::on_play_controls_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_main_toolbar_t&		   toolbar = *static_cast<editor_main_toolbar_t*>(user_data);
		const editor_global_toolbar_t& global  = editor_global_toolbar_t::get();
		if (toolbar._displayed_mode != global.get_play_mode())
			toolbar.refresh_play_controls();
	}

	void to_json(nlohmann::json& j, const editor_main_toolbar_world_view_e& view)
	{
		switch (view)
		{
		case editor_main_toolbar_world_view_e::final:
			j = "final";
			break;
		case editor_main_toolbar_world_view_e::gbuffer_albedo:
			j = "gbuffer_albedo";
			break;
		case editor_main_toolbar_world_view_e::gbuffer_orm:
			j = "gbuffer_orm";
			break;
		case editor_main_toolbar_world_view_e::gbuffer_normal:
			j = "gbuffer_normal";
			break;
		case editor_main_toolbar_world_view_e::gbuffer_emissive:
			j = "gbuffer_emissive";
			break;
		case editor_main_toolbar_world_view_e::lighting:
			j = "lighting";
			break;
		case editor_main_toolbar_world_view_e::post_process:
			j = "post_process";
			break;
		default:
			j = "invalid";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_main_toolbar_world_view_e& view)
	{
		const string_t s = j.get<string_t>();
		if (s == "final")
			view = editor_main_toolbar_world_view_e::final;
		else if (s == "gbuffer_albedo")
			view = editor_main_toolbar_world_view_e::gbuffer_albedo;
		else if (s == "gbuffer_orm")
			view = editor_main_toolbar_world_view_e::gbuffer_orm;
		else if (s == "gbuffer_normal")
			view = editor_main_toolbar_world_view_e::gbuffer_normal;
		else if (s == "gbuffer_emissive")
			view = editor_main_toolbar_world_view_e::gbuffer_emissive;
		else if (s == "lighting")
			view = editor_main_toolbar_world_view_e::lighting;
		else if (s == "post_process")
			view = editor_main_toolbar_world_view_e::post_process;
		else
			view = editor_main_toolbar_world_view_e::invalid;
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
