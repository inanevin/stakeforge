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
#include "ui/editor_popup_controller.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/frame_string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr u32 POPUP_FG_DRAW_ORDER	   = 61000u;
		constexpr u32 POPUP_DRAW_ORDER		   = 61001u;
		constexpr u32 ASSET_POPUP_VISIBLE_ROWS = 8u;

		editor_popup_controller_t* s_controllers[editor_popup_controller_t::MAX_CONTROLLERS] = {};
		u32						   s_controller_count										 = 0;

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u16>(ui::wf_visible | (input ? ui::wf_input : 0)) : 0;
		}

		void set_rect(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& fill, f32 rounding = 0.0f)
		{
			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = fill;
			rect.fill_color_b		 = fill;
			rect.rounding			 = rounding;
			paint.set_rect(id, rect);
		}
	}

	void editor_popup_controller_t::init(ui::ui_context& ui)
	{
		SFG_ASSERT(_ui == nullptr);
		SFG_ASSERT(s_controller_count < MAX_CONTROLLERS);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "popup_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = POPUP_FG_DRAW_ORDER;

		ui::layout_in_t& foreground_in = tree.in(_foreground);
		foreground_in.flags			   = 0;
		foreground_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_value	   = {1.0f, 1.0f};

		_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_frame, "popup_frame");
		tree.attach(_foreground, _frame);
		tree.draw_order(_frame) = POPUP_DRAW_ORDER;

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.flags			  = 0;
		frame_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
		frame_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
		frame_in.size_mode_x	  = ui::axis_mode_e::fixed;
		frame_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		frame_in.flow			  = ui::flow_e::column;
		frame_in.child_spacing	  = 0.0f;
		frame_in.child_margins	  = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		ui::vg_rect_paint_t frame_rect = {};
		frame_rect.fill_color_a		   = theme.color_frame_light;
		frame_rect.fill_color_b		   = theme.color_frame_light;
		frame_rect.outline_color	   = theme.color_outline_light;
		frame_rect.outline_thickness   = theme.outline_thickness;
		paint.set_rect(_frame, frame_rect);

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = on_row_click;

		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			_row_frames[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_frames[i], "popup_item");
			tree.attach(_frame, _row_frames[i]);

			ui::layout_in_t& row_in = tree.in(_row_frames[i]);
			row_in.flags			= 0;
			row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
			row_in.size_mode_y		= ui::axis_mode_e::fixed;
			row_in.size_value		= {1.0f, theme.item_height};
			row_in.flow				= ui::flow_e::row;
			row_in.child_spacing	= 0.0f;
			row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, 0.0f};

			ui::vg_rect_paint_t row_rect = {};
			row_rect.fill_color_a		 = {0.0f, 0.0f, 0.0f, 0.0f};
			row_rect.fill_color_b		 = {0.0f, 0.0f, 0.0f, 0.0f};
			paint.set_rect(_row_frames[i], row_rect);
			paint.set_hover_color(_row_frames[i], theme.color_panel);
			paint.set_press_color(_row_frames[i], theme.color_frame_light);
			ui.get_input().set_listener(_row_frames[i], row_listener);

			_row_markers[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_markers[i], "popup_selected_marker");
			tree.attach(_row_frames[i], _row_markers[i]);

			ui::layout_in_t& marker_in = tree.in(_row_markers[i]);
			marker_in.flags			   = 0;
			marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
			marker_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			marker_in.size_value	   = {theme.item_height, 1.0f};

			_row_marker_labels[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_marker_labels[i], "popup_selected_marker_icon");
			tree.attach(_row_markers[i], _row_marker_labels[i]);

			ui::layout_in_t& marker_label_in = tree.in(_row_marker_labels[i]);
			marker_label_in.flags			 = 0;
			marker_label_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
			marker_label_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
			marker_label_in.pos_value		 = {0.5f, 0.5f};
			marker_label_in.anchor_x		 = ui::anchor_e::center;
			marker_label_in.anchor_y		 = ui::anchor_e::center;
			ui.set_widget_text(_row_marker_labels[i], ICON_FILLED_CIRCLE);
			paint.set_text(_row_marker_labels[i],
						   ui.widget_text(_row_marker_labels[i]),
						   ui.widget_text_len(_row_marker_labels[i]),
						   {.font = theme.font_icons, .color = theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

			_row_labels[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_labels[i], "popup_item_label");
			tree.attach(_row_frames[i], _row_labels[i]);

			ui::layout_in_t& label_in = tree.in(_row_labels[i]);
			label_in.flags			  = 0;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.y	  = 0.5f;
			label_in.anchor_y		  = ui::anchor_e::center;
			paint.set_text(_row_labels[i], nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		_asset_list_area = ui.allocate_widget();
		ui.set_widget_debug_name(_asset_list_area, "asset_popup_list");
		tree.attach(_frame, _asset_list_area);

		ui::layout_in_t& asset_list_in = tree.in(_asset_list_area);
		asset_list_in.flags			   = 0;
		asset_list_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		asset_list_in.size_mode_y	   = ui::axis_mode_e::fixed;
		asset_list_in.size_value	   = {1.0f, theme.item_height};
		asset_list_in.flow			   = ui::flow_e::column;
		asset_list_in.child_spacing	   = 0.0f;
		asset_list_in.child_clip_mode  = ui::clip_mode_e::scissor_rect;
		ui.set_pre_layout_tick(_asset_list_area, on_asset_list_tick, this);

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _asset_list_area;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_asset_scrollbar.init(ui, scrollbar_config);

		editor_input_field_config_t input_config = {};
		input_config.on_submitted				 = on_input_submitted;
		input_config.user_data					 = this;
		_input.init(ui, _foreground, input_config);
		_input.set_draw_order(POPUP_DRAW_ORDER);

		s_controllers[s_controller_count++] = this;
		set_visible(false);
	}

	void editor_popup_controller_t::uninit()
	{
		close_popup(false);
		destroy_asset_rows();
		_asset_scrollbar.uninit();
		_input.uninit();
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

		_ui				 = nullptr;
		_foreground		 = NULL_WIDGET;
		_frame			 = NULL_WIDGET;
		_asset_list_area = NULL_WIDGET;
		_desc			 = {};
		_input_desc		 = {};
		_asset_desc		 = {};
		_mode			 = popup_mode_e::none;
		_visible		 = false;
		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			_row_frames[i]		  = NULL_WIDGET;
			_row_markers[i]		  = NULL_WIDGET;
			_row_marker_labels[i] = NULL_WIDGET;
			_row_labels[i]		  = NULL_WIDGET;
			_items[i]			  = {};
		}
		_asset_items.resize(0);
		_asset_rows.resize(0);
	}

	void editor_popup_controller_t::request_popup(const editor_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(desc.items != nullptr);
		SFG_ASSERT(desc.item_count <= MAX_ITEMS);

		close_popup(false);
		_desc = desc;
		_mode = popup_mode_e::items;
		for (u32 i = 0; i < MAX_ITEMS; ++i)
			_items[i] = i < desc.item_count ? desc.items[i] : editor_popup_item_desc_t{};

		refresh_rows();
		refresh_layout();
		set_visible(true);

		ui::widget_id_t popup_roots[] = {_frame};
		_ui->get_input().set_popup_scope(_frame, popup_roots, 1, on_popup_outside, this);
	}

	void editor_popup_controller_t::request_input_popup(const editor_input_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);

		close_popup(false);
		_input_desc = desc;
		_mode		= popup_mode_e::input;
		_input.set_placeholder(desc.placeholder);
		_input.set_text(desc.text != nullptr ? desc.text : "");

		refresh_layout();
		set_visible(true);
		_ui->get_input().set_focus(_input.get_root(), false);
		_input.select_all();

		ui::widget_id_t popup_roots[] = {_input.get_root()};
		_ui->get_input().set_popup_scope(_input.get_root(), popup_roots, 1, on_popup_outside, this);
	}

	void editor_popup_controller_t::request_asset_popup(const editor_asset_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);

		close_popup(false);
		_asset_desc = desc;
		_mode		= popup_mode_e::assets;
		collect_asset_items();
		refresh_asset_rows();
		refresh_layout();
		begin_asset_scroll_to_selected();
		set_visible(true);

		ui::widget_id_t popup_roots[] = {_frame, _asset_scrollbar.get_root()};
		_ui->get_input().set_popup_scope(_frame, popup_roots, 2, on_popup_outside, this);
	}

	void editor_popup_controller_t::close_popup(bool notify_input)
	{
		if (_ui == nullptr || !_visible)
			return;

		const bool						   notify			= notify_input && _mode == popup_mode_e::input && _input_desc.closed != nullptr;
		const editor_popup_input_closed_fn closed			= _input_desc.closed;
		void*							   closed_user_data = _input_desc.user_data;
		const frame_string_t<char>		   input_value		= _input.get_text();

		set_visible(false);
		_ui->get_input().clear_popup_scope();
		_desc						 = {};
		_input_desc					 = {};
		_asset_desc					 = {};
		_mode						 = popup_mode_e::none;
		_asset_scroll_pending_frames = 0;
		for (editor_popup_item_desc_t& item : _items)
			item = {};
		_asset_items.resize(0);

		if (notify)
			closed(input_value.c_str(), closed_user_data);
	}

	editor_popup_controller_t* editor_popup_controller_t::find(ui::ui_context& ui)
	{
		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i]->_ui == &ui)
				return s_controllers[i];
		}
		return nullptr;
	}

	void editor_popup_controller_t::set_visible(bool visible)
	{
		_visible				= visible;
		ui::layout_tree_t& tree = _ui->get_tree();
		set_widget_visible(tree, _foreground, visible, false);
		set_widget_visible(tree, _frame, visible && (_mode == popup_mode_e::items || _mode == popup_mode_e::assets), false);
		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			const bool item_visible	  = visible && _mode == popup_mode_e::items && i < _desc.item_count;
			const bool marker_visible = item_visible && _items[i].selected;
			set_widget_visible(tree, _row_frames[i], item_visible, item_visible);
			set_widget_visible(tree, _row_markers[i], item_visible, false);
			set_widget_visible(tree, _row_marker_labels[i], marker_visible, false);
			set_widget_visible(tree, _row_labels[i], item_visible, false);
		}
		ui::layout_in_t& asset_list_in = tree.in(_asset_list_area);
		asset_list_in.flags			   = visible && _mode == popup_mode_e::assets ? static_cast<u16>(ui::wf_visible | ui::wf_input | ui::wf_scroll_y) : 0;
		for (size_t i = 0; i < _asset_rows.size(); ++i)
		{
			const asset_row_t& row		   = _asset_rows[i];
			const bool		   row_visible = visible && _mode == popup_mode_e::assets && i < _asset_items.size();
			set_widget_visible(tree, row.root, row_visible, row_visible);
			set_widget_visible(tree, row.marker, row_visible, false);
			set_widget_visible(tree, row.marker_icon, row_visible, false);
			set_widget_visible(tree, row.thumbnail, row_visible, false);
			set_widget_visible(tree, row.label, row_visible, false);
		}
		_input.set_visible(visible && _mode == popup_mode_e::input);
	}

	void editor_popup_controller_t::refresh_rows()
	{
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();
		for (u32 i = 0; i < _desc.item_count; ++i)
		{
			_ui->set_widget_text(_row_labels[i], _items[i].text);
			paint.set_text(_row_labels[i],
						   _ui->widget_text(_row_labels[i]),
						   _ui->widget_text_len(_row_labels[i]),
						   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			paint.def(_row_marker_labels[i]).text.color = theme.color_accent0;
			set_widget_visible(_ui->get_tree(), _row_marker_labels[i], _visible && _items[i].selected, false);
		}
	}

	void editor_popup_controller_t::refresh_asset_rows()
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = on_asset_row_click;

		_asset_rows.reserve(_asset_items.size());
		for (size_t i = 0; i < _asset_items.size(); ++i)
		{
			const asset_popup_item_t& item = _asset_items[i];
			if (i >= _asset_rows.size())
			{
				asset_row_t row = {};

				row.root = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.root, "asset_popup_item");
				tree.attach(_asset_list_area, row.root);
				tree.draw_order(row.root) = tree.draw_order_const(_asset_list_area) + 1;

				ui::layout_in_t& row_in = tree.in(row.root);
				row_in.flags			= 0;
				row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
				row_in.size_mode_y		= ui::axis_mode_e::fixed;
				row_in.size_value		= {1.0f, theme.item_height};
				row_in.flow				= ui::flow_e::row;
				row_in.child_spacing	= theme.item_spacing;
				row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

				set_rect(paint, row.root, {0.0f, 0.0f, 0.0f, 0.0f});
				paint.set_hover_color(row.root, theme.color_panel);
				paint.set_press_color(row.root, theme.color_frame_light);
				_ui->get_input().set_listener(row.root, row_listener);

				row.marker = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.marker, "asset_popup_item_selected_marker");
				tree.attach(row.root, row.marker);

				ui::layout_in_t& marker_in = tree.in(row.marker);
				marker_in.flags			   = 0;
				marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
				marker_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
				marker_in.size_value	   = {theme.item_height, 1.0f};

				row.marker_icon = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.marker_icon, "asset_popup_item_selected_marker_icon");
				tree.attach(row.marker, row.marker_icon);

				ui::layout_in_t& marker_icon_in = tree.in(row.marker_icon);
				marker_icon_in.flags			= 0;
				marker_icon_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
				marker_icon_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
				marker_icon_in.pos_value		= {0.5f, 0.5f};
				marker_icon_in.anchor_x			= ui::anchor_e::center;
				marker_icon_in.anchor_y			= ui::anchor_e::center;
				_ui->set_widget_text(row.marker_icon, ICON_FILLED_CIRCLE);
				paint.set_text(row.marker_icon,
							   _ui->widget_text(row.marker_icon),
							   _ui->widget_text_len(row.marker_icon),
							   {.font = theme.font_icons, .color = {}, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				row.thumbnail = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.thumbnail, "asset_popup_item_thumbnail");
				tree.attach(row.root, row.thumbnail);

				ui::layout_in_t& thumbnail_in = tree.in(row.thumbnail);
				thumbnail_in.flags			  = 0;
				thumbnail_in.size_mode_x	  = ui::axis_mode_e::copy_other;
				thumbnail_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
				thumbnail_in.size_value		  = {1.0f, 0.75f};
				thumbnail_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				thumbnail_in.pos_value.y	  = 0.5f;
				thumbnail_in.anchor_y		  = ui::anchor_e::center;
				set_rect(paint, row.thumbnail, {1.0f, 1.0f, 1.0f, 1.0f}, theme.item_rounding);

				row.label = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.label, "asset_popup_item_label");
				tree.attach(row.root, row.label);

				ui::layout_in_t& label_in = tree.in(row.label);
				label_in.flags			  = 0;
				label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				label_in.pos_value.y	  = 0.5f;
				label_in.anchor_y		  = ui::anchor_e::center;

				_asset_rows.push_back(row);
			}

			asset_row_t& row					  = _asset_rows[i];
			row.guid							  = item.guid;
			paint.def(row.marker_icon).text.color = item.guid == _asset_desc.selected ? theme.color_accent0 : vec4f_t{};

			_ui->set_widget_text(row.label, item.name.c_str());
			paint.set_text(row.label,
						   _ui->widget_text(row.label),
						   _ui->widget_text_len(row.label),
						   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		for (size_t i = _asset_items.size(); i < _asset_rows.size(); ++i)
			_asset_rows[i].guid = NULL_SID;
	}

	void editor_popup_controller_t::refresh_layout()
	{
		ui::layout_tree_t&		tree   = _ui->get_tree();
		const editor_theme_t&	theme  = editor_theme_t::get();
		const ui::layout_out_t& screen = tree.out(tree.get_root());

		f32 width  = _desc.width;
		f32 height = static_cast<f32>(_desc.item_count) * theme.item_height + theme.margin_vertical * 2.0f;
		if (_mode == popup_mode_e::items)
		{
			for (u32 i = 0; i < _desc.item_count; ++i)
				width = math::max(width, theme.item_height + static_cast<f32>(_ui->widget_text_len(_row_labels[i])) * theme.text_default_px_size * 0.7f + theme.margin_horizontal * 2.0f);
		}
		else if (_mode == popup_mode_e::input)
		{
			width  = math::max(_input_desc.width, theme.item_width);
			height = theme.item_height + theme.margin_vertical * 2.0f;
		}
		else if (_mode == popup_mode_e::assets)
		{
			width				   = math::max(_asset_desc.width, theme.item_width * 2.0f);
			const u32 visible_rows = math::min(ASSET_POPUP_VISIBLE_ROWS, static_cast<u32>(math::max<size_t>(1, _asset_items.size())));
			height				   = static_cast<f32>(visible_rows) * theme.item_height + theme.margin_vertical * 2.0f;
			for (const asset_row_t& row : _asset_rows)
				width = math::max(width, theme.item_height + theme.item_spacing + theme.item_height + theme.item_spacing + static_cast<f32>(_ui->widget_text_len(row.label)) * theme.text_default_px_size * 0.7f + theme.margin_horizontal * 2.0f);
		}

		const f32 scale		= _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		const f32 width_px	= width * scale;
		const f32 height_px = height * scale;
		f32		  x			= _mode == popup_mode_e::input ? _input_desc.pos.x : _mode == popup_mode_e::assets ? _asset_desc.pos.x : _desc.pos.x;
		f32		  y			= _mode == popup_mode_e::input ? _input_desc.pos.y : _mode == popup_mode_e::assets ? _asset_desc.pos.y : _desc.pos.y;
		if (x + width_px > screen.clip.x + screen.clip.z)
			x = screen.clip.x + screen.clip.z - width_px;
		if (y + height_px > screen.clip.y + screen.clip.w)
			y = (_mode == popup_mode_e::input ? _input_desc.pos.y : _mode == popup_mode_e::assets ? _asset_desc.pos.y : _desc.pos.y) - height_px;
		x = math::clamp(x, screen.clip.x, math::max(screen.clip.x, screen.clip.x + screen.clip.z - width_px));
		y = math::clamp(y, screen.clip.y, math::max(screen.clip.y, screen.clip.y + screen.clip.w - height_px));

		ui::layout_in_t& frame_in = tree.in(_frame);
		if (_mode == popup_mode_e::input)
		{
			ui::layout_in_t& input_in = tree.in(_input.get_root());
			input_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
			input_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
			input_in.pos_value		  = {x, y};
			input_in.size_mode_x	  = ui::axis_mode_e::fixed;
			input_in.size_mode_y	  = ui::axis_mode_e::fixed;
			input_in.size_value		  = {width, theme.item_height};
			return;
		}

		frame_in.pos_value	= {x, y};
		frame_in.size_value = {width, 0.0f};
		if (_mode == popup_mode_e::assets)
			tree.in(_asset_list_area).size_value.y = height - theme.margin_vertical * 2.0f;
	}

	void editor_popup_controller_t::collect_asset_items()
	{
		_asset_items.resize(0);

		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  tree			= asset_manager.get_asset_tree();
		_asset_items.reserve(asset_manager.get_assets().size());

		for (auto it = tree.begin_handle(); it != tree.end_handle(); ++it)
		{
			const editor_asset_node_t& node = tree.value(*it);
			if (node.type != editor_asset_node_type_e::asset)
				continue;

			const editor_asset_t* asset = asset_manager.find_asset(node.asset_id);
			if (asset == nullptr || asset->asset_type != _asset_desc.asset_type)
				continue;

			_asset_items.push_back({.name = node.name, .guid = asset->guid});
		}
	}

	void editor_popup_controller_t::destroy_asset_rows()
	{
		for (size_t i = _asset_rows.size(); i > 0; --i)
			_ui->deallocate_widget(_asset_rows[i - 1].root);
		_asset_rows.resize(0);
		if (_ui != nullptr && _asset_list_area != NULL_WIDGET)
			_ui->get_tree().in(_asset_list_area).scroll_offset = {};
	}

	void editor_popup_controller_t::begin_asset_scroll_to_selected()
	{
		_asset_scroll_pending_frames = 0;
		_asset_scroll_target		 = 0;
		_asset_scrollbar.set_scroll_y_immediate(0.0f);
		for (u32 i = 0; i < _asset_items.size(); ++i)
		{
			if (_asset_items[i].guid == _asset_desc.selected)
			{
				_asset_scroll_target		 = i;
				_asset_scroll_pending_frames = 2;
				return;
			}
		}
	}

	u32 editor_popup_controller_t::find_row(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _desc.item_count; ++i)
		{
			if (_row_frames[i] == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	u32 editor_popup_controller_t::find_asset_row(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _asset_rows.size(); ++i)
		{
			if (_asset_rows[i].root == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	void editor_popup_controller_t::on_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_popup_controller_t&		   popup			 = *static_cast<editor_popup_controller_t*>(user_data);
		const u32						   row				 = popup.find_row(id);
		const u16						   value			 = popup._items[row].id;
		const editor_popup_item_pressed_fn pressed			 = popup._desc.pressed;
		void*							   pressed_user_data = popup._desc.user_data;
		const bool						   close_on_pressed	 = popup._desc.close_on_pressed;
		if (close_on_pressed)
			popup.close_popup();
		if (pressed != nullptr)
			pressed(value, pressed_user_data);
	}

	void editor_popup_controller_t::on_asset_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_popup_controller_t&			popup			  = *static_cast<editor_popup_controller_t*>(user_data);
		const u32							row				  = popup.find_asset_row(id);
		const sid_t							value			  = popup._asset_rows[row].guid;
		const editor_popup_asset_pressed_fn pressed			  = popup._asset_desc.pressed;
		void*								pressed_user_data = popup._asset_desc.user_data;
		const bool							close_on_pressed  = popup._asset_desc.close_on_pressed;
		if (close_on_pressed)
			popup.close_popup();
		if (pressed != nullptr)
			pressed(value, pressed_user_data);
	}

	void editor_popup_controller_t::on_popup_outside(ui::input_router_t&, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		if (btn == ui::mouse_button_e::left)
			popup.close_popup(true);
	}

	void editor_popup_controller_t::on_input_submitted(const char*, f32, void* user_data)
	{
		static_cast<editor_popup_controller_t*>(user_data)->close_popup(true);
	}

	void editor_popup_controller_t::on_asset_list_tick(ui::ui_context& ui, ui::widget_id_t, f32, void* user_data)
	{
		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		if (popup._asset_scroll_pending_frames == 0)
			return;
		if (!popup._visible || popup._mode != popup_mode_e::assets)
		{
			popup._asset_scroll_pending_frames = 0;
			return;
		}
		if (popup._asset_scroll_pending_frames > 1)
		{
			popup._asset_scroll_pending_frames--;
			return;
		}

		const editor_theme_t&	 theme		= editor_theme_t::get();
		const ui::layout_tree_t& tree		= ui.get_tree();
		const ui::layout_out_t&	 list_out	= tree.out(popup._asset_list_area);
		const f32				 ui_scale	= ui.get_ui_scale() > 0.0f ? ui.get_ui_scale() : 1.0f;
		const f32				 viewport	= list_out.size.y / ui_scale;
		const f32				 row_center = (static_cast<f32>(popup._asset_scroll_target) + 0.5f) * theme.item_height;
		const f32				 target		= math::clamp(row_center - viewport * 0.5f, 0.0f, list_out.max_scroll.y);
		popup._asset_scrollbar.set_scroll_y_immediate(-target);
		popup._asset_scroll_pending_frames = 0;
	}
}
