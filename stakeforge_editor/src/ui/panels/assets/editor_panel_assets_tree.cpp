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
#include "assets/editor_asset_creator.hpp"
#include "assets/editor_asset_importer.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_string.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <algorithm>

namespace sfg
{
	namespace
	{
		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			u16 flags = 0;
			if (visible)
			{
				flags = ui::wf_visible;
				if (input)
					flags |= ui::wf_input;
			}
			tree.in(id).flags = flags;
		}
	}
	void editor_panel_assets_t::refresh_folder_rows()
	{
		if (!can_mutate_ui_topology())
		{
			_folder_rows_refresh_pending = true;
			request_ui_mutation();
			return;
		}

		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		_asset_tree_generation						= asset_manager.get_generation();
		_visible_folder_row_count					= 0;
		_selected_folder_node						= {};

		const editor_asset_node_handle_t root_handle = asset_manager.get_root_node();
		if (!asset_tree.empty() && !root_handle.is_null() && asset_tree.is_valid(root_handle))
		{
			_selected_folder_node			= _selected_folder_hash == 0 ? root_handle : editor_asset_node_handle_t{};
			const editor_asset_node_t& root = asset_tree.value(root_handle);
			frame_string_t<char>	   current_path;
			current_path.assign(root.name.c_str(), root.name.size());
			append_folder_rows(root_handle, 0, current_path);
		}

		for (size_t i = _visible_folder_row_count; i < _folder_rows.size(); ++i)
			set_folder_row_visible(_folder_rows[i], false);

		for (size_t i = 0; i < _selected_folder_hashes.size();)
		{
			if (find_visible_folder_index(_selected_folder_hashes[i]) == SIZE_MAX)
				_selected_folder_hashes.erase(_selected_folder_hashes.begin() + i);
			else
				++i;
		}
		_selected_folder_hash			 = _selected_folder_hashes.empty() ? 0 : _selected_folder_hashes.back();
		const folder_row_t* selected_row = find_row_by_hash(_selected_folder_hash);
		_selected_folder_node			 = selected_row != nullptr ? selected_row->node : editor_asset_node_handle_t{};
		if (_folder_selection_anchor != 0 && find_visible_folder_index(_folder_selection_anchor) == SIZE_MAX)
			_folder_selection_anchor = _selected_folder_hash;

		update_current_directory_label();
		update_import_button_state();
		refresh_asset_grid(true);
	}

	bool editor_panel_assets_t::append_folder_rows(editor_asset_node_handle_t node, u16 depth, frame_string_t<char>& current_path)
	{
		const editor_asset_tree_t& tree		  = editor_asset_manager_t::get().get_asset_tree();
		const editor_asset_node_t& asset_node = tree.value(node);
		if ((asset_node.flags & editor_asset_node_flag_hidden) != 0)
			return false;

		const u64 path_hash = hashing_t::hash_u64(current_path.c_str(), current_path.size());
		if (path_hash == _selected_folder_hash)
			_selected_folder_node = node;

		const bool search_active = !_search_str_lower.empty();
		const bool promoted		 = (asset_node.flags & editor_asset_node_flag_promoted) != 0;
		const bool favourite	 = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), path_hash) != _favourite_folder_hashes.end();
		const bool passes_filter = !_favourites_only || favourite || has_favourite_asset_descendant(node);

		bool self_matches_search = !search_active;
		if (search_active)
		{
			frame_string_t<char> name_lower;
			name_lower.assign(asset_node.name.c_str(), asset_node.name.size());
			string_util::to_lower(name_lower);
			self_matches_search = name_lower.find(_search_str_lower.c_str()) != frame_string_t<char>::npos;
		}

		const bool is_expanded	 = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), path_hash) != _expanded_folder_hashes.end();
		const bool may_emit_self = !promoted && passes_filter;
		const bool will_fold	 = may_emit_self && !search_active && !is_expanded;

		const size_t my_slot	 = may_emit_self ? _visible_folder_row_count++ : 0;
		const u16	 child_depth = static_cast<u16>(depth + (may_emit_self ? 1 : 0));
		bool		 any_visible = false;

		if (!will_fold)
		{
			editor_asset_node_handle_t child = tree.first_child(node);
			while (!child.is_null())
			{
				const editor_asset_node_t& child_node = tree.value(child);
				if (child_node.type == editor_asset_node_type_e::folder)
				{
					const size_t saved_len = current_path.size();
					current_path += '/';
					current_path.append(child_node.name.c_str(), child_node.name.size());
					any_visible |= append_folder_rows(child, child_depth, current_path);
					current_path.resize(saved_len);
				}
				child = tree.next_sibling(child);
			}
		}

		const bool should_emit_self = may_emit_self && (!search_active || self_matches_search || any_visible);
		if (may_emit_self && !should_emit_self)
			--_visible_folder_row_count;

		if (should_emit_self)
		{
			bool has_children = any_visible;
			if (will_fold)
			{
				editor_asset_node_handle_t child = tree.first_child(node);
				while (!child.is_null() && !has_children)
				{
					const editor_asset_node_t& child_node = tree.value(child);
					if (child_node.type == editor_asset_node_type_e::folder && (child_node.flags & editor_asset_node_flag_hidden) == 0)
						has_children = true;
					child = tree.next_sibling(child);
				}
			}

			folder_row_t& row = get_or_create_folder_row(my_slot);
			update_folder_row(row, node, asset_node.name.c_str(), depth, path_hash, has_children, will_fold, favourite);
		}

		return should_emit_self || any_visible;
	}

	editor_panel_assets_t::folder_row_t& editor_panel_assets_t::get_or_create_folder_row(size_t index)
	{
		if (index < _folder_rows.size())
			return _folder_rows[index];

		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		folder_row_t row = {};

		row.root = ui.allocate_widget();
		ui.set_widget_debug_name(row.root, "asset_folder_row");
		tree.attach(_assets_left_pane_body, row.root);
		tree.draw_order(row.root) = tree.draw_order_const(_assets_left_pane_body) + 1;

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.flags			= ui::wf_visible | ui::wf_input | ui::wf_focusable;
		row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value		= {1.0f, theme.item_height};
		row_in.flow				= ui::flow_e::row;
		row_in.child_spacing	= theme.item_spacing * 0.5f;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		paint.set_rect(row.root, row_rect);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_folder_row_clicked;
		listener.on_double_click	   = on_folder_row_double_clicked;
		listener.on_drag_begin		   = on_folder_row_drag_begin;
		listener.on_focus_gain		   = on_folder_row_focus_gain;
		listener.on_focus_lose		   = on_folder_row_focus_lost;
		ui.get_input().set_listener(row.root, listener);

		row.icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon, "asset_folder_row_icon_wrapper");
		tree.attach(row.root, row.icon);

		ui::layout_in_t& icon_in = tree.in(row.icon);
		icon_in.flags			 = ui::wf_visible;
		icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value.y		 = 0.5f;
		icon_in.anchor_y		 = ui::anchor_e::center;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;
		icon_in.size_value		 = {theme.item_height, theme.item_height};

		ui::listener_bundle_t icon_listener = {};
		icon_listener.user_data				= this;
		icon_listener.on_click				= on_folder_icon_clicked;
		ui.get_input().set_listener(row.icon, icon_listener);

		row.icon_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon_text, "asset_folder_row_icon");
		tree.attach(row.icon, row.icon_text);

		ui::layout_in_t& icon_text_in = tree.in(row.icon_text);
		icon_text_in.flags			  = ui::wf_visible;
		icon_text_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_value		  = {0.5f, 0.5f};
		icon_text_in.anchor_x		  = ui::anchor_e::center;
		icon_text_in.anchor_y		  = ui::anchor_e::center;
		icon_text_in.size_mode_x	  = ui::axis_mode_e::fixed;
		icon_text_in.size_mode_y	  = ui::axis_mode_e::fixed;

		row.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.star_text, "asset_folder_row_star");
		tree.attach(row.root, row.star_text);

		ui::layout_in_t& star_in = tree.in(row.star_text);
		star_in.flags			 = 0;
		star_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_value		 = {1.0f, 0.5f};
		star_in.anchor_x		 = ui::anchor_e::end;
		star_in.anchor_y		 = ui::anchor_e::center;
		star_in.size_mode_x		 = ui::axis_mode_e::fixed;
		star_in.size_mode_y		 = ui::axis_mode_e::fixed;
		star_in.size_value		 = {theme.item_height, theme.item_height};

		ui.set_widget_text(row.star_text, ICON_STAR);
		paint.set_text(row.star_text,
					   ui.widget_text(row.star_text),
					   ui.widget_text_len(row.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		row.label = ui.allocate_widget();
		ui.set_widget_debug_name(row.label, "asset_folder_row_label");
		tree.attach(row.root, row.label);

		ui::layout_in_t& label_in = tree.in(row.label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;

		_folder_rows.push_back(row);
		return _folder_rows.back();
	}

	void editor_panel_assets_t::update_folder_row(folder_row_t& row, editor_asset_node_handle_t node, const char* name, u16 depth, u64 path_hash, bool has_children, bool is_folded, bool is_favourite)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		row.node		 = node;
		row.path_hash	 = path_hash;
		row.depth		 = depth;
		row.has_children = has_children;
		row.is_favourite = is_favourite;

		set_folder_row_visible(row, true);
		update_folder_row_background(row);

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.child_margins	= {0.0f, theme.item_height, 0.0f, theme.margin_horizontal + static_cast<f32>(depth) * theme.indent_horizontal * ASSETS_FOLDER_INDENT_MULT};
		tree.in(row.icon).flags = has_children ? static_cast<u16>(ui::wf_visible | ui::wf_input) : static_cast<u16>(ui::wf_visible);

		const char* icon = has_children ? (is_folded ? ICON_DD_RIGHT : ICON_DD_DOWN) : "";
		_ui->set_widget_text(row.icon_text, icon);
		paint.set_text(row.icon_text,
					   _ui->widget_text(row.icon_text),
					   _ui->widget_text_len(row.icon_text),
					   {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.label, name);
		paint.set_text(row.label,
					   _ui->widget_text(row.label),
					   _ui->widget_text_len(row.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_assets_t::update_folder_row_background(const folder_row_t& row)
	{
		const editor_theme_t& theme				 = editor_theme_t::get();
		const bool			  selected			 = is_folder_selected(row.path_hash);
		const bool			  payload_target	 = _folder_payload_highlight_active;
		const vec4f_t		  selected_color	 = _focused ? theme.color_accent0 : theme.color_outline_light;
		const vec4f_t		  selected_color_dim = _focused ? theme.color_accent0_dim : theme.color_outline_light;
		vec4f_t				  payload_color		 = theme.color_accent1;
		payload_color.w							 = 50.0f / 255.0f;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = payload_target ? payload_color : selected ? selected_color : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.fill_color_b		 = payload_target ? payload_color : selected ? selected_color_dim : row_rect.fill_color_a;
		row_rect.gradient			 = ui::vg_gradient_e::horizontal;
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		_ui->get_paint().set_rect(row.root, row_rect);
		_ui->get_paint().set_hover_color(row.root, selected ? selected_color : theme.color_panel_light);
		_ui->get_paint().set_press_color(row.root, selected ? selected_color : theme.color_light);
	}

	void editor_panel_assets_t::set_folder_row_visible(const folder_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.in(row.root).flags = visible ? static_cast<u16>(ui::wf_visible | ui::wf_input | ui::wf_focusable) : 0;
		set_widget_visible(tree, row.icon, visible, /*input=*/false);
		set_widget_visible(tree, row.icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.star_text, visible && row.is_favourite, /*input=*/false);
		set_widget_visible(tree, row.label, visible, /*input=*/false);
	}

	void editor_panel_assets_t::set_folder_payload_highlight_active(bool active)
	{
		if (_folder_payload_highlight_active == active)
			return;

		_folder_payload_highlight_active = active;
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
			update_folder_row_background(_folder_rows[i]);
	}

}
