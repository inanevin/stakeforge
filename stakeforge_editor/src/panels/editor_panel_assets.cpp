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
#include "panels/editor_panel_assets.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ASSETS_PANE_SPLIT_MIN			   0.15f
#define ASSETS_PANE_SPLIT_MAX			   0.35f
#define ASSETS_SPLIT_BORDER_THICKNESS_MULT 2.0f

	namespace
	{
		const char* assets_filter_to_text(u8 filter)
		{
			return filter == assets_filter_favourites ? "Favourites" : "All";
		}
	}

	editor_panel_assets_t::editor_panel_assets_t()
	{
		set_type(editor_panel_type_e::assets);
		set_title(editor_panel_type_to_string(editor_panel_type_e::assets));
	}

	void editor_panel_assets_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_assets_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane, "assets_left_pane");
		tree.attach(_root, _assets_left_pane);

		ui::layout_in_t& left_pane_in = tree.in(_assets_left_pane);
		left_pane_in.flow			  = ui::flow_e::column;
		left_pane_in.child_spacing	  = 0.0f;
		left_pane_in.child_margins	  = {0.0f, 0.0f, 0.0f, 0.0f};

		_assets_left_pane_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane_top_row, "assets_left_pane_top_row");
		tree.attach(_assets_left_pane, _assets_left_pane_top_row);

		ui::layout_in_t& top_row_in = tree.in(_assets_left_pane_top_row);
		top_row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_row_in.size_mode_y		= ui::axis_mode_e::fixed;
		top_row_in.size_value		= {1.0f, theme.item_area_height};
		top_row_in.flow				= ui::flow_e::row;
		top_row_in.child_spacing	= theme.item_spacing;
		top_row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		editor_icon_button_config_t filter_button_config = {};
		filter_button_config.frame_color				 = {0.0f, 0.0f, 0.0f, 0.0f};
		filter_button_config.hover_color				 = theme.color_panel_light;
		filter_button_config.press_color				 = theme.color_frame;
		filter_button_config.frame_toggled_color		 = theme.color_frame;
		filter_button_config.icon						 = ICON_FILTER;
		filter_button_config.toggled_icon				 = ICON_FILTER;
		filter_button_config.icon_color					 = theme.color_text0;
		filter_button_config.tooltip					 = "Filter";
		filter_button_config.size						 = theme.item_height;
		filter_button_config.icon_size					 = theme.text_default_px_size;
		filter_button_config.on_clicked					 = on_filter_button_pressed;
		filter_button_config.user_data					 = this;
		_filter_button.init(ui, _assets_left_pane_top_row, filter_button_config);

		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.text_value				  = _search_str.c_str();
		search_config.type						  = editor_input_field_type_e::text;
		search_config.on_text_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _assets_left_pane_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.flags |= ui::wf_overlay;
		search_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value	  = {1.0f, 0.5f};
		search_in.anchor_x	  = ui::anchor_e::end;
		search_in.anchor_y	  = ui::anchor_e::center;
		search_in.size_mode_x = ui::axis_mode_e::fixed;
		search_in.size_mode_y = ui::axis_mode_e::fixed;
		search_in.size_value  = {theme.item_width * 1.5f, theme.item_height};

		_assets_left_pane_body = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane_body, "assets_left_pane_body");
		tree.attach(_assets_left_pane, _assets_left_pane_body);

		ui::layout_in_t& left_body_in = tree.in(_assets_left_pane_body);
		left_body_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		left_body_in.size_mode_y	  = ui::axis_mode_e::fill;
		left_body_in.size_value		  = {1.0f, 1.0f};

		ui::vg_rect_paint_t left_body_rect = {};
		left_body_rect.fill_color_a		   = theme.color_frame;
		left_body_rect.fill_color_b		   = theme.color_frame;
		paint.set_rect(_assets_left_pane_body, left_body_rect);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);

		_assets_body_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane, "assets_body_pane");
		tree.attach(_root, _assets_body_pane);

		apply_pane_split();
	}

	void editor_panel_assets_t::uninit()
	{
		editor_popup_controller_t::find(*_ui)->close_popup();
		_search_input.uninit();
		_filter_button.uninit();
		_split_border.uninit();
		_ui->deallocate_widget(_assets_left_pane_top_row);
		_ui->deallocate_widget(_assets_left_pane_body);
		_ui->deallocate_widget(_assets_left_pane);
		_ui->deallocate_widget(_assets_body_pane);

		_assets_left_pane		  = NULL_WIDGET;
		_assets_left_pane_top_row = NULL_WIDGET;
		_assets_left_pane_body	  = NULL_WIDGET;
		_assets_body_pane		  = NULL_WIDGET;
		_search_str.clear();

		editor_panel_t::uninit();
	}

	void editor_panel_assets_t::serialize(nlohmann::json& j) const
	{
		j				= nlohmann::json::object();
		j["pane_split"] = _pane_split;
		j["filter"]		= static_cast<u32>(_filter_flags);
		j["search_str"] = _search_str;
	}

	void editor_panel_assets_t::deserialize(const nlohmann::json& j)
	{
		_pane_split		= math::clamp(j.value<f32>("pane_split", _pane_split), ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);
		const u8 filter = static_cast<u8>(j.value<u32>("filter", static_cast<u32>(assets_filter_all))) & static_cast<u8>(assets_filter_all | assets_filter_favourites);
		_filter_flags	= (filter & assets_filter_favourites) != 0 ? assets_filter_favourites : assets_filter_all;
		_search_str		= j.value<string_t>("search_str", {});
	}

	void editor_panel_assets_t::make_visible(bool visible)
	{
		editor_panel_t::make_visible(visible);
		if (!visible)
			editor_popup_controller_t::find(*_ui)->close_popup();
	}

	void editor_panel_assets_t::apply_pane_split()
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();
		_pane_split					= math::clamp(_pane_split, ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);

		ui::layout_in_t& left_in = tree.in(_assets_left_pane);
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * ASSETS_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		ui::layout_in_t& body_in = tree.in(_assets_body_pane);
		body_in.size_mode_x		 = ui::axis_mode_e::fill;
		body_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		body_in.size_value		 = {1.0f, 1.0f};
	}

	void editor_panel_assets_t::open_filter_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_popup_item_desc_t items[] = {
			{.text = assets_filter_to_text(assets_filter_all), .id = assets_filter_all, .selected = _filter_flags == assets_filter_all},
			{.text = assets_filter_to_text(assets_filter_favourites), .id = assets_filter_favourites, .selected = _filter_flags == assets_filter_favourites},
		};

		const editor_theme_t&	theme = editor_theme_t::get();
		const ui::layout_out_t& out	  = _ui->get_tree().out(_filter_button.get_root());

		editor_popup_desc_t desc = {};
		desc.items				 = items;
		desc.item_count			 = static_cast<u16>(sizeof(items) / sizeof(items[0]));
		desc.pos				 = {out.pos.x, out.pos.y + out.size.y + theme.item_spacing};
		desc.width				 = theme.item_width;
		desc.pressed			 = on_filter_popup_pressed;
		desc.user_data			 = this;
		popup->request_popup(desc);
	}

	void editor_panel_assets_t::on_filter_popup_pressed(u16 value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel._filter_flags			 = value == assets_filter_favourites ? assets_filter_favourites : assets_filter_all;
	}

	void editor_panel_assets_t::on_filter_button_pressed(bool, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.open_filter_popup();
	}

	void editor_panel_assets_t::on_search_changed(const char* value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel._search_str			 = value != nullptr ? value : "";
	}

	void editor_panel_assets_t::on_split_border_drag(editor_split_border_t&, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_panel_assets_t&	assets_panel = *static_cast<editor_panel_assets_t*>(user_data);
		const ui::layout_out_t& out			 = assets_panel._ui->get_tree().out(assets_panel._root);
		SFG_ASSERT(out.size.x > 0.0f);

		assets_panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);
		assets_panel.apply_pane_split();
	}
}
