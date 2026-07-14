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
#include "ui/editor_tooltip_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_thumbnail.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr u32 TOOLTIP_DRAW_ORDER  = 59000u;
		constexpr f32 TOOLTIP_OFFSET_X	  = 14.0f;
		constexpr f32 TOOLTIP_OFFSET_Y	  = 18.0f;
		constexpr f32 TOOLTIP_TEXT_FACTOR = 0.7f;

		editor_tooltip_controller_t* s_controllers[editor_tooltip_controller_t::MAX_CONTROLLERS] = {};
		u32							 s_controller_count											 = 0;

		ui::widget_id_t make_tooltip_row(ui::ui_context& ui, ui::widget_id_t parent, const char* debug_name, ui::axis_mode_e size_mode_x, ui::axis_mode_e size_mode_y, vec2f_t size_value, bool center = false)
		{
			ui::layout_tree_t&	  tree	= ui.get_tree();
			ui::paint_layer_t&	  paint = ui.get_paint();
			const editor_theme_t& theme = editor_theme_t::get();

			const ui::widget_id_t row = ui.allocate_widget();
			ui.set_widget_debug_name(row, debug_name);
			tree.attach(parent, row);

			ui::layout_in_t& row_in = tree.in(row);
			row_in.size_mode_x		= size_mode_x;
			row_in.size_mode_y		= size_mode_y;
			row_in.size_value		= size_value;
			row_in.flow				= ui::flow_e::row;
			row_in.child_spacing	= theme.item_spacing;
			row_in.child_margins	= {0.0f, theme.margin_horizontal * 0.5f, 0.0f, theme.margin_horizontal * 0.5f};

			if (center)
			{
				row_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
				row_in.pos_value.x = 0.5f;
				row_in.anchor_x	   = ui::anchor_e::center;
			}

			return row;
		}

		ui::widget_id_t make_tooltip_label(ui::ui_context& ui, ui::widget_id_t parent, const char* debug_name, vec4f_t color)
		{
			ui::layout_tree_t&	  tree	= ui.get_tree();
			ui::paint_layer_t&	  paint = ui.get_paint();
			const editor_theme_t& theme = editor_theme_t::get();

			const ui::widget_id_t label = ui.allocate_widget();
			ui.set_widget_debug_name(label, debug_name);
			tree.attach(parent, label);

			ui::layout_in_t& label_in = tree.in(label);
			label_in.pos_mode_x		  = ui::pos_mode_e::flow;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.y	  = 0.5f;
			label_in.anchor_y		  = ui::anchor_e::center;
			label_in.size_mode_x	  = ui::axis_mode_e::fixed;
			label_in.size_mode_y	  = ui::axis_mode_e::fixed;
			paint.set_text(label, nullptr, 0, {.font = theme.font_default, .color = color, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

			return label;
		}

		void set_tooltip_label_text(ui::ui_context& ui, ui::widget_id_t label, const char* text, vec4f_t color, f32 max_width)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui.set_widget_text(label, text != nullptr ? text : "");
			ui::layout_in_t& label_in = ui.get_tree().in(label);
			label_in.size_value		  = {math::min(max_width, static_cast<f32>(ui.widget_text_len(label)) * theme.text_default_px_size * TOOLTIP_TEXT_FACTOR), theme.text_default_px_size + 2.0f};
			ui.get_paint().set_text(
				label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = color, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}
	}

	void editor_tooltip_controller_t::init(ui::ui_context& ui)
	{
		SFG_ASSERT(s_controller_count < MAX_CONTROLLERS);

		_visible					= true;
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "tooltip_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = TOOLTIP_DRAW_ORDER;
		ui.set_pre_layout_tick(_foreground, on_pre_layout_tick, this);

		ui::layout_in_t& foreground_in = tree.in(_foreground);
		foreground_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_value	   = {1.0f, 1.0f};

		_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_frame, "tooltip_frame");
		tree.attach(_foreground, _frame);

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
		frame_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
		frame_in.size_mode_x	  = ui::axis_mode_e::max_children;
		frame_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		frame_in.flow			  = ui::flow_e::column;
		frame_in.child_margins	  = {0.0, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t frame_rect = {};
		frame_rect.fill_color_a		   = theme.color_frame;
		frame_rect.fill_color_b		   = theme.color_frame;
		frame_rect.outline_color	   = theme.color_outline_light;
		frame_rect.outline_thickness   = theme.outline_thickness;
		frame_rect.rounding			   = theme.item_rounding;
		paint.set_rect(_frame, frame_rect);

		_default_row							= make_tooltip_row(ui, _frame, "tooltip_default_row", ui::axis_mode_e::sum_children, ui::axis_mode_e::fixed, {0.0f, theme.item_height});
		_label									= make_tooltip_label(ui, _default_row, "tooltip_label", theme.color_text0);
		_asset_name_row							= make_tooltip_row(ui, _frame, "tooltip_asset_name_row", ui::axis_mode_e::sum_children, ui::axis_mode_e::fixed, {0.0f, theme.item_height});
		_asset_guid_row							= make_tooltip_row(ui, _frame, "tooltip_asset_guid_row", ui::axis_mode_e::sum_children, ui::axis_mode_e::fixed, {0.0f, theme.item_height});
		_asset_type_row							= make_tooltip_row(ui, _frame, "tooltip_asset_type_row", ui::axis_mode_e::sum_children, ui::axis_mode_e::fixed, {0.0f, theme.item_height});
		_asset_thumb_row						= make_tooltip_row(ui, _frame, "tooltip_asset_thumb_row", ui::axis_mode_e::fixed, ui::axis_mode_e::fixed, {theme.item_height * 4.0f, theme.item_height * 4.0f}, true);
		tree.in(_asset_thumb_row).child_margins = {0.0f, 0.0f, 0.0f, 0.0f};

		_asset_name_title = make_tooltip_label(ui, _asset_name_row, "tooltip_asset_name_title", theme.color_text1);
		_asset_name_value = make_tooltip_label(ui, _asset_name_row, "tooltip_asset_name_value", theme.color_text0);
		_asset_guid_title = make_tooltip_label(ui, _asset_guid_row, "tooltip_asset_guid_title", theme.color_text1);
		_asset_guid_value = make_tooltip_label(ui, _asset_guid_row, "tooltip_asset_guid_value", theme.color_text0);
		_asset_type_title = make_tooltip_label(ui, _asset_type_row, "tooltip_asset_type_title", theme.color_text1);
		_asset_type_value = make_tooltip_label(ui, _asset_type_row, "tooltip_asset_type_value", theme.color_text0);

		set_tooltip_label_text(ui, _asset_name_title, "Name:", theme.color_text1, theme.item_width);
		set_tooltip_label_text(ui, _asset_guid_title, "GUID:", theme.color_text1, theme.item_width);
		set_tooltip_label_text(ui, _asset_type_title, "Type:", theme.color_text1, theme.item_width);

		_asset_thumbnail = new editor_widget_thumbnail_t();
		_asset_thumbnail->init(ui, _asset_thumb_row, {.thumbnail = NULL_SID});

		_entries.reserve(64);
		s_controllers[s_controller_count++] = this;
		set_mode(editor_tooltip_mode_e::normal);
		set_visible(false);
	}

	void editor_tooltip_controller_t::uninit()
	{
		set_visible(false);
		_asset_thumbnail->uninit();
		delete _asset_thumbnail;
		_asset_thumbnail = nullptr;
		_ui->deallocate_widget(_foreground);

		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i] == this)
			{
				s_controllers[i]					  = s_controllers[s_controller_count - 1];
				s_controllers[s_controller_count - 1] = nullptr;
				s_controller_count--;
				break;
			}
		}

		_entries.resize(0);
		_ui					  = nullptr;
		_foreground			  = NULL_WIDGET;
		_frame				  = NULL_WIDGET;
		_default_row		  = NULL_WIDGET;
		_label				  = NULL_WIDGET;
		_asset_name_row		  = NULL_WIDGET;
		_asset_guid_row		  = NULL_WIDGET;
		_asset_type_row		  = NULL_WIDGET;
		_asset_thumb_row	  = NULL_WIDGET;
		_asset_name_title	  = NULL_WIDGET;
		_asset_name_value	  = NULL_WIDGET;
		_asset_guid_title	  = NULL_WIDGET;
		_asset_guid_value	  = NULL_WIDGET;
		_asset_type_title	  = NULL_WIDGET;
		_asset_type_value	  = NULL_WIDGET;
		_asset_thumbnail_guid = NULL_SID;
		_hovered_owner		  = NULL_WIDGET;
		_hover_seconds		  = 0.0f;
		_mode				  = editor_tooltip_mode_e::invalid;
		_visible			  = false;
	}

	void editor_tooltip_controller_t::set_tooltip(ui::widget_id_t owner, const editor_tooltip_desc_t& desc)
	{
		tooltip_entry_t* entry = find_entry(owner);
		if (entry == nullptr)
		{
			_entries.push_back({});
			entry		 = &_entries.back();
			entry->owner = owner;
		}
		entry->mode = editor_tooltip_mode_e::normal;
		entry->desc = desc;
	}

	void editor_tooltip_controller_t::set_asset_tooltip(ui::widget_id_t owner, const editor_asset_tooltip_desc_t& desc)
	{
		tooltip_entry_t* entry = find_entry(owner);
		if (entry == nullptr)
		{
			_entries.push_back({});
			entry		 = &_entries.back();
			entry->owner = owner;
		}
		entry->mode		   = editor_tooltip_mode_e::asset;
		entry->asset_name  = desc.name != nullptr ? desc.name : "";
		entry->asset_guid  = desc.guid != nullptr ? desc.guid : "";
		entry->asset_type  = desc.type != nullptr ? desc.type : "";
		entry->asset_thumb = desc.thumbnail;
		entry->asset_delay = desc.delay;
		entry->asset_width = desc.max_width;
	}

	void editor_tooltip_controller_t::clear_tooltip(ui::widget_id_t owner)
	{
		for (auto it = _entries.begin(); it != _entries.end(); ++it)
		{
			if (it->owner == owner)
			{
				_entries.erase(it);
				if (_hovered_owner == owner)
				{
					_hovered_owner = NULL_WIDGET;
					_hover_seconds = 0.0f;
					set_visible(false);
				}
				return;
			}
		}
	}

	editor_tooltip_controller_t* editor_tooltip_controller_t::find(ui::ui_context& ui)
	{
		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i]->_ui == &ui)
				return s_controllers[i];
		}
		return nullptr;
	}

	void editor_tooltip_controller_t::tick(f32 dt_seconds)
	{
		ui::layout_tree_t&	tree  = _ui->get_tree();
		ui::input_router_t& input = _ui->get_input();

		const ui::widget_id_t  hovered = input.get_hovered();
		const tooltip_entry_t* entry   = find_entry(hovered);
		if (entry == nullptr || input.is_popup_scope_active() || input.is_pressed(ui::mouse_button_e::left) != NULL_WIDGET)
		{
			_hovered_owner = NULL_WIDGET;
			_hover_seconds = 0.0f;
			set_visible(false);
			return;
		}

		if (_hovered_owner != hovered)
		{
			_hovered_owner = hovered;
			_hover_seconds = 0.0f;
			set_visible(false);
		}

		_hover_seconds += dt_seconds;
		const f32 delay = entry->mode == editor_tooltip_mode_e::asset ? entry->asset_delay : entry->desc.delay;
		if (_hover_seconds < delay)
			return;

		const editor_theme_t& theme = editor_theme_t::get();
		if (entry->mode == editor_tooltip_mode_e::asset)
		{
			set_mode(editor_tooltip_mode_e::asset);
			set_tooltip_label_text(*_ui, _asset_name_value, entry->asset_name.c_str(), theme.color_text0, entry->asset_width);
			set_tooltip_label_text(*_ui, _asset_guid_value, entry->asset_guid.c_str(), theme.color_text0, entry->asset_width);
			set_tooltip_label_text(*_ui, _asset_type_value, entry->asset_type.c_str(), theme.color_text0, entry->asset_width);
			if (_asset_thumbnail_guid != entry->asset_thumb)
			{
				_asset_thumbnail->set_thumbnail(entry->asset_thumb);
				_asset_thumbnail_guid = entry->asset_thumb;
			}
		}
		else
		{
			set_mode(editor_tooltip_mode_e::normal);
			set_tooltip_label_text(*_ui, _label, entry->desc.text, theme.color_text0, entry->desc.max_width);
		}

		const vec2f_t& mouse = input.get_mouse_position();
		f32			   x	 = mouse.x + TOOLTIP_OFFSET_X;
		f32			   y	 = mouse.y + TOOLTIP_OFFSET_Y;

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.pos_value		  = {x, y};
		set_visible(true);
	}

	void editor_tooltip_controller_t::set_mode(editor_tooltip_mode_e mode)
	{
		if (_mode == mode)
			return;

		_mode							  = mode;
		ui::layout_tree_t& tree			  = _ui->get_tree();
		const bool		   normal_visible = mode == editor_tooltip_mode_e::normal;
		const bool		   asset_visible  = mode == editor_tooltip_mode_e::asset;
		tree.set_visible(_default_row, normal_visible, false);
		tree.set_visible(_asset_name_row, asset_visible, false);
		tree.set_visible(_asset_guid_row, asset_visible, false);
		tree.set_visible(_asset_type_row, asset_visible, false);
		tree.set_visible(_asset_thumb_row, asset_visible, false);
	}

	void editor_tooltip_controller_t::set_visible(bool visible)
	{
		if (_visible == visible)
			return;

		_visible				= visible;
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_foreground, visible, false);
	}

	editor_tooltip_controller_t::tooltip_entry_t* editor_tooltip_controller_t::find_entry(ui::widget_id_t owner)
	{
		for (tooltip_entry_t& entry : _entries)
		{
			if (entry.owner == owner)
				return &entry;
		}
		return nullptr;
	}

	const editor_tooltip_controller_t::tooltip_entry_t* editor_tooltip_controller_t::find_entry(ui::widget_id_t owner) const
	{
		for (const tooltip_entry_t& entry : _entries)
		{
			if (entry.owner == owner)
				return &entry;
		}
		return nullptr;
	}

	void editor_tooltip_controller_t::on_pre_layout_tick(ui::ui_context&, ui::widget_id_t, f32 dt_seconds, void* user_data)
	{
		static_cast<editor_tooltip_controller_t*>(user_data)->tick(dt_seconds);
	}
}
