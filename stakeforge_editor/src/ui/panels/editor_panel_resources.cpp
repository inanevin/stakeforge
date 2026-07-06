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
#include "ui/panels/editor_panel_resources.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/file_system.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <algorithm>
#include <string>

namespace sfg
{
	namespace
	{
		void paint_label(ui::ui_context& ui, ui::widget_id_t label, const vec4f_t& color)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui.get_paint().set_text(
				label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = color, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		ui::widget_id_t add_table_cell(ui::ui_context& ui, ui::widget_id_t parent, const char* text, const vec4f_t& text_color)
		{
			ui::layout_tree_t&	  tree	= ui.get_tree();
			const editor_theme_t& theme = editor_theme_t::get();

			const ui::widget_id_t cell = ui.allocate_widget();
			ui.set_widget_debug_name(cell, "resources_cell");
			tree.attach(parent, cell);

			ui::layout_in_t& cell_in = tree.in(cell);
			cell_in.flags			 = ui::wf_visible;
			cell_in.size_mode_x		 = ui::axis_mode_e::fill;
			cell_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
			cell_in.size_value		 = {1.0f, 1.0f};
			cell_in.child_clip_mode	 = ui::clip_mode_e::cpu_rect;
			cell_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
			cell_in.flow			 = ui::flow_e::row;

			const ui::widget_id_t label = ui.allocate_widget();
			ui.set_widget_debug_name(label, "resources_cell_label");
			tree.attach(cell, label);

			ui::layout_in_t& label_in = tree.in(label);
			label_in.flags			  = ui::wf_visible;
			label_in.pos_mode_x		  = ui::pos_mode_e::flow;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.y	  = 0.5f;
			label_in.anchor_y		  = ui::anchor_e::center;

			ui.set_widget_text(label, text != nullptr ? text : "");
			paint_label(ui, label, text_color);
			return cell;
		}

		void add_table_divider(ui::ui_context& ui, ui::widget_id_t parent)
		{
			ui::layout_tree_t&	  tree	= ui.get_tree();
			ui::paint_layer_t&	  paint = ui.get_paint();
			const editor_theme_t& theme = editor_theme_t::get();

			const ui::widget_id_t divider = ui.allocate_widget();
			ui.set_widget_debug_name(divider, "resources_divider");
			tree.attach(parent, divider);

			ui::layout_in_t& divider_in = tree.in(divider);
			divider_in.flags			= ui::wf_visible;
			divider_in.size_mode_x		= ui::axis_mode_e::fixed;
			divider_in.size_mode_y		= ui::axis_mode_e::parent_relative;
			divider_in.size_value		= {theme.divider_thickness, 1.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_outline_light;
			rect.fill_color_b		 = theme.color_outline_light;
			paint.set_rect(divider, rect);
		}

		ui::widget_id_t add_row_divider(ui::ui_context& ui, ui::widget_id_t parent)
		{
			ui::layout_tree_t&	  tree	= ui.get_tree();
			ui::paint_layer_t&	  paint = ui.get_paint();
			const editor_theme_t& theme = editor_theme_t::get();

			const ui::widget_id_t divider = ui.allocate_widget();
			ui.set_widget_debug_name(divider, "resources_row_divider");
			tree.attach(parent, divider);

			ui::layout_in_t& divider_in = tree.in(divider);
			divider_in.flags			= ui::wf_visible;
			divider_in.size_mode_x		= ui::axis_mode_e::parent_relative;
			divider_in.size_mode_y		= ui::axis_mode_e::fixed;
			divider_in.size_value		= {1.0f, theme.divider_thickness};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_outline_light;
			rect.fill_color_b		 = theme.color_outline_light;
			paint.set_rect(divider, rect);
			return divider;
		}

		void add_table_cells(ui::ui_context& ui, ui::widget_id_t row, const char* const* labels, const vec4f_t& text_color)
		{
			for (u32 i = 0; i < 6; ++i)
			{
				add_table_cell(ui, row, labels[i], text_color);
				if (i != 5)
					add_table_divider(ui, row);
			}
		}
	}

	editor_panel_resources_t::editor_panel_resources_t()
	{
		set_type(editor_panel_type_e::resources);
		set_title(editor_panel_type_to_string(editor_panel_type_e::resources));
		set_icon(ICON_TRIANGLE);
	}

	void editor_panel_resources_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		_header = ui.allocate_widget();
		ui.set_widget_debug_name(_header, "resources_header");
		tree.attach(_root, _header);

		ui::layout_in_t& header_in = tree.in(_header);
		header_in.flags			   = ui::wf_visible;
		header_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		header_in.size_mode_y	   = ui::axis_mode_e::fixed;
		header_in.size_value	   = {1.0f, theme.item_area_height};
		header_in.flow			   = ui::flow_e::row;

		ui::vg_rect_paint_t header_rect = {};
		header_rect.fill_color_a		= theme.color_panel_light;
		header_rect.fill_color_b		= theme.color_panel_light;
		header_rect.outline_color		= theme.color_outline;
		header_rect.outline_thickness	= theme.outline_thickness;
		paint.set_rect(_header, header_rect);

		const char* const header_labels[] = {"Name", "Type", "GUID", "Cache Size", "Ref Count", "State"};
		add_table_cells(ui, _header, header_labels, theme.color_text0);

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "resources_body");
		tree.attach(_root, _body);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		body_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_in.size_value		 = {1.0f, 1.0f};
		body_in.flow			 = ui::flow_e::column;
		body_in.child_spacing	 = 0.0f;

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _body;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_scrollbar.init(ui, scrollbar_config);

