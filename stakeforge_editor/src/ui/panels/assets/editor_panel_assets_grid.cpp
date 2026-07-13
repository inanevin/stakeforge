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
#include "ui/panels/assets/editor_panel_assets.hpp"
#include "ui/panels/assets/editor_panel_assets_internal.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_thumbnail.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "editor_project.hpp"
#include "assets/editor_asset_manager.hpp"

#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_assets_t::refresh_asset_grid(bool force)
	{
		if (!can_mutate_ui_topology())
		{
			_asset_grid_refresh_pending = true;
			_asset_grid_refresh_force |= force;
			request_ui_mutation();
			return;
		}

		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 asset_tree	   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t folder		   = _selected_folder_node;
		const bool						 folder_valid  = !folder.is_null() && asset_tree.is_valid(folder);
		const u64						 folder_hash   = folder_valid ? _selected_folder_hash : 0;

		if (!force && _asset_grid_generation == asset_manager.get_generation() && _asset_grid_folder_hash == folder_hash)
			return;

		const bool preserve_scroll = _asset_grid_folder_hash == folder_hash;
		f32		   scroll_y		   = 0.0f;
		if (preserve_scroll)
			scroll_y = _ui->get_tree().in_const(_assets_body_pane_mid).scroll_offset.y;

		_asset_grid_generation	= asset_manager.get_generation();
		_asset_grid_folder_hash = folder_hash;

		clear_asset_grid(!preserve_scroll);
		if (preserve_scroll)
			restore_asset_grid_scroll(scroll_y);

		const ui::layout_out_t& body_out = _ui->get_tree().out(_assets_body_pane_mid);
		if (asset_tree.empty() || !folder_valid || body_out.size.x <= 0.0f)
			return;

		const editor_theme_t& theme			= editor_theme_t::get();
		const f32			  scale			= ui::get_valid_scale(_ui->get_ui_scale());
		const f32			  body_width	= body_out.size.x / scale;
		const f32			  content_width = math::max(0.0f, body_width - theme.margin_horizontal * 2.0f);
		const vec2f_t		  item_size		= {theme.item_height * 4.5f, theme.item_height * 6.5f};
		const f32			  slot_width	= item_size.x + theme.item_spacing;
		const u32			  column_count	= math::max(1u, static_cast<u32>((content_width + theme.item_spacing) / slot_width));

		u32						   column_index = 0;
		ui::widget_id_t			   row			= NULL_WIDGET;
		editor_asset_node_handle_t child		= asset_tree.first_child(folder);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = asset_tree.value(child);
			if (child_node.type != editor_asset_node_type_e::folder && (child_node.flags & editor_asset_node_flag_hidden) == 0)
			{
				if (!_show_file_assets && child_node.type == editor_asset_node_type_e::file)
				{
					child = asset_tree.next_sibling(child);
					continue;
				}

				if (_asset_favourites_only && !is_asset_favourite(get_asset_guid(child)))
				{
					child = asset_tree.next_sibling(child);
					continue;
				}

				if (!_asset_search_str_lower.empty())
				{
					frame_string_t<char> name_lower;
					name_lower.assign(child_node.name.c_str(), child_node.name.size());
					string_util::to_lower(name_lower);
					if (name_lower.find(_asset_search_str_lower.c_str()) == frame_string_t<char>::npos)
					{
						child = asset_tree.next_sibling(child);
						continue;
					}
				}

				if (_asset_item_style == asset_item_style_e::list)
				{
					append_asset_list_item(child);
					child = asset_tree.next_sibling(child);
					continue;
				}

				if (column_index == 0)
				{
					row = _ui->allocate_widget();
					_ui->set_widget_debug_name(row, "asset_grid_row");
					_ui->get_tree().attach(_assets_body_pane_mid, row);
					_ui->get_tree().draw_order(row) = _ui->get_tree().draw_order_const(_assets_body_pane_mid) + 1;

					ui::layout_in_t& row_in = _ui->get_tree().in(row);
					row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
					row_in.size_mode_y		= ui::axis_mode_e::fixed;
					row_in.size_value		= {1.0f, item_size.y};
					row_in.flow				= ui::flow_e::row;
					row_in.child_spacing	= theme.item_spacing;

					_asset_grid_rows.push_back(row);
				}

				append_asset_grid_item(row, child, item_size);
				column_index = (column_index + 1) % column_count;
			}
			child = asset_tree.next_sibling(child);
		}

		for (size_t i = 0; i < _selected_asset_nodes.size();)
		{
			if (find_visible_asset_index(_selected_asset_nodes[i]) == SIZE_MAX)
				_selected_asset_nodes.erase(_selected_asset_nodes.begin() + i);
			else
				++i;
		}
		_selected_asset_node = _selected_asset_nodes.empty() ? editor_asset_node_handle_t{} : _selected_asset_nodes.back();
		if (!_asset_selection_anchor.is_null() && find_visible_asset_index(_asset_selection_anchor) == SIZE_MAX)
			_asset_selection_anchor = _selected_asset_node;
	}

	void editor_panel_assets_t::clear_asset_grid(bool reset_scroll)
	{
		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui);
		if (tooltip_controller != nullptr)
		{
			for (const asset_grid_item_t& item : _asset_grid_items)
				tooltip_controller->clear_tooltip(item.root);
		}

		for (asset_grid_item_t& item : _asset_grid_items)
		{
			if (item.thumbnail != nullptr)
			{
				item.thumbnail->uninit();
				delete item.thumbnail;
			}
		}
		for (ui::widget_id_t row : _asset_grid_rows)
			_ui->deallocate_widget(row);
		_asset_grid_rows.resize(0);
		_asset_grid_items.resize(0);
		if (reset_scroll)
		{
			_grid_scroll_restore_pending = false;
			_ui->clear_post_layout_tick(_assets_body_pane_mid);
			_right_scrollbar.set_scroll_y_immediate(0.0f);
		}
	}

	void editor_panel_assets_t::restore_asset_grid_scroll(f32 scroll_y)
	{
		_grid_restore_scroll_y		 = scroll_y;
		_grid_scroll_restore_pending = true;
		_ui->set_post_layout_tick(_assets_body_pane_mid, on_asset_grid_scroll_restore_tick, this);
	}

	void editor_panel_assets_t::apply_pending_asset_grid_scroll_restore()
	{
		if (!_grid_scroll_restore_pending)
		{
			_ui->clear_post_layout_tick(_assets_body_pane_mid);
			return;
		}

		_right_scrollbar.set_scroll_y_immediate(_grid_restore_scroll_y);
		_ui->request_post_layout_solve();
		_grid_restore_scroll_y		 = 0.0f;
		_grid_scroll_restore_pending = false;
		_ui->clear_post_layout_tick(_assets_body_pane_mid);
	}

	void editor_panel_assets_t::update_current_directory_label()
	{
		const editor_asset_node_handle_t folder = _selected_folder_node;
		const editor_asset_tree_t&		 tree	= editor_asset_manager_t::get().get_asset_tree();

		if (folder.is_null() || !tree.is_valid(folder))
		{
			_ui->set_widget_text(_assets_body_pane_path, "");
			_ui->get_paint().set_text(
				_assets_body_pane_path,
				_ui->widget_text(_assets_body_pane_path),
				_ui->widget_text_len(_assets_body_pane_path),
				{.font = editor_theme_t::get().font_default, .color = editor_theme_t::get().color_text1, .point_size = editor_theme_t::get().text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			update_import_button_state();
			return;
		}

		const string_t& folder_path = tree.value(folder).full_path;
		SFG_ASSERT(!folder_path.empty());

		const string_t project_directory = file_system_t::get_directory_of_file(editor_project_t::get()._runtime.path.c_str());
		string_t	   relative_path	 = file_system_t::get_relative(project_directory.c_str(), folder_path.c_str());
		if (relative_path.empty())
			relative_path = "assets";

		frame_string_t<char> label;
		label += '/';
		label.append(relative_path.c_str(), relative_path.size());
		_ui->set_widget_text(_assets_body_pane_path, label.c_str());
		_ui->get_paint().set_text(
			_assets_body_pane_path,
			_ui->widget_text(_assets_body_pane_path),
			_ui->widget_text_len(_assets_body_pane_path),
			{.font = editor_theme_t::get().font_default, .color = editor_theme_t::get().color_text1, .point_size = editor_theme_t::get().text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		update_import_button_state();
	}

	void editor_panel_assets_t::update_import_button_state()
	{
		_import_button.set_disabled(_selected_folder_node.is_null());
	}

	void editor_panel_assets_t::append_asset_grid_item(ui::widget_id_t row, editor_asset_node_handle_t node, const vec2f_t& item_size)
	{
		ui::ui_context&					 ui			= *_ui;
		ui::layout_tree_t&				 tree		= ui.get_tree();
		ui::paint_layer_t&				 paint		= ui.get_paint();
		const editor_theme_t&			 theme		= editor_theme_t::get();
		const editor_asset_node_t&		 asset_node = editor_asset_manager_t::get().get_asset_tree().value(node);
		const editor_asset_t*			 asset		= asset_node.type == editor_asset_node_type_e::asset ? editor_asset_manager_t::get().find_asset(asset_node.asset_id) : nullptr;
		const editor_asset_descriptor_t* descriptor = nullptr;
		if (asset != nullptr)
		{
			const auto& descriptors	  = editor_asset_manager_t::get().get_asset_descriptors();
			const auto	descriptor_it = descriptors.find(asset->asset_type);
			descriptor				  = descriptor_it != descriptors.end() ? &descriptor_it->second : nullptr;
		}
		const char*	  type_text	 = descriptor != nullptr && !descriptor->display_name.empty() ? descriptor->display_name.c_str() : "File";
		const vec4f_t item_color = descriptor != nullptr ? descriptor->color : theme.color_outline_light;
		const bool	  has_status = asset != nullptr && asset->status != editor_asset_status_e::ok;

		asset_grid_item_t item = {};
		item.node			   = node;

		item.root = ui.allocate_widget();
		ui.set_widget_debug_name(item.root, "asset_grid_item");
		tree.attach(row, item.root);
		tree.draw_order(item.root) = tree.draw_order_const(row) + 1;

		ui::layout_in_t& root_in = tree.in(item.root);
		root_in.flags |= ui::wf_input | ui::wf_focusable;

		root_in.size_mode_x	  = ui::axis_mode_e::fixed;
		root_in.size_mode_y	  = ui::axis_mode_e::fixed;
		root_in.size_value	  = item_size;
		root_in.flow		  = ui::flow_e::column;
		root_in.child_spacing = 0.0f;

		ui::vg_rect_paint_t root_rect = {};
		root_rect.fill_color_a		  = theme.color_panel;
		root_rect.fill_color_b		  = theme.color_panel;
		root_rect.rounding			  = theme.item_rounding;
		root_rect.rounding_segs		  = 4;
		paint.set_rect(item.root, root_rect);
		paint.set_hover_color(item.root, theme.color_panel_light);
		paint.set_press_color(item.root, theme.color_light);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_asset_grid_item_clicked;
		listener.on_double_click	   = on_asset_grid_item_double_clicked;
		listener.on_drag_begin		   = on_asset_grid_item_drag_begin;
		listener.on_focus_gain		   = on_asset_item_focus_gain;
		listener.on_focus_lose		   = on_asset_item_focus_lost;
		ui.get_input().set_listener(item.root, listener);

		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(ui);
		if (tooltip_controller != nullptr)
		{
			editor_tooltip_desc_t tooltip = {};
			tooltip.text				  = asset_node.name.c_str();
			tooltip_controller->set_tooltip(item.root, tooltip);
		}

		item.thumbnail_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.thumbnail_frame, "asset_grid_item_thumbnail");
		tree.attach(item.root, item.thumbnail_frame);

		ui::layout_in_t& thumbnail_in = tree.in(item.thumbnail_frame);
		thumbnail_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		thumbnail_in.size_mode_y	  = ui::axis_mode_e::copy_other;
		thumbnail_in.size_value		  = {1.0f, 1.0f};

		ui::vg_rect_paint_t thumbnail_rect = {};
		thumbnail_rect.fill_color_a		   = theme.color_frame;
		thumbnail_rect.fill_color_b		   = theme.color_frame;
		thumbnail_rect.rounding			   = theme.item_rounding;
		thumbnail_rect.rounding_segs	   = 4;
		paint.set_rect(item.thumbnail_frame, thumbnail_rect);

		item.thumbnail = new editor_widget_thumbnail_t();
		item.thumbnail->init(ui, item.thumbnail_frame, {.thumbnail = asset != nullptr ? asset->thumbnail_guid : NULL_SID});

		item.status_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.status_text, "asset_grid_item_status");
		tree.attach(item.thumbnail_frame, item.status_text);

		ui::layout_in_t& status_in		  = tree.in(item.status_text);
		status_in.flags					  = ui::wf_overlay;
		status_in.pos_mode_x			  = ui::pos_mode_e::relative_in_parent;
		status_in.pos_mode_y			  = ui::pos_mode_e::relative_in_parent;
		status_in.pos_value				  = {0.0f, 1.0f};
		status_in.anchor_x				  = ui::anchor_e::start;
		status_in.anchor_y				  = ui::anchor_e::end;
		status_in.size_mode_x			  = ui::axis_mode_e::fixed;
		status_in.size_mode_y			  = ui::axis_mode_e::fixed;
		status_in.size_value			  = {theme.item_area_height, theme.item_area_height};
		tree.draw_order(item.status_text) = tree.draw_order_const(item.thumbnail_frame) + 1;
		tree.set_visible(item.status_text, has_status, false);

		ui.set_widget_text(item.status_text, ICON_WARN);
		paint.set_text(item.status_text,
					   ui.widget_text(item.status_text),
					   ui.widget_text_len(item.status_text),
					   {.font = theme.font_icons, .color = theme.color_accent_warn, .point_size = theme.icon_default_px_size * 1.5f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.star_text, "asset_grid_item_star");
		tree.attach(item.thumbnail_frame, item.star_text);

		ui::layout_in_t& star_in		= tree.in(item.star_text);
		star_in.flags					= ui::wf_overlay;
		star_in.pos_mode_x				= ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y				= ui::pos_mode_e::relative_in_parent;
		star_in.pos_value				= {1.0f, 1.0f};
		star_in.anchor_x				= ui::anchor_e::end;
		star_in.anchor_y				= ui::anchor_e::end;
		star_in.size_mode_x				= ui::axis_mode_e::fixed;
		star_in.size_mode_y				= ui::axis_mode_e::fixed;
		star_in.size_value				= {theme.item_area_height, theme.item_area_height};
		tree.draw_order(item.star_text) = tree.draw_order_const(item.root) + 1;
		tree.set_visible(item.star_text, is_asset_favourite(get_asset_guid(node)), false);

		ui.set_widget_text(item.star_text, ICON_STAR);
		paint.set_text(item.star_text,
					   ui.widget_text(item.star_text),
					   ui.widget_text_len(item.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size * 1.5f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.info_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.info_frame, "asset_grid_item_info");
		tree.attach(item.root, item.info_frame);

		ui::layout_in_t& info_in = tree.in(item.info_frame);
		info_in.child_clip_mode	 = ui::clip_mode_e::cpu_rect;
		info_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		info_in.size_mode_y		 = ui::axis_mode_e::fill;
		info_in.size_value		 = {1.0f, 0.0f};
		info_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal * 0.5f, theme.margin_vertical, theme.margin_horizontal * 0.5f};

		const bool			selected		   = is_asset_selected(item.node);
		ui::vg_rect_paint_t info_rect		   = {};
		const vec4f_t		selected_color	   = _focused ? theme.color_accent0 : theme.color_outline_light;
		const vec4f_t		selected_color_dim = _focused ? theme.color_accent0_dim : theme.color_outline_light;
		info_rect.fill_color_a				   = selected ? selected_color : theme.color_frame;
		info_rect.fill_color_b				   = selected ? selected_color_dim : theme.color_frame;
		info_rect.gradient					   = selected ? ui::vg_gradient_e::vertical : ui::vg_gradient_e::none;
		paint.set_rect(item.info_frame, info_rect);

		item.color_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.color_frame, "asset_grid_item_color");
		tree.attach(item.root, item.color_frame);

		ui::layout_in_t& color_in = tree.in(item.color_frame);
		color_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		color_in.size_mode_y	  = ui::axis_mode_e::fixed;
		color_in.size_value		  = {1.0f, theme.border_thickness};

		ui::vg_rect_paint_t color_rect = {};
		color_rect.fill_color_a		   = item_color;
		color_rect.fill_color_b		   = item_color;
		paint.set_rect(item.color_frame, color_rect);

		item.label = ui.allocate_widget();
		ui.set_widget_debug_name(item.label, "asset_grid_item_label");
		tree.attach(item.info_frame, item.label);

		ui::layout_in_t& label_in = tree.in(item.label);
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::flow;

		ui.set_widget_text(item.label, asset_node.name.c_str());
		paint.set_text(item.label,
					   ui.widget_text(item.label),
					   ui.widget_text_len(item.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.type_label = ui.allocate_widget();
		ui.set_widget_debug_name(item.type_label, "asset_grid_item_type_label");
		tree.attach(item.info_frame, item.type_label);

		ui::layout_in_t& type_label_in = tree.in(item.type_label);
		type_label_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		type_label_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		type_label_in.pos_value		   = {0.5f, 1.0f};
		type_label_in.anchor_x		   = ui::anchor_e::center;
		type_label_in.anchor_y		   = ui::anchor_e::end;

		ui.set_widget_text(item.type_label, type_text);
		paint.set_text(item.type_label,
					   ui.widget_text(item.type_label),
					   ui.widget_text_len(item.type_label),
					   {.font = theme.font_title, .color = theme.color_text2, .point_size = theme.text_small_title_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_asset_grid_items.push_back(item);
	}

	void editor_panel_assets_t::append_asset_list_item(editor_asset_node_handle_t node)
	{
		ui::ui_context&					 ui			= *_ui;
		ui::layout_tree_t&				 tree		= ui.get_tree();
		ui::paint_layer_t&				 paint		= ui.get_paint();
		const editor_theme_t&			 theme		= editor_theme_t::get();
		const editor_asset_node_t&		 asset_node = editor_asset_manager_t::get().get_asset_tree().value(node);
		const editor_asset_t*			 asset		= asset_node.type == editor_asset_node_type_e::asset ? editor_asset_manager_t::get().find_asset(asset_node.asset_id) : nullptr;
		const editor_asset_descriptor_t* descriptor = nullptr;
		if (asset != nullptr)
		{
			const auto& descriptors	  = editor_asset_manager_t::get().get_asset_descriptors();
			const auto	descriptor_it = descriptors.find(asset->asset_type);
			descriptor				  = descriptor_it != descriptors.end() ? &descriptor_it->second : nullptr;
		}
		const vec4f_t item_color = descriptor != nullptr ? descriptor->color : theme.color_outline_light;
		const bool	  has_status = asset != nullptr && asset->status != editor_asset_status_e::ok;

		asset_grid_item_t item = {};
		item.node			   = node;

		item.root = ui.allocate_widget();
		ui.set_widget_debug_name(item.root, "asset_list_item");
		tree.attach(_assets_body_pane_mid, item.root);
		tree.draw_order(item.root) = tree.draw_order_const(_assets_body_pane_mid) + 1;

		ui::layout_in_t& root_in = tree.in(item.root);
		root_in.flags |= ui::wf_input | ui::wf_focusable;

		root_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y	  = ui::axis_mode_e::fixed;
		root_in.size_value	  = {1.0f, theme.item_height};
		root_in.flow		  = ui::flow_e::row;
		root_in.child_spacing = theme.item_spacing * 0.5f;
		root_in.child_margins = {0.0f, theme.item_height * 2.0f, 0.0f, 0.0f};

		const bool			selected		   = _selected_asset_node == item.node;
		ui::vg_rect_paint_t root_rect		   = {};
		const vec4f_t		selected_color	   = _focused ? theme.color_accent0 : theme.color_outline_light;
		const vec4f_t		selected_color_dim = _focused ? theme.color_accent0_dim : theme.color_outline_light;
		root_rect.fill_color_a				   = selected ? selected_color_dim : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		root_rect.fill_color_b				   = selected ? selected_color : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		root_rect.gradient					   = selected ? ui::vg_gradient_e::horizontal : ui::vg_gradient_e::none;
		paint.set_rect(item.root, root_rect);
		paint.set_hover_color(item.root, selected ? selected_color : theme.color_panel_light);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_asset_grid_item_clicked;
		listener.on_double_click	   = on_asset_grid_item_double_clicked;
		listener.on_drag_begin		   = on_asset_grid_item_drag_begin;
		listener.on_focus_gain		   = on_asset_item_focus_gain;
		listener.on_focus_lose		   = on_asset_item_focus_lost;
		ui.get_input().set_listener(item.root, listener);

		editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(ui);
		if (tooltip_controller != nullptr)
		{
			editor_tooltip_desc_t tooltip = {};
			tooltip.text				  = asset_node.name.c_str();
			tooltip_controller->set_tooltip(item.root, tooltip);
		}

		item.info_frame = item.root;

		item.color_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.color_frame, "asset_list_item_color");
		tree.attach(item.root, item.color_frame);

		ui::layout_in_t& color_in = tree.in(item.color_frame);
		color_in.size_mode_x	  = ui::axis_mode_e::fixed;
		color_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		color_in.size_value		  = {theme.border_thickness, 1.0f};

		ui::vg_rect_paint_t color_rect = {};
		color_rect.fill_color_a		   = item_color;
		color_rect.fill_color_b		   = item_color;
		paint.set_rect(item.color_frame, color_rect);

		item.thumbnail_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.thumbnail_frame, "asset_list_item_thumbnail");
		tree.attach(item.root, item.thumbnail_frame);

		ui::layout_in_t& thumbnail_in = tree.in(item.thumbnail_frame);
		thumbnail_in.size_mode_x	  = ui::axis_mode_e::copy_other;
		thumbnail_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		thumbnail_in.size_value		  = {1.0f, 0.85f};
		thumbnail_in.anchor_y		  = ui::anchor_e::center;
		thumbnail_in.pos_value.y	  = 0.5f;
		thumbnail_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;

		ui::vg_rect_paint_t thumbnail_rect = {};
		thumbnail_rect.fill_color_a		   = theme.color_frame;
		thumbnail_rect.fill_color_b		   = theme.color_frame;
		thumbnail_rect.rounding			   = theme.item_rounding;
		thumbnail_rect.rounding_segs	   = 4;
		paint.set_rect(item.thumbnail_frame, thumbnail_rect);

		item.thumbnail = new editor_widget_thumbnail_t();
		item.thumbnail->init(ui, item.thumbnail_frame, {.thumbnail = asset != nullptr ? asset->thumbnail_guid : NULL_SID});

		item.label = ui.allocate_widget();
		ui.set_widget_debug_name(item.label, "asset_list_item_label");
		tree.attach(item.root, item.label);

		ui::layout_in_t& label_in = tree.in(item.label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fill;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.text_default_px_size};

		ui.set_widget_text(item.label, asset_node.name.c_str());
		paint.set_text(item.label,
					   ui.widget_text(item.label),
					   ui.widget_text_len(item.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.status_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.status_text, "asset_list_item_status");
		tree.attach(item.root, item.status_text);

		ui::layout_in_t& status_in = tree.in(item.status_text);
		status_in.flags			   = ui::wf_overlay;
		status_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		status_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		status_in.pos_value		   = {1.0f, 0.5f};
		status_in.anchor_x		   = ui::anchor_e::end;
		status_in.anchor_y		   = ui::anchor_e::center;
		status_in.size_mode_x	   = ui::axis_mode_e::fixed;
		status_in.size_mode_y	   = ui::axis_mode_e::fixed;
		status_in.size_value	   = {theme.item_height, theme.item_height};
		const f32 root_w		   = _ui->get_tree().out(_assets_body_pane_mid).size.x;
		if (root_w > 0.0f)
			status_in.pos_value.x = 1.0f - theme.item_height / root_w;
		tree.set_visible(item.status_text, has_status, false);

		ui.set_widget_text(item.status_text, ICON_WARN);
		paint.set_text(item.status_text,
					   ui.widget_text(item.status_text),
					   ui.widget_text_len(item.status_text),
					   {.font = theme.font_icons, .color = theme.color_accent_warn, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.star_text, "asset_list_item_star");
		tree.attach(item.root, item.star_text);

		ui::layout_in_t& star_in		= tree.in(item.star_text);
		star_in.flags					= ui::wf_overlay;
		star_in.pos_mode_x				= ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y				= ui::pos_mode_e::relative_in_parent;
		star_in.pos_value				= {1.0f, 0.5f};
		star_in.anchor_x				= ui::anchor_e::end;
		star_in.anchor_y				= ui::anchor_e::center;
		star_in.size_mode_x				= ui::axis_mode_e::fixed;
		star_in.size_mode_y				= ui::axis_mode_e::fixed;
		star_in.size_value				= {theme.item_height, theme.item_height};
		tree.draw_order(item.star_text) = tree.draw_order_const(item.root) + 1;
		tree.set_visible(item.star_text, is_asset_favourite(get_asset_guid(node)), false);

		ui.set_widget_text(item.star_text, ICON_STAR);
		paint.set_text(item.star_text,
					   ui.widget_text(item.star_text),
					   ui.widget_text_len(item.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_asset_grid_rows.push_back(item.root);
		_asset_grid_items.push_back(item);
	}

	void editor_panel_assets_t::refresh_asset_grid_item_backgrounds()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			const bool	  selected			 = is_asset_selected(item.node);
			const vec4f_t selected_color	 = _focused ? theme.color_accent0 : theme.color_outline_light;
			const vec4f_t selected_color_dim = _focused ? theme.color_accent0_dim : theme.color_outline_light;

			ui::vg_rect_paint_t info_rect = {};
			if (_asset_item_style == asset_item_style_e::list)
			{
				info_rect.fill_color_a = selected ? selected_color : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
				info_rect.fill_color_b = selected ? selected_color_dim : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
				_ui->get_paint().set_hover_color(item.root, selected ? selected_color : theme.color_panel_light);
			}
			else
			{
				info_rect.fill_color_a = selected ? selected_color : theme.color_frame;
				info_rect.fill_color_b = selected ? selected_color_dim : theme.color_frame;
			}
			info_rect.gradient = selected ? ui::vg_gradient_e::horizontal : ui::vg_gradient_e::none;
			_ui->get_paint().set_rect(item.info_frame, info_rect);
		}
	}

	void editor_panel_assets_t::refresh_asset_favourite_icons()
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			const bool favourite = is_asset_favourite(get_asset_guid(item.node));
			tree.set_visible(item.star_text, favourite, false);
		}
	}

}
