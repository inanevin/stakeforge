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
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "world_edit/editor_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "commands/editor_command_component_edit.hpp"
#include "ui/widgets/editor_widget_entity_info.hpp"
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs.hpp>

namespace sfg
{
	void editor_panel_inspector_t::set_display_none()
	{
		save_scroll_state();
		save_display_state();
		_display_type		  = editor_inspector_display_type_e::none;
		_display_world		  = nullptr;
		_display_world_handle = {};
		_display_entities.resize(0);
		refresh_display();
	}

	void editor_panel_inspector_t::set_display_entity(world_handle_t world, entity_id_t entity)
	{
		const entity_id_t entities[] = {entity};
		set_display_entity(world, {.data = entities, .size = 1});
	}

	void editor_panel_inspector_t::set_display_entity(world_handle_t world, span_t<const entity_id_t> entities)
	{
		save_scroll_state();
		_display_type		  = editor_inspector_display_type_e::entity;
		_display_world_handle = world;
		_display_world		  = &editor_world_controller_t::get().get_world(world);
		_display_entities.assign(entities.data, entities.data + entities.size);
		_skip_scroll_state_save = true;
		refresh_display();
	}

	void editor_panel_inspector_t::refresh_display()
	{
		if (!can_mutate_ui_topology())
		{
			request_refresh_display();
			return;
		}

		if (!_skip_scroll_state_save)
			save_scroll_state();
		_skip_scroll_state_save = false;

		save_display_state();
		clear_display();
		if (_display_type == editor_inspector_display_type_e::entity)
			create_entity_display();
		restore_scroll_state();
	}

	void editor_panel_inspector_t::refresh_from_selection()
	{
		if (_edit_context.is_null())
		{
			set_display_none();
			return;
		}

		editor_world_edit_context_t&	controller = editor_world_controller_t::get().get_edit_context(_edit_context);
		const span_t<const entity_id_t> selected   = controller.get_selected_entities();
		const world_handle_t			world	   = controller.get_world();
		if (world.is_null() || selected.size == 0)
		{
			set_display_none();
			return;
		}

		if (selected.size == 1)
			set_display_entity(world, selected.data[0]);
		else
			set_display_entity(world, selected);
	}

