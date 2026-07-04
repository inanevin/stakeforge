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
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "commands/editor_commands_entity.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

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
	void editor_panel_entities_t::refresh_entities()
	{
		if (!can_mutate_ui_topology())
		{
			request_refresh_entities();
			return;
		}

		_entity_generation = editor_app_t::get().get_command_system().get_entity_generation();
		collect_entities();
		prune_entity_selection();
		_visible_entity_count = 0;

		const bool search_active = !_search_str_lower.empty();
		u16		   hidden_depth	 = UINT16_MAX;

		editor_world_metadata_t&			 metadata = editor_app_t::get().get_world_metadata();
		const span_t<editor_outliner_item_t> items	  = metadata.get_outliner_items();
		frame_vector_t<u8>					 search_visible;
		frame_vector_t<size_t>				 search_ancestor_stack;
		if (search_active)
		{
			search_visible.reserve(items.size);
			search_ancestor_stack.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
			for (size_t i = 0; i < items.size; ++i)
				search_visible.push_back(0);

			for (size_t i = 0; i < items.size; ++i)
			{
				const editor_outliner_item_t& item = items.data[i];
				while (search_ancestor_stack.size() > item.depth)
					search_ancestor_stack.pop_back();

				string_t name_lower;
				name_lower.assign(item.name, std::strlen(item.name));
				string_util::to_lower(name_lower);
				if (name_lower.find(_search_str_lower.c_str()) != string_t::npos)
				{
					search_visible[i] = 1;
					for (size_t ancestor_index : search_ancestor_stack)
						search_visible[ancestor_index] = 1;
				}

				search_ancestor_stack.push_back(i);
			}
		}

		for (size_t i = 0; i < items.size; ++i)
		{
			const editor_outliner_item_t& item = items.data[i];
			if (search_active && !search_visible[i])
				continue;

			if (hidden_depth != UINT16_MAX)
			{
				if (item.depth > hidden_depth)
					continue;
				hidden_depth = UINT16_MAX;
			}

			const bool			   is_expanded = item.type == editor_outliner_item_type_e::folder ? !metadata.get_folder(item.folder_handle).folded : metadata.is_entity_expanded(item.entity_guid);
			const bool			   is_folded   = item.has_children && !search_active && !is_expanded;
			editor_outliner_row_t& row		   = get_or_create_outliner_row(_visible_entity_count++);
			update_outliner_row(row, item, is_folded);
			if (is_folded)
				hidden_depth = item.depth;
		}

		vector_t<editor_outliner_row_t>& rows = metadata.get_outliner_rows();
		for (size_t i = _visible_entity_count; i < rows.size(); ++i)
			set_outliner_row_visible(rows[i], false);
	}

	void editor_panel_entities_t::refresh_entity_name(entity_id_t entity)
	{
		if (!_search_str_lower.empty())
		{
			refresh_entities();
			return;
		}

		world_t&	world = editor_app_t::get().get_world_controller().get_world(_main_world);
		const char* name  = world.get_entity_name(entity);
		const char* text  = name != nullptr ? name : "Entity";

		editor_world_metadata_t&	   metadata = editor_app_t::get().get_world_metadata();
		editor_outliner_item_t*		   desc		= nullptr;
		span_t<editor_outliner_item_t> items	= metadata.get_outliner_items();
		for (size_t i = 0; i < items.size; ++i)
		{
			editor_outliner_item_t& cached_entity = items.data[i];
			if (cached_entity.entity == entity)
			{
				cached_entity.name = text;
				desc			   = &cached_entity;
				break;
			}
		}

		if (desc == nullptr)
			return;

		const size_t row_index = find_visible_entity_index(entity);
		if (row_index == SIZE_MAX)
			return;

		const bool is_folded = desc->has_children && !is_entity_expanded(entity);
		update_outliner_row(metadata.get_outliner_rows()[row_index], *desc, is_folded);
	}

	void editor_panel_entities_t::collect_entities()
	{
		editor_app_t&		 app		= editor_app_t::get();
		const world_handle_t main_world = app.get_world_controller().get_main_world();
		_main_world						= main_world;
		app.get_selection_controller().set_world(main_world);
		if (main_world.is_null())
			return;

		const world_t& world = app.get_world_controller().get_world(main_world);
		app.get_world_metadata().collect_outliner_items(world);
	}

	editor_outliner_row_t& editor_panel_entities_t::get_or_create_outliner_row(size_t index)
	{
		vector_t<editor_outliner_row_t>& rows = editor_app_t::get().get_world_metadata().get_outliner_rows();
		if (index < rows.size())
			return rows[index];

		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		editor_outliner_row_t row = {};

		row.root = ui.allocate_widget();
		ui.set_widget_debug_name(row.root, "outliner_row");
		tree.attach(_entity_list_area, row.root);
		tree.draw_order(row.root) = tree.draw_order_const(_entity_list_area) + 1;

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
		listener.on_click			   = on_entity_row_clicked;
		listener.on_double_click	   = on_entity_row_double_clicked;
		listener.on_drag_begin		   = on_entity_row_drag_begin;
		listener.on_focus_gain		   = on_entity_row_focus_gain;
		listener.on_focus_lose		   = on_entity_row_focus_lost;
		ui.get_input().set_listener(row.root, listener);

		row.fold_icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.fold_icon, "outliner_row_fold_icon_wrapper");
		tree.attach(row.root, row.fold_icon);

		ui::layout_in_t& icon_in = tree.in(row.fold_icon);
		icon_in.flags			 = ui::wf_visible;
		icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value.y		 = 0.5f;
		icon_in.anchor_y		 = ui::anchor_e::center;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;
		icon_in.size_value		 = {theme.item_height, theme.item_height};

		ui::listener_bundle_t icon_listener = {};
		icon_listener.user_data				= this;
		icon_listener.on_click				= on_entity_icon_clicked;
		ui.get_input().set_listener(row.fold_icon, icon_listener);

		row.fold_icon_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.fold_icon_text, "outliner_row_fold_icon");
		tree.attach(row.fold_icon, row.fold_icon_text);

		ui::layout_in_t& icon_text_in = tree.in(row.fold_icon_text);
		icon_text_in.flags			  = ui::wf_visible;
		icon_text_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_value		  = {0.5f, 0.5f};
		icon_text_in.anchor_x		  = ui::anchor_e::center;
		icon_text_in.anchor_y		  = ui::anchor_e::center;
		icon_text_in.size_mode_x	  = ui::axis_mode_e::fixed;
		icon_text_in.size_mode_y	  = ui::axis_mode_e::fixed;

		row.type_icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.type_icon, "outliner_row_type_icon_wrapper");
		tree.attach(row.root, row.type_icon);

		ui::layout_in_t& type_icon_in = tree.in(row.type_icon);
		type_icon_in.flags			  = ui::wf_visible;
		type_icon_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		type_icon_in.pos_value.y	  = 0.5f;
		type_icon_in.anchor_y		  = ui::anchor_e::center;
		type_icon_in.size_mode_x	  = ui::axis_mode_e::fixed;
		type_icon_in.size_mode_y	  = ui::axis_mode_e::fixed;
		type_icon_in.size_value		  = {theme.item_height, theme.item_height};

		row.type_icon_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.type_icon_text, "outliner_row_type_icon");
		tree.attach(row.type_icon, row.type_icon_text);

		ui::layout_in_t& type_icon_text_in = tree.in(row.type_icon_text);
		type_icon_text_in.flags			   = ui::wf_visible;
		type_icon_text_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		type_icon_text_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		type_icon_text_in.pos_value		   = {0.5f, 0.5f};
		type_icon_text_in.anchor_x		   = ui::anchor_e::center;
		type_icon_text_in.anchor_y		   = ui::anchor_e::center;
		type_icon_text_in.size_mode_x	   = ui::axis_mode_e::fixed;
		type_icon_text_in.size_mode_y	   = ui::axis_mode_e::fixed;

		row.label = ui.allocate_widget();
		ui.set_widget_debug_name(row.label, "outliner_row_label");
		tree.attach(row.root, row.label);

		ui::layout_in_t& label_in = tree.in(row.label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fill;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;

		row.disable_button = ui.allocate_widget();
		ui.set_widget_debug_name(row.disable_button, "outliner_row_disable_button");
		tree.attach(row.root, row.disable_button);
		tree.draw_order(row.disable_button) = tree.draw_order_const(row.root) + 1;

		ui::layout_in_t& disable_button_in = tree.in(row.disable_button);
		disable_button_in.flags			   = ui::wf_visible | ui::wf_input;
		disable_button_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		disable_button_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		disable_button_in.pos_value		   = {1.0f, 0.5f};
		disable_button_in.anchor_x		   = ui::anchor_e::end;
		disable_button_in.anchor_y		   = ui::anchor_e::center;
		disable_button_in.size_mode_x	   = ui::axis_mode_e::fixed;
		disable_button_in.size_mode_y	   = ui::axis_mode_e::fixed;
		disable_button_in.size_value	   = {theme.item_height, theme.item_height};

		ui::listener_bundle_t disable_listener = {};
		disable_listener.user_data			   = this;
		disable_listener.on_click			   = on_entity_disable_clicked;
		ui.get_input().set_listener(row.disable_button, disable_listener);

		editor_tooltip_desc_t tooltip = {};
		tooltip.text				  = "Toggle Disable";
		editor_tooltip_controller_t::find(ui)->set_tooltip(row.disable_button, tooltip);

		row.disable_icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.disable_icon, "outliner_row_disable_icon");
		tree.attach(row.disable_button, row.disable_icon);
		tree.draw_order(row.disable_icon) = tree.draw_order_const(row.disable_button) + 1;

		ui::layout_in_t& disable_icon_in = tree.in(row.disable_icon);
		disable_icon_in.flags			 = ui::wf_visible;
		disable_icon_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		disable_icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		disable_icon_in.pos_value		 = {0.5f, 0.5f};
		disable_icon_in.anchor_x		 = ui::anchor_e::center;
		disable_icon_in.anchor_y		 = ui::anchor_e::center;
		disable_icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		disable_icon_in.size_mode_y		 = ui::axis_mode_e::fixed;

		rows.push_back(row);
		return rows.back();
	}

	void editor_panel_entities_t::update_outliner_row(editor_outliner_row_t& row, const editor_outliner_item_t& item, bool is_folded)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		row.entity		  = item.entity;
		row.folder_handle = item.folder_handle;
		row.depth		  = item.depth;
		row.type		  = item.type;
		row.has_children  = item.has_children;
		row.disabled	  = item.disabled;

		set_outliner_row_visible(row, true);
		update_outliner_row_background(row);

		ui::layout_in_t& row_in			= tree.in(row.root);
		row_in.child_margins			= {0.0f, theme.item_height, 0.0f, theme.margin_horizontal + static_cast<f32>(item.depth) * theme.indent_horizontal * ENTITIES_INDENT_MULT};
		tree.in(row.fold_icon).flags	= item.has_children ? static_cast<u16>(ui::wf_visible | ui::wf_input) : static_cast<u16>(ui::wf_visible);
		const vec4f_t entity_text_color = item.disabled ? theme.color_text_disabled : theme.color_text0;
		const vec4f_t entity_icon_color = item.disabled ? theme.color_text_disabled : theme.color_text1;

		const char* icon = item.has_children ? (is_folded ? ICON_DD_RIGHT : ICON_DD_DOWN) : "";
		_ui->set_widget_text(row.fold_icon_text, icon);
		paint.set_text(row.fold_icon_text,
					   _ui->widget_text(row.fold_icon_text),
					   _ui->widget_text_len(row.fold_icon_text),
					   {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.type_icon_text, item.type_icon != nullptr ? item.type_icon : "");
		paint.set_text(row.type_icon_text,
					   _ui->widget_text(row.type_icon_text),
					   _ui->widget_text_len(row.type_icon_text),
					   {.font		 = theme.font_icons,
						.color		 = item.type == editor_outliner_item_type_e::folder ? item.color.to_vector() : entity_icon_color,
						.point_size	 = theme.icon_default_px_size,
						.spacing	 = 0,
						.raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.label, item.name);
		paint.set_text(row.label,
					   _ui->widget_text(row.label),
					   _ui->widget_text_len(row.label),
					   {.font		 = item.type == editor_outliner_item_type_e::folder ? theme.font_default : theme.font_default,
						.color		 = item.type == editor_outliner_item_type_e::folder ? item.color.to_vector() : entity_text_color,
						.point_size	 = theme.text_default_px_size,
						.spacing	 = 0,
						.raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		const bool disable_button_visible = item.type == editor_outliner_item_type_e::entity;
		tree.in(row.disable_button).flags = disable_button_visible ? static_cast<u16>(ui::wf_visible | ui::wf_input) : 0;
		tree.in(row.disable_icon).flags	  = disable_button_visible ? static_cast<u16>(ui::wf_visible) : 0;

		ui::vg_rect_paint_t disable_rect = {};
		disable_rect.fill_color_a		 = item.disabled ? theme.color_frame_light : vec4f_t::zero;
		disable_rect.fill_color_b		 = disable_rect.fill_color_a;
		disable_rect.rounding			 = theme.item_rounding;
		disable_rect.outline_color		 = item.disabled ? theme.color_outline_light : vec4f_t::zero;
		disable_rect.outline_thickness	 = theme.outline_thickness;
		paint.set_rect(row.disable_button, disable_rect);
		paint.set_hover_color(row.disable_button, theme.color_panel_light);
		paint.set_press_color(row.disable_button, theme.color_frame_light);

		_ui->set_widget_text(row.disable_icon, item.disabled ? ICON_EYE_CROSS : ICON_EYE);
		paint.set_text(row.disable_icon,
					   _ui->widget_text(row.disable_icon),
					   _ui->widget_text_len(row.disable_icon),
					   {.font = theme.font_icons, .color = item.disabled ? theme.color_text_disabled : theme.color_text1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		paint.set_state_source(row.disable_icon, row.disable_button);
	}

	void editor_panel_entities_t::update_outliner_row_background(const editor_outliner_row_t& row)
	{
		const editor_theme_t& theme				 = editor_theme_t::get();
		const bool			  selected			 = row.type == editor_outliner_item_type_e::entity && is_entity_selected(row.entity);
		const vec4f_t		  selected_color	 = _focused ? theme.color_accent0 : theme.color_outline_light;
		const vec4f_t		  selected_color_dim = _focused ? theme.color_accent0_dim : theme.color_outline_light;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = selected ? selected_color : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.fill_color_b		 = selected ? selected_color_dim : row_rect.fill_color_a;
		row_rect.gradient			 = ui::vg_gradient_e::horizontal;
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		_ui->get_paint().set_rect(row.root, row_rect);
		_ui->get_paint().set_hover_color(row.root, selected ? selected_color : theme.color_panel_light);
		_ui->get_paint().set_press_color(row.root, selected ? selected_color : theme.color_light);
	}

	void editor_panel_entities_t::set_outliner_row_visible(const editor_outliner_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.in(row.root).flags = visible ? static_cast<u16>(ui::wf_visible | ui::wf_input | ui::wf_focusable) : 0;
		set_widget_visible(tree, row.fold_icon, visible, /*input=*/false);
		set_widget_visible(tree, row.fold_icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.type_icon, visible, /*input=*/false);
		set_widget_visible(tree, row.type_icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.label, visible, /*input=*/false);
		set_widget_visible(tree, row.disable_button, visible && row.type == editor_outliner_item_type_e::entity, /*input=*/visible && row.type == editor_outliner_item_type_e::entity);
		set_widget_visible(tree, row.disable_icon, visible && row.type == editor_outliner_item_type_e::entity, /*input=*/false);
	}

	void editor_panel_entities_t::set_focus_state(bool focused)
	{
		if (_focused == focused)
			return;

		_focused = focused;
		for (const editor_outliner_row_t& row : editor_app_t::get().get_world_metadata().get_outliner_rows())
			update_outliner_row_background(row);
	}

	bool editor_panel_entities_t::can_mutate_ui_topology() const
	{
		if (_ui == nullptr)
			return false;

		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_panel_entities_t::request_refresh_entities()
	{
		_refresh_entities_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_entities_t::flush_pending_ui_mutations()
	{
		if (!_refresh_entities_pending)
			return;

		_refresh_entities_pending = false;
		refresh_entities();
	}

}
