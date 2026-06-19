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
#include "ui/widgets/editor_widget_asset_reference.hpp"
#include "assets/editor_asset.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		void set_rect(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& fill, f32 rounding = 0.0f, const vec4f_t& outline = {}, f32 outline_thickness = 0.0f)
		{
			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = fill;
			rect.fill_color_b		 = fill;
			rect.rounding			 = rounding;
			rect.outline_color		 = outline;
			rect.outline_thickness	 = outline_thickness;
			paint.set_rect(id, rect);
		}
	}

	void editor_widget_asset_reference_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_asset_reference_config_t& config)
	{
		SFG_ASSERT(config.asset_type != editor_asset_type_e::invalid);

		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "asset_reference");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = theme.item_spacing;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		root_in.child_clip_mode	 = ui::clip_mode_e::cpu_rect;

		set_rect(paint, _root, theme.color_frame, theme.item_rounding, theme.color_panel_light, theme.outline_thickness);
		paint.set_hover_color(_root, theme.color_panel);
		paint.set_press_color(_root, theme.color_frame_light);

		ui::listener_bundle_t root_listener = {};
		root_listener.user_data				= this;
		root_listener.on_click				= on_root_click;
		ui.get_input().set_listener(_root, root_listener);

		_thumbnail = ui.allocate_widget();
		ui.set_widget_debug_name(_thumbnail, "asset_reference_thumbnail");
		tree.attach(_root, _thumbnail);

		ui::layout_in_t& thumbnail_in = tree.in(_thumbnail);
		thumbnail_in.flags			  = ui::wf_visible;
		thumbnail_in.size_mode_x	  = ui::axis_mode_e::copy_other;
		thumbnail_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		thumbnail_in.size_value		  = {1.0f, 0.75f};
		thumbnail_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		thumbnail_in.pos_value.y	  = 0.5f;
		thumbnail_in.anchor_y		  = ui::anchor_e::center;
		set_rect(paint, _thumbnail, {1.0f, 1.0f, 1.0f, 1.0f}, theme.item_rounding);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "asset_reference_label");
		tree.attach(_root, _label);

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		tree.draw_order(_label)	  = tree.draw_order_const(_root) + 1;

		paint.set_text(_label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		refresh_title();
	}

	void editor_widget_asset_reference_t::uninit()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);
		popup->close_popup();

		_ui->deallocate_widget(_root);
		_ui		   = nullptr;
		_root	   = NULL_WIDGET;
		_thumbnail = NULL_WIDGET;
		_label	   = NULL_WIDGET;
		_config	   = {};
	}

	void editor_widget_asset_reference_t::refresh_title()
	{
		const sid_t selected = get_selected();
		const char* label	 = editor_asset_util_t::find_asset_display_name(selected);
		if (label == nullptr)
			label = "None";

		const editor_theme_t& theme = editor_theme_t::get();
		_ui->set_widget_text(_label, label);
		_ui->get_paint().set_text(
			_label, _ui->widget_text(_label), _ui->widget_text_len(_label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	sid_t editor_widget_asset_reference_t::get_selected() const
	{
		return _config.selected != nullptr ? _config.selected(_config.user_data) : NULL_SID;
	}

	void editor_widget_asset_reference_t::on_root_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_asset_reference_t& reference = *static_cast<editor_widget_asset_reference_t*>(user_data);
		editor_popup_controller_t*		 popup	   = editor_popup_controller_t::find(*reference._ui);
		SFG_ASSERT(popup != nullptr);

		const editor_theme_t&	  theme	   = editor_theme_t::get();
		const ui::layout_out_t&	  root_out = reference._ui->get_tree().out(reference._root);
		editor_asset_popup_desc_t desc	   = {};
		desc.asset_type					   = reference._config.asset_type;
		desc.pos						   = {root_out.pos.x, root_out.pos.y + root_out.size.y + theme.item_spacing};
		desc.width						   = root_out.size.x;
		desc.pressed					   = on_popup_asset_pressed;
		desc.user_data					   = &reference;
		desc.selected					   = reference.get_selected();
		popup->request_asset_popup(desc);
	}

	void editor_widget_asset_reference_t::on_popup_asset_pressed(sid_t guid, void* user_data)
	{
		editor_widget_asset_reference_t& reference = *static_cast<editor_widget_asset_reference_t*>(user_data);
		if (reference._config.pressed != nullptr)
			reference._config.pressed(guid, reference._config.user_data);
		reference.refresh_title();
	}
}
