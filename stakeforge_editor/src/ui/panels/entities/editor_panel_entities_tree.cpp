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
#include "ui/panels/inspector/editor_panel_inspector.hpp"
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

		for (const entity_desc_t& entity : _entity_cache)
		{
			if (hidden_depth != UINT16_MAX)
			{
				if (entity.depth > hidden_depth)
					continue;
				hidden_depth = UINT16_MAX;
			}

			bool self_matches_search = !search_active;
			if (search_active)
			{
				string_t name_lower;
				name_lower.assign(entity.name, std::strlen(entity.name));
				string_util::to_lower(name_lower);
				self_matches_search = name_lower.find(_search_str_lower.c_str()) != string_t::npos;
			}

			const bool is_expanded = is_entity_expanded(entity.id);
			const bool is_folded   = entity.has_children && !search_active && !is_expanded;
			if (self_matches_search)
			{
				entity_row_t& row = get_or_create_entity_row(_visible_entity_count++);
				update_entity_row(row, entity, is_folded);
			}
			if (is_folded)
				hidden_depth = entity.depth;
		}

		for (size_t i = _visible_entity_count; i < _entity_rows.size(); ++i)
			set_entity_row_visible(_entity_rows[i], false);

		refresh_panel_inspector();
	}

	void editor_panel_entities_t::refresh_entity_name(entity_id_t entity)
	{
		if (!_search_str_lower.empty())
		{
			refresh_entities();
			return;
		}

		world_t&	world = editor_app_t::get().get_runtime().get_world(_main_world);
		const char* name  = world.get_entity_name(entity);
		const char* text  = name != nullptr ? name : "Entity";

		entity_desc_t* desc = nullptr;
		for (entity_desc_t& cached_entity : _entity_cache)
		{
			if (cached_entity.id == entity)
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
		update_entity_row(_entity_rows[row_index], *desc, is_folded);
	}

	void editor_panel_entities_t::refresh_panel_inspector()
	{
		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::inspector);
		if (panel == nullptr)
			return;

		editor_panel_inspector_t* inspector = static_cast<editor_panel_inspector_t*>(panel);
		if (_main_world.is_null() || _selected_entities.empty())
		{
			inspector->set_display_none();
			return;
		}

		if (_selected_entities.size() == 1)
			inspector->set_display_entity(_main_world, _selected_entities.front());
		else
			inspector->set_display_entity(_main_world, {.data = _selected_entities.data(), .size = _selected_entities.size()});
	}

	void editor_panel_entities_t::collect_entities()
	{
		_entity_cache.resize(0);

		editor_app_t&		 app		= editor_app_t::get();
		const world_handle_t main_world = app.get_main_world();
		_main_world						= main_world;
		if (main_world.is_null())
			return;

		const world_t&				   world		   = app.get_runtime().get_world(main_world);
		const world_component_table_t* alive_table	   = world.find_component_table(type_id_t<component_alive_t>::value);
		const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
		const world_component_table_t* name_table	   = world.find_component_table(type_id_t<component_name_t>::value);
		SFG_ASSERT(alive_table != nullptr);
		SFG_ASSERT(hierarchy_table != nullptr);
		SFG_ASSERT(name_table != nullptr);

		const ecs_component_table_ref_t table_refs[] = {
			alive_table->table.ref(),
			hierarchy_table->table.ref(),
			name_table->table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_hierarchy_t& hierarchy = ecs_helpers_t::row_get<component_hierarchy_t>(row, 1);
			if (hierarchy.parent == NULL_ENTITY_ID)
				append_entity_desc(world, hierarchy_table->table, name_table->table, row.id, 0);
		}
	}

	void editor_panel_entities_t::append_entity_desc(const world_t& world, const ecs_component_table_t& hierarchy_table, const ecs_component_table_t& name_table, entity_id_t id, u16 depth)
	{
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, id);
		const component_name_t&		 name	   = ecs_helpers_t::table_get_as_const<component_name_t>(name_table, id);
		const entity_id_t			 parent	   = hierarchy.parent;

		_entity_cache.push_back({.name = name.text, .id = id, .parent = parent, .depth = depth, .has_children = hierarchy.first_child != NULL_ENTITY_ID});

		entity_id_t child = hierarchy.first_child;
		while (child != NULL_ENTITY_ID)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
			append_entity_desc(world, hierarchy_table, name_table, child, static_cast<u16>(depth + 1));
			child = child_hierarchy.next_sibling;
		}
	}

	editor_panel_entities_t::entity_row_t& editor_panel_entities_t::get_or_create_entity_row(size_t index)
	{
		if (index < _entity_rows.size())
			return _entity_rows[index];

		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		entity_row_t row = {};

		row.root = ui.allocate_widget();
		ui.set_widget_debug_name(row.root, "entity_row");
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

		row.icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon, "entity_row_icon_wrapper");
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
		icon_listener.on_click				= on_entity_icon_clicked;
		ui.get_input().set_listener(row.icon, icon_listener);

		row.icon_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon_text, "entity_row_icon");
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

		row.label = ui.allocate_widget();
		ui.set_widget_debug_name(row.label, "entity_row_label");
		tree.attach(row.root, row.label);

		ui::layout_in_t& label_in = tree.in(row.label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;

		_entity_rows.push_back(row);
		return _entity_rows.back();
	}

	void editor_panel_entities_t::update_entity_row(entity_row_t& row, const entity_desc_t& entity, bool is_folded)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		row.entity		 = entity.id;
		row.depth		 = entity.depth;
		row.has_children = entity.has_children;

		set_entity_row_visible(row, true);
		update_entity_row_background(row);

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.child_margins	= {0.0f, theme.item_height, 0.0f, theme.margin_horizontal + static_cast<f32>(entity.depth) * theme.indent_horizontal * ENTITIES_INDENT_MULT};
		tree.in(row.icon).flags = entity.has_children ? static_cast<u16>(ui::wf_visible | ui::wf_input) : static_cast<u16>(ui::wf_visible);

		const char* icon = entity.has_children ? (is_folded ? ICON_DD_RIGHT : ICON_DD_DOWN) : "";
		_ui->set_widget_text(row.icon_text, icon);
		paint.set_text(row.icon_text,
					   _ui->widget_text(row.icon_text),
					   _ui->widget_text_len(row.icon_text),
					   {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.label, entity.name);
		paint.set_text(row.label,
					   _ui->widget_text(row.label),
					   _ui->widget_text_len(row.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_entities_t::update_entity_row_background(const entity_row_t& row)
	{
		const editor_theme_t& theme				 = editor_theme_t::get();
		const bool			  selected			 = is_entity_selected(row.entity);
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

	void editor_panel_entities_t::set_entity_row_visible(const entity_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.in(row.root).flags = visible ? static_cast<u16>(ui::wf_visible | ui::wf_input | ui::wf_focusable) : 0;
		set_widget_visible(tree, row.icon, visible, /*input=*/false);
		set_widget_visible(tree, row.icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.label, visible, /*input=*/false);
	}

	void editor_panel_entities_t::set_focus_state(bool focused)
	{
		if (_focused == focused)
			return;

		_focused = focused;
		for (const entity_row_t& row : _entity_rows)
			update_entity_row_background(row);
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
