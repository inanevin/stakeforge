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
#include "ui/panels/editor_panel_entities.hpp"
#include "commands/editor_commands_entity.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
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
#define ENTITIES_INITIAL_ROW_CAPACITY 64
#define ENTITIES_INDENT_MULT		  2.0f

	namespace
	{
		enum entity_action_menu_command_e : u16
		{
			entity_action_menu_create_empty = 1,
			entity_action_menu_duplicate,
			entity_action_menu_delete,
		};

		struct editor_command_entity_selection_payload_t
		{
			chunk_handle32_t previous_entities = {};
			chunk_handle32_t next_entities	   = {};
			entity_id_t		 previous_anchor   = NULL_ENTITY_ID;
			entity_id_t		 next_anchor	   = NULL_ENTITY_ID;
			u32				 previous_count	   = 0;
			u32				 next_count		   = 0;
		};

		editor_action_menu_row_desc_t ENTITY_CREATE_ROWS[] = {
			{.text = "Empty Entity", .command = entity_action_menu_create_empty},
		};

		editor_action_menu_row_desc_t ENTITY_EMPTY_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
		};

		editor_action_menu_row_desc_t ENTITY_ROW_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
			{.text = "Duplicate Entity", .shortcut = "CTRL+D", .command = entity_action_menu_duplicate},
			{.text = "Delete Entity", .shortcut = "DEL", .command = entity_action_menu_delete},
		};

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

	editor_panel_entities_t::editor_panel_entities_t()
	{
		set_type(editor_panel_type_e::entities);
		set_title(editor_panel_type_to_string(editor_panel_type_e::entities));
	}

	void editor_panel_entities_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_entity_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_entity_top_row, "entity_top_row");
		tree.attach(_root, _entity_top_row);

		ui::layout_in_t& top_row_in = tree.in(_entity_top_row);
		top_row_in.flags			= ui::wf_visible;
		top_row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_row_in.size_mode_y		= ui::axis_mode_e::fixed;
		top_row_in.size_value		= {1.0f, theme.item_area_height};
		top_row_in.flow				= ui::flow_e::row;
		top_row_in.child_spacing	= theme.item_spacing;
		top_row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		u8*							data		  = reinterpret_cast<u8*>(&_search_str);
		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.field						  = {.type = editor_input_field_field_type_e::string, .fields = {.data = &data, .size = 1}};
		search_config.on_data_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _entity_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value		   = {1.0f, 0.5f};
		search_in.anchor_x		   = ui::anchor_e::end;
		search_in.anchor_y		   = ui::anchor_e::center;
		search_in.size_mode_x	   = ui::axis_mode_e::fixed;
		search_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_in.size_value	   = {theme.item_width, theme.item_height};

		_entity_list_area = ui.allocate_widget();
		ui.set_widget_debug_name(_entity_list_area, "entity_list_area");
		tree.attach(_root, _entity_list_area);

		ui::layout_in_t& list_in = tree.in(_entity_list_area);
		list_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_focusable | ui::wf_scroll_y;
		list_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		list_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		list_in.size_mode_y		 = ui::axis_mode_e::fill;
		list_in.size_value		 = {1.0f, 1.0f};
		list_in.flow			 = ui::flow_e::column;
		list_in.child_spacing	 = 0.0f;
		list_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, 0.0f};

		ui::vg_rect_paint_t list_rect = {};
		list_rect.fill_color_a		  = theme.color_frame;
		list_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_entity_list_area, list_rect);

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _entity_list_area;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_scrollbar.init(ui, scrollbar_config);

		ui::listener_bundle_t body_listener = {};
		body_listener.user_data				= this;
		body_listener.on_click				= on_entities_body_clicked;
		body_listener.on_wheel				= on_entities_body_wheel;
		body_listener.on_key				= on_entities_key;
		body_listener.on_focus_gain			= on_entities_focus_gain;
		body_listener.on_focus_lose			= on_entities_focus_lost;
		ui.get_input().set_listener(_entity_list_area, body_listener);

		ui.set_pre_layout_tick(_entity_list_area, on_entity_tree_tick, this);

		_entity_rows.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_entity_cache.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_expanded_entities.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_selected_entities.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_payload_entities.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_command_listener = editor_app_t::get().get_command_system().add_listener(on_command_system_event, this);
		editor_payload_controller_t::get().register_listener(on_payload_drop, nullptr, nullptr, this);
		refresh_entities();
	}

	void editor_panel_entities_t::uninit()
	{
		editor_app_t::get().get_command_system().remove_listener(_command_listener);
		editor_payload_controller_t::get().unregister_listener(this);
		_ui->cancel_mutations(this);
		_search_input.uninit();
		_scrollbar.uninit();
		_ui->deallocate_widget(_entity_top_row);
		_ui->deallocate_widget(_entity_list_area);

		_entity_rows.clear();
		_entity_cache.clear();
		_expanded_entities.clear();
		_selected_entities.clear();
		_payload_entities.clear();

		_entity_top_row		  = NULL_WIDGET;
		_entity_list_area	  = NULL_WIDGET;
		_command_listener	  = {};
		_main_world			  = {};
		_selection_anchor	  = NULL_ENTITY_ID;
		_action_menu_entity	  = NULL_ENTITY_ID;
		_entity_generation	  = 0;
		_visible_entity_count = 0;

		refresh_panel_inspector();
		editor_panel_t::uninit();
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

	void editor_panel_entities_t::select_entity_row(entity_id_t entity, bool range_select, bool incremental_select)
	{
		frame_vector_t<entity_id_t> selection;
		selection.reserve(_selected_entities.size() + _visible_entity_count);
		for (entity_id_t selected : _selected_entities)
			selection.push_back(selected);

		entity_id_t anchor = _selection_anchor;
		if (entity == NULL_ENTITY_ID)
		{
			selection.resize(0);
			anchor = NULL_ENTITY_ID;
		}
		else if (range_select && _selection_anchor != NULL_ENTITY_ID)
		{
			if (!incremental_select)
				selection.resize(0);

			const size_t anchor_index = find_visible_entity_index(_selection_anchor);
			const size_t entity_index = find_visible_entity_index(entity);
			if (anchor_index != SIZE_MAX && entity_index != SIZE_MAX)
			{
				const size_t first = std::min(anchor_index, entity_index);
				const size_t last  = std::max(anchor_index, entity_index);
				for (size_t i = first; i <= last; ++i)
				{
					if (std::find(selection.begin(), selection.end(), _entity_rows[i].entity) == selection.end())
						selection.push_back(_entity_rows[i].entity);
				}
			}
			else
			{
				selection.resize(0);
				selection.push_back(entity);
				anchor = entity;
			}
		}
		else if (incremental_select)
		{
			auto it = std::find(selection.begin(), selection.end(), entity);
			if (it != selection.end())
				selection.erase(it);
			else
				selection.push_back(entity);
			anchor = entity;
		}
		else
		{
			selection.resize(0);
			selection.push_back(entity);
			anchor = entity;
		}

		issue_entity_selection_command({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : anchor);
	}

	void editor_panel_entities_t::issue_entity_selection_command(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		bool same_selection = _selected_entities.size() == entities.size;
		for (size_t i = 0; same_selection && i < entities.size; ++i)
			same_selection = _selected_entities[i] == entities.data[i];
		if (same_selection && _selection_anchor == anchor)
			return;

		SFG_ASSERT(_selected_entities.size() <= UINT32_MAX);
		SFG_ASSERT(entities.size <= UINT32_MAX);

		editor_command_system_t& command_system = editor_app_t::get().get_command_system();

		editor_command_entity_selection_payload_t payload = {};
		payload.previous_anchor							  = _selection_anchor;
		payload.next_anchor								  = anchor;
		payload.previous_count							  = static_cast<u32>(_selected_entities.size());
		payload.next_count								  = static_cast<u32>(entities.size);

		if (payload.previous_count != 0)
		{
			entity_id_t* previous	  = nullptr;
			payload.previous_entities = command_system.get_aux_data().allocate<entity_id_t>(payload.previous_count, previous);
			SFG_MEMCPY(previous, _selected_entities.data(), sizeof(entity_id_t) * payload.previous_count);
		}

		if (payload.next_count != 0)
		{
			entity_id_t* next	  = nullptr;
			payload.next_entities = command_system.get_aux_data().allocate<entity_id_t>(payload.next_count, next);
			SFG_MEMCPY(next, entities.data, sizeof(entity_id_t) * payload.next_count);
		}

		const editor_command_issue_desc_t desc{
			.undo		= on_entity_selection_undo,
			.redo		= on_entity_selection_redo,
			.cleanup	= on_entity_selection_cleanup,
			.debug_name = "Select Entities",
			.type		= editor_command_type_e::entity_selection,
		};
		command_system.issue_command(desc, payload);
	}

	void editor_panel_entities_t::apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor)
	{
		_selected_entities.resize(0);
		_selected_entities.reserve(entities.size);
		for (size_t i = 0; i < entities.size; ++i)
			_selected_entities.push_back(entities.data[i]);
		_selection_anchor = anchor;
		for (const entity_row_t& row : _entity_rows)
			update_entity_row_background(row);
		refresh_panel_inspector();
	}

	void editor_panel_entities_t::clear_entity_selection()
	{
		issue_entity_selection_command({}, NULL_ENTITY_ID);
	}

	void editor_panel_entities_t::select_all_visible_entities()
	{
		frame_vector_t<entity_id_t> selection;
		selection.reserve(_visible_entity_count);
		for (u32 i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
			selection.push_back(_entity_rows[i].entity);
		issue_entity_selection_command({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : selection.back());
	}

	void editor_panel_entities_t::append_selected_root_entities(frame_vector_t<entity_id_t>& out_entities) const
	{
		out_entities.resize(0);
		out_entities.reserve(_selected_entities.size());
		for (entity_id_t entity : _selected_entities)
		{
			if (!has_selected_ancestor(entity))
				out_entities.push_back(entity);
		}
	}

	void editor_panel_entities_t::collect_payload_entities(entity_id_t entity)
	{
		_payload_entities.resize(0);

		const world_handle_t main_world = editor_app_t::get().get_main_world();
		if (main_world.is_null())
			return;

		world_t& world = editor_app_t::get().get_runtime().get_world(main_world);
		if (is_entity_selected(entity))
		{
			frame_vector_t<entity_id_t> root_entities;
			append_selected_root_entities(root_entities);
			for (entity_id_t selected_entity : root_entities)
			{
				if (selected_entity != NULL_ENTITY_ID && world.is_alive(selected_entity))
					_payload_entities.push_back({.world = main_world, .entity = selected_entity});
			}
		}

		if (_payload_entities.empty() && entity != NULL_ENTITY_ID && world.is_alive(entity))
			_payload_entities.push_back({.world = main_world, .entity = entity});
	}

	void editor_panel_entities_t::prune_entity_selection()
	{
		for (size_t i = 0; i < _selected_entities.size();)
		{
			if (find_entity_desc(_selected_entities[i]) == nullptr)
				_selected_entities.erase(_selected_entities.begin() + i);
			else
				++i;
		}
		if (_selection_anchor != NULL_ENTITY_ID && !is_entity_selected(_selection_anchor))
			_selection_anchor = _selected_entities.empty() ? NULL_ENTITY_ID : _selected_entities.back();
	}

	void editor_panel_entities_t::toggle_entity_fold(entity_id_t entity)
	{
		auto it = std::find(_expanded_entities.begin(), _expanded_entities.end(), entity);
		if (it == _expanded_entities.end())
			_expanded_entities.push_back(entity);
		else
			_expanded_entities.erase(it);
		refresh_entities();
	}

	void editor_panel_entities_t::create_entity(entity_id_t parent)
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());

		const entity_id_t entity = editor_commands_entity_t::create(main_world, parent);
		if (parent != NULL_ENTITY_ID && !is_entity_expanded(parent))
			_expanded_entities.push_back(parent);
		issue_entity_selection_command({.data = &entity, .size = 1}, entity);
		refresh_entities();
	}

	void editor_panel_entities_t::start_entity_payload(entity_id_t entity)
	{
		collect_payload_entities(entity);
		if (_payload_entities.empty())
			return;

		editor_payload_controller_t& payload_controller = editor_payload_controller_t::get();
		if (payload_controller.is_payload_active())
			return;

		if (_payload_entities.size() == 1)
		{
			_payload_entity		 = _payload_entities.front();
			const world_t& world = editor_app_t::get().get_runtime().get_world(_payload_entity.world);
			const char*	   name	 = world.get_entity_name(_payload_entity.entity);
			payload_controller.create_payload(name != nullptr ? name : "Entity", editor_payload_type_e::entity, &_payload_entity);
			return;
		}

		string_t text = std::to_string(_payload_entities.size());
		text += " entities";
		payload_controller.create_payload(text.c_str(), editor_payload_type_e::entity_multi, &_payload_entities);
	}

	bool editor_panel_entities_t::reparent_payload_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent)
	{
		if (!can_reparent_entities(entities, parent))
			return false;

		const world_handle_t		main_world = editor_app_t::get().get_main_world();
		frame_vector_t<entity_id_t> moved_entities;
		moved_entities.reserve(entities.size());
		for (const editor_entity_payload_t& payload_entity : entities)
			moved_entities.push_back(payload_entity.entity);

		if (!editor_commands_entity_t::reparent(main_world, moved_entities, parent))
			return false;

		if (parent != NULL_ENTITY_ID && !is_entity_expanded(parent))
			_expanded_entities.push_back(parent);
		refresh_entities();
		return true;
	}

	void editor_panel_entities_t::duplicate_selected_entities()
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());
		SFG_ASSERT(!_selected_entities.empty());

		frame_vector_t<entity_id_t> entities;
		append_selected_root_entities(entities);
		frame_vector_t<entity_id_t> duplicates;
		if (editor_commands_entity_t::duplicate(main_world, entities, duplicates))
		{
			const entity_id_t entity = duplicates.back();
			issue_entity_selection_command({.data = &entity, .size = 1}, entity);
		}
		refresh_entities();
	}

	void editor_panel_entities_t::destroy_selected_entities()
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());
		SFG_ASSERT(!_selected_entities.empty());

		frame_vector_t<entity_id_t> entities;
		append_selected_root_entities(entities);
		clear_entity_selection();
		editor_commands_entity_t::destroy(main_world, entities);
		refresh_entities();
	}

	void editor_panel_entities_t::open_empty_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_entity = NULL_ENTITY_ID;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ENTITY_EMPTY_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ENTITY_EMPTY_ACTION_MENU_ROWS) / sizeof(ENTITY_EMPTY_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_empty_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_panel_entities_t::open_entity_action_menu(const vec2f_t& pos, entity_id_t entity)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_entity						= entity;
		ENTITY_ROW_ACTION_MENU_ROWS[0].disabled = !is_create_enabled();

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ENTITY_ROW_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ENTITY_ROW_ACTION_MENU_ROWS) / sizeof(ENTITY_ROW_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_entity_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	bool editor_panel_entities_t::is_entity_expanded(entity_id_t entity) const
	{
		return std::find(_expanded_entities.begin(), _expanded_entities.end(), entity) != _expanded_entities.end();
	}

	bool editor_panel_entities_t::is_entity_selected(entity_id_t entity) const
	{
		return entity != NULL_ENTITY_ID && std::find(_selected_entities.begin(), _selected_entities.end(), entity) != _selected_entities.end();
	}

	bool editor_panel_entities_t::is_create_enabled() const
	{
		return _selected_entities.size() <= 1;
	}

	bool editor_panel_entities_t::has_selected_ancestor(entity_id_t entity) const
	{
		const entity_desc_t* desc = find_entity_desc(entity);
		while (desc != nullptr && desc->parent != NULL_ENTITY_ID)
		{
			if (is_entity_selected(desc->parent))
				return true;
			desc = find_entity_desc(desc->parent);
		}
		return false;
	}

	bool editor_panel_entities_t::can_reparent_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent) const
	{
		if (entities.empty())
			return false;

		const world_handle_t main_world = editor_app_t::get().get_main_world();
		if (main_world.is_null())
			return false;

		world_t& world = editor_app_t::get().get_runtime().get_world(main_world);
		if (parent != NULL_ENTITY_ID && !world.is_alive(parent))
			return false;

		for (const editor_entity_payload_t& payload_entity : entities)
		{
			if (!(payload_entity.world == main_world) || payload_entity.entity == NULL_ENTITY_ID || !world.is_alive(payload_entity.entity))
				return false;
			if (payload_entity.entity == parent)
				return false;

			for (entity_id_t cur = parent; cur != NULL_ENTITY_ID; cur = world.get_entity_parent(cur))
			{
				if (cur == payload_entity.entity)
					return false;
			}
		}
		return true;
	}

	size_t editor_panel_entities_t::find_visible_entity_index(entity_id_t entity) const
	{
		for (size_t i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
		{
			if (_entity_rows[i].entity == entity)
				return i;
		}
		return SIZE_MAX;
	}

	const editor_panel_entities_t::entity_desc_t* editor_panel_entities_t::find_entity_desc(entity_id_t entity) const
	{
		for (const entity_desc_t& desc : _entity_cache)
		{
			if (desc.id == entity)
				return &desc;
		}
		return nullptr;
	}

	const editor_panel_entities_t::entity_row_t* editor_panel_entities_t::find_row_by_widget(ui::widget_id_t id, bool match_icon) const
	{
		for (u32 i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
		{
			const entity_row_t& row = _entity_rows[i];
			if (row.root == id || row.label == id || (match_icon && (row.icon == id || row.icon_text == id)))
				return &row;
		}
		return nullptr;
	}

	const editor_panel_entities_t::entity_row_t* editor_panel_entities_t::find_row_by_pos(const vec2f_t& pos) const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		for (u32 i = 0; i < _visible_entity_count && i < _entity_rows.size(); ++i)
		{
			const entity_row_t&		row = _entity_rows[i];
			const ui::layout_out_t& out = tree.out(row.root);
			if (rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(pos))
				return &row;
		}
		return nullptr;
	}

	void editor_panel_entities_t::on_search_changed(void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel._search_str_lower		   = panel._search_str;
		string_util::to_lower(panel._search_str_lower);
		panel.refresh_entities();
	}

	void editor_panel_entities_t::on_empty_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (command == entity_action_menu_create_empty)
			panel.create_entity(NULL_ENTITY_ID);
	}

	void editor_panel_entities_t::on_entity_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (command == entity_action_menu_create_empty)
		{
			if (panel.is_create_enabled())
				panel.create_entity(panel._action_menu_entity);
		}
		else if (command == entity_action_menu_duplicate)
			panel.duplicate_selected_entities();
		else if (command == entity_action_menu_delete)
			panel.destroy_selected_entities();
	}

	void editor_panel_entities_t::on_entities_body_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (btn == ui::mouse_button_e::right && id == panel._entity_list_area)
			panel.open_empty_action_menu(pos);
	}

	void editor_panel_entities_t::on_entities_body_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel._scrollbar.scroll_y(delta);
	}

	void editor_panel_entities_t::on_entities_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press)
			return;

		editor_panel_entities_t& panel		  = *static_cast<editor_panel_entities_t*>(user_data);
		const bool				 ctrl_pressed = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
		if (ev.key == static_cast<u16>(input_code::key_a) && ctrl_pressed)
		{
			panel.select_all_visible_entities();
			return;
		}

		if (panel._selected_entities.empty())
			return;

		if (ev.key == static_cast<u16>(input_code::key_delete))
			panel.destroy_selected_entities();
		else if (ev.key == static_cast<u16>(input_code::key_d) && ctrl_pressed)
			panel.duplicate_selected_entities();
	}

	void editor_panel_entities_t::on_entities_focus_gain(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->set_focus_state(true);
	}

	void editor_panel_entities_t::on_entities_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_entities_t::on_entity_row_focus_gain(ui::input_router_t&, ui::widget_id_t id, bool from_nav, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel.set_focus_state(true);
		if (from_nav)
		{
			const entity_row_t* const row = panel.find_row_by_widget(id, /*match_icon=*/false);
			if (row != nullptr)
				panel.select_entity_row(row->entity, false, false);
		}
	}

	void editor_panel_entities_t::on_entity_row_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_entities_t::on_entity_tree_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (!(editor_app_t::get().get_main_world() == panel._main_world) || panel._entity_generation != editor_app_t::get().get_command_system().get_entity_generation())
			panel.refresh_entities();
	}

	void editor_panel_entities_t::on_entity_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/true);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_entity_selected(row->entity))
				panel.select_entity_row(row->entity, false, false);
			panel.open_entity_action_menu(pos, row->entity);
		}
		else if (row->has_children)
			panel.toggle_entity_fold(row->entity);
	}

	void editor_panel_entities_t::on_entity_row_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_entity_selected(row->entity))
				panel.select_entity_row(row->entity, false, false);
			panel.open_entity_action_menu(pos, row->entity);
		}
		else
		{
			const bool shift_pressed = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
			const bool ctrl_pressed	 = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			panel.select_entity_row(row->entity, shift_pressed, ctrl_pressed);
		}
	}

	void editor_panel_entities_t::on_entity_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr)
			return;

		panel.start_entity_payload(row->entity);
	}

	void editor_panel_entities_t::on_entity_row_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr || !row->has_children)
			return;
		panel.toggle_entity_fold(row->entity);
	}

	bool editor_panel_entities_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::entity && payload.type != editor_payload_type_e::entity_multi)
			return false;
		SFG_ASSERT(payload.user_ptr != nullptr);

		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		const ui::layout_out_t&	 out   = panel._ui->get_tree().out(panel._entity_list_area);
		const vec2f_t&			 mouse = panel._ui->get_input().get_mouse_position();
		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(mouse))
			return false;

		const entity_row_t* const row	  = panel.find_row_by_pos(mouse);
		const entity_id_t		  parent  = row != nullptr ? row->entity : NULL_ENTITY_ID;
		bool					  changed = false;
		if (payload.type == editor_payload_type_e::entity)
		{
			const editor_entity_payload_t& entity = *static_cast<const editor_entity_payload_t*>(payload.user_ptr);
			panel._payload_entities.resize(0);
			panel._payload_entities.push_back(entity);
			changed = panel.reparent_payload_entities(panel._payload_entities, parent);
		}
		else
		{
			const vector_t<editor_entity_payload_t>& entities = *static_cast<const vector_t<editor_entity_payload_t>*>(payload.user_ptr);
			changed											  = panel.reparent_payload_entities(entities, parent);
		}
		return changed;
	}

	bool editor_panel_entities_t::on_entity_selection_undo(editor_command_system_t& system, editor_command_t& command)
	{
		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel == nullptr)
			return true;

		const editor_command_entity_selection_payload_t& payload  = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
		const entity_id_t*								 entities = payload.previous_count != 0 ? system.get_aux_data().get<entity_id_t>(payload.previous_entities) : nullptr;
		static_cast<editor_panel_entities_t*>(panel)->apply_entity_selection({.data = entities, .size = payload.previous_count}, payload.previous_anchor);
		return true;
	}

	bool editor_panel_entities_t::on_entity_selection_redo(editor_command_system_t& system, editor_command_t& command)
	{
		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel == nullptr)
			return true;

		const editor_command_entity_selection_payload_t& payload  = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
		const entity_id_t*								 entities = payload.next_count != 0 ? system.get_aux_data().get<entity_id_t>(payload.next_entities) : nullptr;
		static_cast<editor_panel_entities_t*>(panel)->apply_entity_selection({.data = entities, .size = payload.next_count}, payload.next_anchor);
		return true;
	}

	bool editor_panel_entities_t::on_entity_selection_cleanup(editor_command_system_t& system, editor_command_t& command)
	{
		editor_command_entity_selection_payload_t& payload = system.get_payload_as<editor_command_entity_selection_payload_t>(command);
		if (payload.previous_entities)
		{
			system.get_aux_data().free(payload.previous_entities);
			payload.previous_entities = {};
		}
		if (payload.next_entities)
		{
			system.get_aux_data().free(payload.next_entities);
			payload.next_entities = {};
		}
		return true;
	}

	void editor_panel_entities_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		if (command.type != editor_command_type_e::entity_duplicate || command.state != editor_command_state_e::done)
			return;

		editor_panel_entities_t&						 panel	  = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_command_duplicate_entity_payload_t& payload  = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
		const entity_id_t*								 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
		const entity_id_t								 entity	  = entities[payload.count - 1];
		panel.issue_entity_selection_command({.data = &entity, .size = 1}, entity);
	}

	void editor_panel_entities_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->flush_pending_ui_mutations();
	}
}
