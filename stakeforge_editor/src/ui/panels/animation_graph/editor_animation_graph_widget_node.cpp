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

#include "ui/panels/animation_graph/editor_animation_graph_widget_node.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	void editor_animation_graph_widget_node_t::init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui				   = &ui;
		_base_size		   = {theme.item_height * 8.0f, theme.item_height * 6.0f};
		_base_title_height = theme.item_height;
		_base_pin_size	   = theme.item_height * 0.5f;
		_id				   = config.id;
		_root			   = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "animation_graph_node");
		ui.get_tree().attach(parent, _root);

		ui::layout_in_t& in = ui.get_tree().in(_root);
		in.pos_mode_x		= ui::pos_mode_e::absolute_screen;
		in.pos_mode_y		= ui::pos_mode_e::absolute_screen;
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= _base_size;
		in.child_margins	= {theme.divider_thickness, theme.divider_thickness, theme.divider_thickness, theme.divider_thickness};
		in.flow				= ui::flow_e::column;

		set_selected(false);

		_title_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_title_frame, "animation_graph_node_title_frame");
		ui.get_tree().attach(_root, _title_frame);
		ui.get_tree().draw_order(_title_frame) = ui.get_tree().draw_order_const(_root) + 1;

		ui::layout_in_t& title_frame_in = ui.get_tree().in(_title_frame);
		title_frame_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		title_frame_in.size_mode_y		= ui::axis_mode_e::fixed;
		title_frame_in.size_value		= {1.0f, _base_title_height};

		ui.get_paint().set_rect(_title_frame,
								{
									.fill_color_a = theme.color_accent1_dim,
									.fill_color_b = theme.color_accent1_dim,
									.rounding	  = theme.item_rounding,
								});

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "animation_graph_node_title");
		ui.get_tree().attach(_title_frame, _title);
		ui.get_tree().draw_order(_title) = ui.get_tree().draw_order_const(_title_frame) + 1;

		ui::layout_in_t& title_in = ui.get_tree().in(_title);
		title_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		title_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		title_in.pos_value		  = {0.5f, 0.5f};
		title_in.anchor_x		  = ui::anchor_e::center;
		title_in.anchor_y		  = ui::anchor_e::center;

		_body_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_body_frame, "animation_graph_node_body_frame");
		ui.get_tree().attach(_root, _body_frame);

		ui::layout_in_t& body_frame_in = ui.get_tree().in(_body_frame);
		body_frame_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		body_frame_in.size_mode_y	   = ui::axis_mode_e::fill;
		body_frame_in.size_value	   = vec2f_t::one;
		body_frame_in.flow			   = ui::flow_e::column;
		body_frame_in.child_margins	   = {theme.margin_vertical * 2, 0.0f, theme.margin_vertical * 2, 0.0f};
		body_frame_in.child_spacing	   = theme.item_spacing;

		_entry_label = ui.allocate_widget();
		ui.set_widget_debug_name(_entry_label, "animation_graph_node_entry_label");
		ui.get_tree().attach(_body_frame, _entry_label);
		ui.get_tree().draw_order(_entry_label) = ui.get_tree().draw_order_const(_body_frame) + 1;

		ui::layout_in_t& entry_label_in = ui.get_tree().in(_entry_label);
		entry_label_in.pos_mode_x		= ui::pos_mode_e::offset_in_parent;
		entry_label_in.pos_value.x		= theme.margin_horizontal;
		entry_label_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		entry_label_in.size_mode_y		= ui::axis_mode_e::fixed;
		entry_label_in.size_value		= {1.0f, _base_title_height};

		ui.set_widget_text(_entry_label, "ENTRY");

		_exit_label = ui.allocate_widget();
		ui.set_widget_debug_name(_exit_label, "animation_graph_node_exit_label");
		ui.get_tree().attach(_body_frame, _exit_label);
		ui.get_tree().draw_order(_exit_label) = ui.get_tree().draw_order_const(_body_frame) + 1;

		ui::layout_in_t& exit_label_in = ui.get_tree().in(_exit_label);
		exit_label_in.pos_mode_x	   = ui::pos_mode_e::offset_in_parent;
		exit_label_in.pos_value.x	   = theme.margin_horizontal;
		exit_label_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		exit_label_in.size_mode_y	   = ui::axis_mode_e::fixed;
		exit_label_in.size_value	   = {1.0f, _base_title_height};

		ui.set_widget_text(_exit_label, "EXIT");

		ui.get_tree().set_visible(_entry_label, false, false);
		ui.get_tree().set_visible(_exit_label, false, false);

		_pin_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_pin_frame, "animation_graph_node_pin_frame");
		ui.get_tree().attach(_body_frame, _pin_frame);

		ui::layout_in_t& pin_frame_in = ui.get_tree().in(_pin_frame);
		pin_frame_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		pin_frame_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		pin_frame_in.pos_value		  = {1.0f, 0.5f};
		pin_frame_in.anchor_x		  = ui::anchor_e::center;
		pin_frame_in.anchor_y		  = ui::anchor_e::center;
		pin_frame_in.size_mode_x	  = ui::axis_mode_e::fixed;
		pin_frame_in.size_mode_y	  = ui::axis_mode_e::fixed;
		pin_frame_in.size_value		  = {_base_pin_size, _base_pin_size};

		ui.get_paint().set_custom(_pin_frame, draw_pin, this);

		update_title(config.title);
		update_status_paint();
	}

	void editor_animation_graph_widget_node_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui				   = nullptr;
		_base_size		   = vec2f_t::zero;
		_root			   = NULL_WIDGET;
		_title_frame	   = NULL_WIDGET;
		_title			   = NULL_WIDGET;
		_body_frame		   = NULL_WIDGET;
		_entry_label	   = NULL_WIDGET;
		_exit_label		   = NULL_WIDGET;
		_pin_frame		   = NULL_WIDGET;
		_base_title_height = 0.0f;
		_base_pin_size	   = 0.0f;
		_zoom			   = 1.0f;
		_id				   = ANIMATION_GRAPH_DEF_NULL_ID;
		_is_entry		   = false;
		_is_exit		   = false;
		_is_start_state	   = false;
	}

	void editor_animation_graph_widget_node_t::set_zoom(f32 zoom)
	{
		if (_zoom == zoom)
			return;

		const editor_theme_t& theme = editor_theme_t::get();

		_zoom = zoom;

		_ui->get_tree().in(_root).size_value		  = _base_size * zoom;
		_ui->get_tree().in(_title_frame).size_value.y = _base_title_height * zoom;
		_ui->get_tree().in(_entry_label).size_value.y = _base_title_height * zoom;
		_ui->get_tree().in(_entry_label).pos_value.x  = theme.margin_horizontal * zoom;
		_ui->get_tree().in(_exit_label).size_value.y  = _base_title_height * zoom;
		_ui->get_tree().in(_exit_label).pos_value.x	  = theme.margin_horizontal * zoom;
		_ui->get_tree().in(_pin_frame).size_value	  = {_base_pin_size * zoom, _base_pin_size * zoom};
		_ui->get_tree().in(_body_frame).child_margins = {theme.margin_vertical * 2 * zoom, 0.0f, theme.margin_vertical * 2 * zoom, 0.0f};
		_ui->get_tree().in(_body_frame).child_spacing = theme.item_spacing * zoom;

		update_title_paint();
		update_status_paint();
	}

	void editor_animation_graph_widget_node_t::set_selected(bool selected)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->get_paint().set_rect(_root,
								  {
									  .fill_color_a		 = theme.color_panel,
									  .fill_color_b		 = theme.color_panel,
									  .outline_color	 = selected ? theme.color_accent1 : theme.color_accent0_dim,
									  .rounding			 = theme.item_rounding,
									  .outline_thickness = theme.divider_thickness * 2,
								  });
	}

	void editor_animation_graph_widget_node_t::set_start_state(bool start_state)
	{
		if (_is_start_state == start_state)
			return;

		_is_start_state				= start_state;
		const editor_theme_t& theme = editor_theme_t::get();
		const vec4f_t&		  color = start_state ? theme.color_accent_green_dim : theme.color_accent1_dim;

		_ui->get_paint().set_rect(_title_frame,
								  {
									  .fill_color_a = color,
									  .fill_color_b = color,
									  .rounding		= theme.item_rounding,
								  });
	}

	void editor_animation_graph_widget_node_t::update_title(const char* title)
	{
		_ui->set_widget_text(_title, title != nullptr ? title : "");
		update_title_paint();
	}

	void editor_animation_graph_widget_node_t::make_entry()
	{
		set_entry_and_exit(true, false);
	}

	void editor_animation_graph_widget_node_t::make_exit()
	{
		set_entry_and_exit(false, true);
	}

	void editor_animation_graph_widget_node_t::make_entry_and_exit()
	{
		set_entry_and_exit(true, true);
	}

	void editor_animation_graph_widget_node_t::clear_entry_and_exit()
	{
		set_entry_and_exit(false, false);
	}

	void editor_animation_graph_widget_node_t::set_entry_and_exit(bool entry, bool exit)
	{
		if (_is_entry == entry && _is_exit == exit)
			return;

		_is_entry = entry;
		_is_exit  = exit;

		_ui->get_tree().set_visible(_entry_label, entry, false);
		_ui->get_tree().set_visible(_exit_label, exit, false);
	}

	void editor_animation_graph_widget_node_t::update_title_paint()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->get_paint().set_text(_title,
								  _ui->widget_text(_title),
								  _ui->widget_text_len(_title),
								  {
									  .font		   = theme.font_title,
									  .color	   = theme.color_text0,
									  .point_size  = theme.text_default_px_size * _zoom,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
	}

	void editor_animation_graph_widget_node_t::update_status_paint()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->get_paint().set_text(_entry_label,
								  _ui->widget_text(_entry_label),
								  _ui->widget_text_len(_entry_label),
								  {
									  .font		   = theme.font_title_bold,
									  .color	   = theme.color_accent_green,
									  .point_size  = theme.text_default_px_size * _zoom,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
		_ui->get_paint().set_text(_exit_label,
								  _ui->widget_text(_exit_label),
								  _ui->widget_text_len(_exit_label),
								  {
									  .font		   = theme.font_title_bold,
									  .color	   = theme.color_accent_err,
									  .point_size  = theme.text_default_px_size * _zoom,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
	}

	void editor_animation_graph_widget_node_t::draw_pin(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_animation_graph_widget_node_t& node = *static_cast<editor_animation_graph_widget_node_t*>(user_data);
		const ui::layout_out_t&						out	 = node._ui->get_tree().out(id);

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;

		const ui::vg_circle_paint_t pin_paint{
			.color		  = editor_theme_t::get().color_accent1,
			.aa_thickness = editor_theme_t::get().aa_thickness * ui::get_valid_scale(node._ui->get_ui_scale()),
		};
		canvas.add_circle(out.pos + out.size * 0.5f, out.size.x * 0.5f, pin_paint, state, node._ui->get_tree().draw_order_const(id));
	}
}
