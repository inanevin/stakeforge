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
#include "ui/panels/editor_panel_inspector.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	editor_panel_inspector_t::editor_panel_inspector_t()
	{
		set_type(editor_panel_type_e::inspector);
		set_title(editor_panel_type_to_string(editor_panel_type_e::inspector));
	}

	void editor_panel_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "inspector_column");
		tree.attach(_root, _column);

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.flags			   = ui::wf_visible;
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		column_in.size_value	   = {1.0f, 1.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;
	}

	void editor_panel_inspector_t::uninit()
	{
		clear_display();
		_display_entities.clear();
		_display_world = nullptr;
		_display_type  = editor_inspector_display_type_e::none;
		_column		   = NULL_WIDGET;

		editor_panel_t::uninit();
	}

	void editor_panel_inspector_t::set_display_none()
	{
		_display_type  = editor_inspector_display_type_e::none;
		_display_world = nullptr;
		_display_entities.resize(0);
		refresh_display();
	}

	void editor_panel_inspector_t::set_display_entity(world_t& world, entity_id_t entity)
	{
		const entity_id_t entities[] = {entity};
		set_display_entity(world, {.data = entities, .size = 1});
	}

	void editor_panel_inspector_t::set_display_entity(world_t& world, span_t<const entity_id_t> entities)
	{
		_display_type  = editor_inspector_display_type_e::entity;
		_display_world = &world;
		_display_entities.assign(entities.data, entities.data + entities.size);
		refresh_display();
	}

	void editor_panel_inspector_t::refresh_display()
	{
		clear_display();
		if (_display_type == editor_inspector_display_type_e::entity)
			create_entity_display();
	}

	void editor_panel_inspector_t::clear_display()
	{
		for (component_display_t& display : _component_displays)
		{
			display.reflect->uninit();
			display.fold->uninit();
			delete display.reflect;
			delete display.fold;
		}
		_component_displays.resize(0);
	}

	void editor_panel_inspector_t::create_entity_display()
	{
		if (_display_entities.empty())
			return;

		const entity_id_t							first_entity	 = _display_entities.front();
		const span_t<const world_component_table_t> component_tables = _display_world->get_component_tables();
		for (size_t table_index = 0; table_index < component_tables.size; ++table_index)
		{
			const world_component_table_t& component_table = component_tables.data[table_index];
			if (component_table.type_desc.flags.is_set(ecs_component_type_flags_system) || !ecs_t::table_has(component_table.table, first_entity))
				continue;

			bool common_component = true;
			for (size_t i = 1; i < _display_entities.size(); ++i)
			{
				if (!ecs_t::table_has(component_table.table, _display_entities[i]))
				{
					common_component = false;
					break;
				}
			}

			if (!common_component)
				continue;

			component_display_t display = {};
			display.fold				= new editor_widget_fold_t();
			display.reflect				= new editor_widget_reflect_type_t();

			display.fold->init(*_ui, _column, {.label = component_table.type_desc.debug_name});
			display.reflect->init(*_ui, display.fold->get_body());
			display.reflect->set_reflected_obj(_display_world->get_entity_component(first_entity, component_table.type_desc.type_id), component_table.type_desc.type_id);
			_component_displays.push_back(display);
		}
	}
}