		_rows.reserve(128);
		_row_sources.reserve(128);
		_resource_generation = resource_manager_t::get().get_generation();
		ui.set_pre_layout_tick(_body, on_resources_tick, this);
		refresh_rows();
	}

	void editor_panel_resources_t::uninit()
	{
		_ui->clear_pre_layout_tick(_body);
		_scrollbar.uninit();
		clear_rows();
		_row_sources.resize(0);
		_header				 = NULL_WIDGET;
		_body				 = NULL_WIDGET;
		_resource_generation = 0;
		_refresh_tick		 = 0;
		editor_panel_t::uninit();
	}

	void editor_panel_resources_t::refresh_rows()
	{
		clear_rows();

		const resource_manager_t& resource_manager = resource_manager_t::get();
		_row_sources.resize(0);
		for (const auto& pair : resource_manager.get_entries())
			_row_sources.push_back({.entry = &pair.second, .hash = pair.first});

		std::sort(_row_sources.begin(), _row_sources.end(), [](const resource_row_source_t& lhs, const resource_row_source_t& rhs) {
			const u8 lhs_type = static_cast<u8>(lhs.entry->type);
			const u8 rhs_type = static_cast<u8>(rhs.entry->type);
			if (lhs_type != rhs_type)
				return lhs_type < rhs_type;
			return lhs.hash < rhs.hash;
		});

		for (const resource_row_source_t& source : _row_sources)
			add_row(source.hash, *source.entry);

		_resource_generation = resource_manager.get_generation();
	}

	void editor_panel_resources_t::clear_rows()
	{
		for (const resource_row_t& row : _rows)
		{
			if (row.divider != NULL_WIDGET)
				_ui->deallocate_widget(row.divider);
			_ui->deallocate_widget(row.root);
		}
		_rows.resize(0);
	}

	void editor_panel_resources_t::add_row(sid_t hash, const resource_entry_t& entry)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		resource_row_t& row = _rows.emplace_back();
		if (_rows.size() > 1)
			row.divider = add_row_divider(*_ui, _body);

		row.root = _ui->allocate_widget();
		_ui->set_widget_debug_name(row.root, "resources_row");
		tree.attach(_body, row.root);

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.flags			= ui::wf_visible;
		row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value		= {1.0f, theme.item_area_height};
		row_in.flow				= ui::flow_e::row;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = theme.color_frame;
		row_rect.fill_color_b		 = theme.color_frame;
		paint.set_rect(row.root, row_rect);

		const editor_asset_t* asset		   = editor_asset_manager_t::get().find_asset(hash);
		const char*			  display_name = editor_asset_util_t::find_asset_display_name(hash);
		const char*			  debug_name   = resource_manager_t::get().get_memory().get_text(entry.debug_name);
		const string_t		  hash_text	   = std::to_string(hash);
		const char*			  name		   = display_name != nullptr ? display_name : (debug_name != nullptr && debug_name[0] != '\0' ? debug_name : hash_text.c_str());

		const editor_asset_descriptor_t* descriptor = nullptr;
		if (asset != nullptr)
		{
			const auto desc_it = editor_asset_manager_t::get().get_asset_descriptors().find(asset->asset_type);
			if (desc_it != editor_asset_manager_t::get().get_asset_descriptors().end())
				descriptor = &desc_it->second;
		}

		string_t cache_path = asset != nullptr ? editor_asset_util_t::get_cache_path_for_asset(*asset) : editor_project_t::get()._runtime.cache_path + hash_text + ".sfg_bin";
		if (asset == nullptr && !file_system_t::exists(cache_path.c_str()))
			cache_path = editor_directories_t::get_editor_resource_cache() + hash_text + ".sfg_bin";

		const char*	   type_name  = descriptor != nullptr ? descriptor->display_name.c_str() : resource_type_to_string(entry.type);
		const u64	   cache_size = file_system_t::exists(cache_path.c_str()) ? file_system_t::get_file_size(cache_path.c_str()) : 0;
		const string_t size_text  = std::to_string(cache_size) + " B";
		const string_t ref_text	  = std::to_string(entry.ref_count);

		const char* const labels[] = {name, type_name, hash_text.c_str(), size_text.c_str(), ref_text.c_str(), resource_state_to_string(entry.state)};
		add_table_cells(*_ui, row.root, labels, theme.color_text1);
	}

	void editor_panel_resources_t::on_resources_tick(ui::ui_context& ui, ui::widget_id_t id, f32, void* user_data)
	{
		if ((ui.get_tree().in_const(id).flags & ui::wf_visible) == 0)
			return;

		editor_panel_resources_t& panel = *static_cast<editor_panel_resources_t*>(user_data);
		panel._refresh_tick++;
		if (panel._refresh_tick < 60)
			return;

		panel._refresh_tick	 = 0;
		const u64 generation = resource_manager_t::get().get_generation();
		if (panel._resource_generation != generation)
			panel.refresh_rows();
	}
}