	bool editor_panel_inspector_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_panel_inspector_t::request_refresh_display()
	{
		_refresh_display_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_inspector_t::request_refresh_component_reflection(sid_t component_type)
	{
		_refresh_component_pending = true;
		_pending_component_type	   = component_type;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_inspector_t::flush_pending_ui_mutations()
	{
		if (_refresh_display_pending)
		{
			_refresh_display_pending   = false;
			_refresh_component_pending = false;
			refresh_display();
			return;
		}

		if (_refresh_component_pending)
		{
			const sid_t component_type = _pending_component_type;
			_refresh_component_pending = false;
			_pending_component_type	   = 0;
			refresh_component_reflection(component_type);
		}
	}

	void editor_panel_inspector_t::save_display_state()
	{
		for (const component_display_t& display : _component_displays)
		{
			display.reflect->save_fold_states();
			component_display_state_t* state = find_component_display_state(display.type_id);
			if (state == nullptr)
			{
				_component_states.push_back({.type_id = display.type_id});
				state = &_component_states.back();
			}
			state->folded = display.fold->is_folded();
		}
	}

	void editor_panel_inspector_t::save_scroll_state()
	{
		if (_display_type != editor_inspector_display_type_e::entity || _display_entities.size() != 1)
			return;

		entity_scroll_state_t* state = find_entity_scroll_state(_display_entities[0]);
		if (state == nullptr)
		{
			_entity_scroll_states.push_back({.entity = _display_entities[0]});
			state = &_entity_scroll_states.back();
		}
		state->scroll_y = _ui->get_tree().in_const(_scroll_area).scroll_offset.y;
	}

	void editor_panel_inspector_t::restore_scroll_state()
	{
		if (_display_type != editor_inspector_display_type_e::entity || _display_entities.size() != 1)
		{
			_scroll_restore_pending = false;
			_ui->clear_pre_layout_tick(_scroll_area);
			return;
		}

		const entity_scroll_state_t* state = find_entity_scroll_state(_display_entities[0]);
		_pending_scroll_y				   = state != nullptr ? state->scroll_y : 0.0f;
		_scroll_restore_wait_ticks		   = 1;
		_scroll_restore_pending			   = true;
		_ui->set_pre_layout_tick(_scroll_area, on_scroll_restore_tick, this);
	}

	void editor_panel_inspector_t::apply_pending_scroll_restore()
	{
		if (!_scroll_restore_pending)
		{
			_ui->clear_pre_layout_tick(_scroll_area);
			return;
		}

		if (_scroll_restore_wait_ticks > 0)
		{
			--_scroll_restore_wait_ticks;
			return;
		}

		_scrollbar.set_scroll_y_immediate(_pending_scroll_y);
		_scroll_restore_pending = false;
		_ui->clear_pre_layout_tick(_scroll_area);
	}

	void editor_panel_inspector_t::clear_display()
	{
		clear_component_edit();

		if (_entity_info != nullptr)
		{
			_entity_info->uninit();
			delete _entity_info;
			_entity_info = nullptr;
		}

		if (_entity_info_fold != nullptr)
		{
			_entity_info_fold->uninit();
			delete _entity_info_fold;
			_entity_info_fold = nullptr;
		}

		if (_add_component_button != nullptr)
		{
			_add_component_button->uninit();
			delete _add_component_button;
			_add_component_button = nullptr;
		}
		for (component_display_t& display : _component_displays)
		{
			display.reflect->uninit();
			display.fold->uninit();
			delete display.reflect;
			delete display.fold;
			delete display.edit_user_data;
		}
		_component_displays.resize(0);
	}

	void editor_panel_inspector_t::create_entity_display()
	{
		if (_display_entities.empty())
			return;

		const entity_id_t first_entity = _display_entities.front();
		_entity_info				   = new editor_widget_entity_info_t();
		_entity_info_fold			   = new editor_widget_fold_t();
		_entity_info_fold->init(*_ui, _column, {.label = "Entity Info", .folded = false, .settings_button = true});
		_entity_info->init(*_ui, _entity_info_fold->get_body(), _display_world_handle);
		_entity_info->set_name_submitted_callback(on_entity_info_name_submitted, this);
		_entity_info->set_entities(*_display_world, {.data = _display_entities.data(), .size = _display_entities.size()});

		ui::listener_bundle_t entity_info_settings_listener = {};
		entity_info_settings_listener.user_data				= this;
		entity_info_settings_listener.on_click				= on_entity_info_settings_clicked;
		_ui->get_input().set_listener(_entity_info_fold->get_settings_button(), entity_info_settings_listener);

		for (const world_component_table_t& component_table : _display_world->get_component_tables())
		{
			if (!ecs_t::table_has(component_table.table, first_entity))
				continue;

			const reflected_type_t* reflected_type = reflection_registry_t::get().find_type(component_table.type_desc.type_id);
			if (reflected_type == nullptr || reflected_type->flags.is_set(reflected_type_flag_no_ui))
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

			_component_displays.push_back({});
			component_display_t& display = _component_displays.back();
			display.fold				 = new editor_widget_fold_t();
			display.reflect				 = new editor_widget_reflection_t();
			display.edit_user_data		 = new component_edit_callback_data_t{.panel = this, .component_type = component_table.type_desc.type_id};
			display.type_id				 = component_table.type_desc.type_id;
			display.objects.reserve(_display_entities.size());
			for (entity_id_t entity : _display_entities)
				display.objects.push_back(ecs_t::table_get(component_table.table, entity));

			component_display_state_t* state = find_component_display_state(display.type_id);
			display.fold->init(*_ui, _column, {.label = reflected_type->display_name != nullptr ? reflected_type->display_name : reflected_type->name, .folded = state != nullptr && state->folded, .settings_button = true});
			display.reflect->init(*_ui,
								  display.fold->get_body(),
								  {
									  .fold_states = &_field_states,
									  .callbacks =
										  {
											  .edit_begin	  = on_component_edit_begin,
											  .edit_submitted = on_component_edit_submitted,
											  .user_data	  = display.edit_user_data,
										  },
									  .objects = {.data = display.objects.data(), .size = display.objects.size()},
									  .type_id = component_table.type_desc.type_id,
									  .world   = _display_world_handle,
								  });

			ui::listener_bundle_t settings_listener = {};
			settings_listener.user_data				= this;
			settings_listener.on_click				= on_component_settings_clicked;
			_ui->get_input().set_listener(display.fold->get_settings_button(), settings_listener);
		}

		create_add_component_button();
	}

	void editor_panel_inspector_t::refresh_component_reflection(sid_t component_type)
	{
		if (!can_mutate_ui_topology())
		{
			request_refresh_component_reflection(component_type);
			return;
		}

		component_display_t* display = find_component_display(component_type);
		if (display == nullptr)
			return;

		display->reflect->save_fold_states();
		display->reflect->uninit();
		delete display->reflect;

		display->reflect = new editor_widget_reflection_t();
		display->reflect->init(*_ui,
							   display->fold->get_body(),
							   {
								   .fold_states = &_field_states,
								   .callbacks =
									   {
										   .edit_begin	   = on_component_edit_begin,
										   .edit_submitted = on_component_edit_submitted,
										   .user_data	   = display->edit_user_data,
									   },
								   .objects = {.data = display->objects.data(), .size = display->objects.size()},
								   .type_id = display->type_id,
								   .world	= _display_world_handle,
							   });
	}

	bool editor_panel_inspector_t::serialize_component_streams(sid_t component_type, span_t<const entity_id_t> entities, vector_t<ostream_t>& out_streams) const
	{
		out_streams.resize(0);
		if (_display_world == nullptr || entities.size == 0)
			return false;

		world_component_table_t* table = _display_world->get_component_table(component_type);
		if (table == nullptr)
			return false;

		out_streams.reserve(entities.size);
		for (size_t i = 0; i < entities.size; ++i)
		{
			if (!ecs_t::table_has(table->table, entities.data[i]))
			{
				out_streams.resize(0);
				return false;
			}

			ostream_t stream;
			if (!reflection_registry_t::get().type_to_stream(table->type_desc.type_id, ecs_t::table_get(table->table, entities.data[i]), nullptr, stream))
			{
				out_streams.resize(0);
				return false;
			}
			out_streams.push_back(std::move(stream));
		}
		return true;
	}

	void editor_panel_inspector_t::begin_component_edit(sid_t component_type)
	{
		clear_component_edit();
		_component_edit_entities.assign(_display_entities.begin(), _display_entities.end());
		if (!serialize_component_streams(component_type, {.data = _component_edit_entities.data(), .size = _component_edit_entities.size()}, _component_edit_prev_streams))
		{
			clear_component_edit();
			return;
		}
		_component_edit_type   = component_type;
		_component_edit_active = true;
	}

	void editor_panel_inspector_t::submit_component_edit(sid_t component_type)
	{
		if (!_component_edit_active || _component_edit_type != component_type)
			return;

		vector_t<ostream_t> post_streams;
		if (serialize_component_streams(component_type, {.data = _component_edit_entities.data(), .size = _component_edit_entities.size()}, post_streams))
		{
			editor_command_component_edit_t::edit(_display_world_handle,
												  {.data = _component_edit_entities.data(), .size = _component_edit_entities.size()},
												  component_type,
												  {.data = _component_edit_prev_streams.data(), .size = _component_edit_prev_streams.size()},
												  {.data = post_streams.data(), .size = post_streams.size()});
		}
		clear_component_edit();
	}

	void editor_panel_inspector_t::clear_component_edit()
	{
		_component_edit_entities.resize(0);
		_component_edit_prev_streams.resize(0);
		_component_edit_type   = 0;
		_component_edit_active = false;
	}

	bool editor_panel_inspector_t::is_displaying_any_entity(span_t<const entity_id_t> entities) const
	{
		if (_display_type != editor_inspector_display_type_e::entity)
			return false;

		for (size_t i = 0; i < entities.size; ++i)
		{
			if (std::find(_display_entities.begin(), _display_entities.end(), entities.data[i]) != _display_entities.end())
				return true;
		}
		return false;
	}

	editor_panel_inspector_t::component_display_t* editor_panel_inspector_t::find_component_display(sid_t type_id)
	{
		for (component_display_t& display : _component_displays)
		{
			if (display.type_id == type_id)
				return &display;
		}
		return nullptr;
	}

	editor_panel_inspector_t::component_display_state_t* editor_panel_inspector_t::find_component_display_state(sid_t type_id)
	{
		for (component_display_state_t& state : _component_states)
		{
			if (state.type_id == type_id)
				return &state;
		}
		return nullptr;
	}

	editor_panel_inspector_t::entity_scroll_state_t* editor_panel_inspector_t::find_entity_scroll_state(entity_id_t entity)
	{
		for (entity_scroll_state_t& state : _entity_scroll_states)
		{
			if (state.entity == entity)
				return &state;
		}
		return nullptr;
	}

}
