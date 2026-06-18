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
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

#include <algorithm>
#include <cstring>
#include <iterator>

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

		editor_action_menu_row_desc_t ENTITY_CREATE_ROWS[] = {
			{.text = "Empty Entity", .command = entity_action_menu_create_empty},
		};

		editor_action_menu_row_desc_t ENTITY_EMPTY_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
		};

		editor_action_menu_row_desc_t ENTITY_ROW_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ENTITY_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ENTITY_CREATE_ROWS) / sizeof(ENTITY_CREATE_ROWS[0]))},
			{.text = "Duplicate Entity", .command = entity_action_menu_duplicate},
			{.text = "Delete Entity", .command = entity_action_menu_delete},
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

		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.text_value				  = _search_str.c_str();
		search_config.type						  = editor_input_field_type_e::text;
		search_config.on_text_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _entity_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.flags |= ui::wf_visible | ui::wf_overlay;
		search_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value	  = {1.0f, 0.5f};
		search_in.anchor_x	  = ui::anchor_e::end;
		search_in.anchor_y	  = ui::anchor_e::center;
		search_in.size_mode_x = ui::axis_mode_e::fixed;
		search_in.size_mode_y = ui::axis_mode_e::fixed;
		search_in.size_value  = {theme.item_width, theme.item_height};

		_entity_list_area = ui.allocate_widget();
		ui.set_widget_debug_name(_entity_list_area, "entity_list_area");
		tree.attach(_root, _entity_list_area);

		ui::layout_in_t& list_in = tree.in(_entity_list_area);
		list_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
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
		ui.get_input().set_listener(_entity_list_area, body_listener);

		ui.set_pre_layout_tick(_entity_list_area, on_entity_tree_tick, this);

		_entity_rows.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_entity_cache.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_expanded_entities.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		refresh_entities();
	}

	void editor_panel_entities_t::uninit()
	{
		_search_input.uninit();
		_scrollbar.uninit();
		_ui->deallocate_widget(_entity_top_row);
		_ui->deallocate_widget(_entity_list_area);

		_entity_rows.clear();
		_entity_cache.clear();
		_expanded_entities.clear();

		_entity_top_row		  = NULL_WIDGET;
		_entity_list_area	  = NULL_WIDGET;
		_main_world			  = {};
		_selected_entity	  = NULL_ENTITY_ID;
		_action_menu_entity	  = NULL_ENTITY_ID;
		_command_generation	  = 0;
		_visible_entity_count = 0;

		editor_panel_t::uninit();
	}

	void editor_panel_entities_t::refresh_entities()
	{
		_command_generation = editor_app_t::get().get_command_system().get_generation();
		collect_entities();
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
		const world_component_table_t* alive_table	   = world.find_component_table(component_alive_t::TYPE_ID);
		const world_component_table_t* hierarchy_table = world.find_component_table(component_hierarchy_t::TYPE_ID);
		const world_component_table_t* name_table	   = world.find_component_table(component_name_t::TYPE_ID);
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
		const char*					 text	   = world.get_text(name.text_index);

		_entity_cache.push_back({.name = text == nullptr ? "Entity" : text, .id = id, .parent = hierarchy.parent, .depth = depth, .has_children = hierarchy.first_child != NULL_ENTITY_ID});

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
		row_in.flags			= ui::wf_visible | ui::wf_input;
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
		const editor_theme_t& theme	   = editor_theme_t::get();
		const bool			  selected = row.entity != NULL_ENTITY_ID && row.entity == _selected_entity;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = selected ? theme.color_accent0 : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.fill_color_b		 = selected ? theme.color_accent0_dim : row_rect.fill_color_a;
		row_rect.gradient			 = ui::vg_gradient_e::vertical;
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		_ui->get_paint().set_rect(row.root, row_rect);
		_ui->get_paint().set_hover_color(row.root, selected ? theme.color_accent0 : theme.color_panel_light);
		_ui->get_paint().set_press_color(row.root, selected ? theme.color_accent0 : theme.color_light);
	}

	void editor_panel_entities_t::set_entity_row_visible(const entity_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		set_widget_visible(tree, row.root, visible, /*input=*/true);
		set_widget_visible(tree, row.icon, visible, /*input=*/false);
		set_widget_visible(tree, row.icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.label, visible, /*input=*/false);
	}

	void editor_panel_entities_t::select_entity_row(entity_id_t entity)
	{
		_selected_entity = entity;
		for (const entity_row_t& row : _entity_rows)
			update_entity_row_background(row);
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
		select_entity_row(entity);
		refresh_entities();
	}

	void editor_panel_entities_t::duplicate_entity(entity_id_t entity)
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());

		const entity_id_t duplicate = editor_commands_entity_t::duplicate(main_world, entity);
		select_entity_row(duplicate);
		refresh_entities();
	}

	void editor_panel_entities_t::destroy_entity(entity_id_t entity)
	{
		const world_handle_t main_world = editor_app_t::get().get_main_world();
		SFG_ASSERT(!main_world.is_null());

		if (editor_commands_entity_t::destroy(main_world, entity))
			select_entity_row(NULL_ENTITY_ID);
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

		_action_menu_entity = entity;

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

	void editor_panel_entities_t::on_search_changed(const char* value, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel._search_str			   = value;
		panel._search_str_lower		   = value;
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
			panel.create_entity(panel._action_menu_entity);
		else if (command == entity_action_menu_duplicate)
			panel.duplicate_entity(panel._action_menu_entity);
		else if (command == entity_action_menu_delete)
			panel.destroy_entity(panel._action_menu_entity);
	}

	void editor_panel_entities_t::on_entities_body_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (id == panel._entity_list_area)
			panel.select_entity_row(NULL_ENTITY_ID);

		if (btn == ui::mouse_button_e::right && id == panel._entity_list_area)
			panel.open_empty_action_menu(pos);
	}

	void editor_panel_entities_t::on_entities_body_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel._scrollbar.scroll_y(delta);
	}

	void editor_panel_entities_t::on_entity_tree_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (!(editor_app_t::get().get_main_world() == panel._main_world) || panel._command_generation != editor_app_t::get().get_command_system().get_generation())
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
			panel.select_entity_row(row->entity);
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

		panel.select_entity_row(row->entity);
		if (btn == ui::mouse_button_e::right)
			panel.open_entity_action_menu(pos, row->entity);
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
}
