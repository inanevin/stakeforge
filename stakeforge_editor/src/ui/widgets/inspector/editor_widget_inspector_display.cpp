/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "editor_widget_inspector.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "commands/editor_command_component_edit.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/widgets/editor_widget_entity_info.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <algorithm>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_widget_inspector_t::set_display_entity(entity_id_t entity)
	{
		const entity_id_t entities[] = {entity};
		set_display_entity({.data = entities, .size = 1});
	}

	void editor_widget_inspector_t::set_display_entity(span_t<const entity_id_t> entities)
	{
		if (entities.size == 0)
			_display_entities.resize(0);
		else
			_display_entities.assign(entities.data, entities.data + entities.size);

		refresh_display();
	}

	void editor_widget_inspector_t::refresh_display()
	{
		save_display_state();
		clear_display();
		if (!_display_entities.empty())
			create_entity_display();
	}

	void editor_widget_inspector_t::save_display_state()
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

	void editor_widget_inspector_t::clear_display()
	{
		clear_entity_info_edit();
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

	void editor_widget_inspector_t::create_entity_display()
	{
		if (_display_entities.empty())
			return;

		world_t&		  world				= editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		const bool		  prefab_referenced = _allow_prefab_blocks && is_selection_prefab_referenced();
		const bool		  prefab_blocked	= _allow_prefab_blocks && is_selection_prefab_child();
		const entity_id_t first_entity		= _display_entities.front();

		_entity_info	  = new editor_widget_entity_info_t();
		_entity_info_fold = new editor_widget_fold_t();
		_entity_info_fold->init(*_ui, _column, {.label = "Entity Info", .folded = false, .settings_button = !prefab_blocked});
		_entity_info->init(*_ui, _entity_info_fold->get_body(), {.break_prefab = on_entity_info_break_prefab, .user_data = this, .world = _edit_world, .is_prefab = prefab_referenced, .block_edits = prefab_blocked});
		_entity_info->set_name_submitted_callback(on_entity_info_name_submitted, this);
		_entity_info->set_edit_callbacks({.edit_begin = on_entity_info_edit_begin, .edit_submitted = on_entity_info_edit_submitted, .user_data = this});
		_entity_info->set_entities(world, {.data = _display_entities.data(), .size = _display_entities.size()});

		if (!prefab_blocked)
		{
			ui::listener_bundle_t entity_info_settings_listener = {};
			entity_info_settings_listener.user_data				= this;
			entity_info_settings_listener.on_click				= on_entity_info_settings_clicked;
			_ui->get_input().set_listener(_entity_info_fold->get_settings_button(), entity_info_settings_listener);
		}

		for (const ecs_component_table_t& component_table : world.get_component_tables())
		{
			if (!component_table.has(first_entity))
				continue;

			const reflected_type_t* reflected_type = reflection_registry_t::get().find_type(component_table.get_type_desc().type_id);

			if (reflected_type == nullptr || reflected_type->flags.is_set(reflected_type_flag_no_ui))
				continue;

			bool common_component = true;

			for (size_t i = 1; i < _display_entities.size(); ++i)
			{
				if (!component_table.has(_display_entities[i]))
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
			display.edit_user_data		 = new component_edit_callback_data_t{.panel = this, .component_type = component_table.get_type_desc().type_id};
			display.type_id				 = component_table.get_type_desc().type_id;
			display.objects.reserve(_display_entities.size());

			for (entity_id_t entity : _display_entities)
				display.objects.push_back(component_table.get(entity));

			component_display_state_t* state = find_component_display_state(display.type_id);
			display.fold->init(*_ui, _column, {.label = reflected_type->display_name != nullptr ? reflected_type->display_name : reflected_type->name, .folded = state != nullptr && state->folded, .settings_button = !prefab_blocked});
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
									  .objects					= {.data = display.objects.data(), .size = display.objects.size()},
									  .type_id					= component_table.get_type_desc().type_id,
									  .world					= _edit_world,
									  .dropdown_items			= resolve_dropdown_items,
									  .dropdown_items_user_data = display.edit_user_data,
									  .block_edits				= prefab_blocked,
								  });

			if (!prefab_blocked)
			{
				ui::listener_bundle_t settings_listener = {};
				settings_listener.user_data				= this;
				settings_listener.on_click				= on_component_settings_clicked;
				_ui->get_input().set_listener(display.fold->get_settings_button(), settings_listener);
			}
		}

		if (!prefab_blocked)
			create_add_component_button();
	}

	void editor_widget_inspector_t::refresh_component_reflection(sid_t component_type)
	{
		component_display_t* display = find_component_display(component_type);

		if (display == nullptr)
			return;

		display->reflect->save_fold_states();
		display->reflect->uninit();
		delete display->reflect;

		const bool prefab_blocked = _allow_prefab_blocks && is_selection_prefab_child();
		display->reflect		  = new editor_widget_reflection_t();
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
								   .objects					 = {.data = display->objects.data(), .size = display->objects.size()},
								   .type_id					 = display->type_id,
								   .world					 = _edit_world,
								   .dropdown_items			 = resolve_dropdown_items,
								   .dropdown_items_user_data = display->edit_user_data,
								   .block_edits				 = prefab_blocked,
							   });

		if (component_type == type_id_t<component_skinned_mesh_renderer_t>::value)
			refresh_component_reflection(type_id_t<component_animation_player_t>::value);
	}

	span_t<const editor_widget_reflection_dropdown_item_t> editor_widget_inspector_t::resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data)
	{
		component_edit_callback_data_t& data = *static_cast<component_edit_callback_data_t*>(user_data);

		if (data.component_type != type_id_t<component_animation_player_t>::value || field_id != "mask"_hs)
			return {};

		editor_widget_inspector_t&	 panel = *data.panel;
		world_t&					 world = editor_world_controller_t::get().get_editor_world(panel._edit_world)->get_world();
		const ecs_component_table_t& table = world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);

		panel._mask_dropdown_names.resize(0);
		panel._mask_dropdown_items.resize(0);
		panel._mask_dropdown_items.push_back({.text = "None", .value = NULL_SID});

		for (size_t entity_index = 0; entity_index < panel._display_entities.size(); ++entity_index)
		{
			const component_skinned_mesh_renderer_t* renderer = table.find_as_const<component_skinned_mesh_renderer_t>(panel._display_entities[entity_index]);
			const editor_asset_t*					 asset	  = renderer == nullptr ? nullptr : editor_asset_manager_t::get().find_asset(renderer->skeleton);

			if (asset == nullptr || asset->asset_type != editor_asset_type_e::skeleton || asset->embedded_source.empty())
			{
				panel._mask_dropdown_names.resize(0);
				break;
			}

			skeleton_def_t		 skeleton = {};
			const nlohmann::json source	  = editor_asset_io_t::get_embedded_source_json(*asset);

			if (!reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &skeleton, nullptr, source))
			{
				SFG_ERR("failed to read skeleton masks for inspector");
				panel._mask_dropdown_names.resize(0);
				break;
			}

			if (entity_index == 0)
			{
				panel._mask_dropdown_names.reserve(skeleton.masks.size());

				for (const skeleton_mask_def_t& mask : skeleton.masks)
					panel._mask_dropdown_names.emplace_back(mask.name);
			}
			else
			{
				for (auto name = panel._mask_dropdown_names.begin(); name != panel._mask_dropdown_names.end();)
				{
					const sid_t name_hash = TO_SID(name->c_str());
					const auto	mask	  = std::find_if(skeleton.masks.begin(), skeleton.masks.end(), [name_hash](const skeleton_mask_def_t& value) { return TO_SID(static_cast<const char*>(value.name)) == name_hash; });

					if (mask == skeleton.masks.end())
						name = panel._mask_dropdown_names.erase(name);
					else
						++name;
				}
			}

			if (panel._mask_dropdown_names.empty())
				break;
		}

		panel._mask_dropdown_items.reserve(panel._mask_dropdown_names.size() + 1);

		for (const string_t& name : panel._mask_dropdown_names)
			panel._mask_dropdown_items.push_back({.text = name.c_str(), .value = TO_SID(name.c_str())});

		return {.data = panel._mask_dropdown_items.data(), .size = panel._mask_dropdown_items.size()};
	}

	bool editor_widget_inspector_t::serialize_component_streams(sid_t component_type, span_t<const entity_id_t> entities, vector_t<ostream_t>& out_streams) const
	{
		out_streams.resize(0);

		if (_edit_world.is_null() || entities.size == 0)
			return false;

		world_t&			   world = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		ecs_component_table_t& table = world.get_component_table(component_type);

		out_streams.reserve(entities.size);

		for (size_t i = 0; i < entities.size; ++i)
		{
			if (!table.has(entities.data[i]))
			{
				out_streams.resize(0);
				return false;
			}

			ostream_t stream = {};

			if (!reflection_registry_t::get().type_to_stream(table.get_type_desc().type_id, table.get(entities.data[i]), nullptr, stream))
			{
				out_streams.resize(0);
				return false;
			}

			out_streams.push_back(std::move(stream));
		}

		return true;
	}

	bool editor_widget_inspector_t::read_entity_infos(span_t<const entity_id_t> entities, vector_t<editor_entity_info_data_t>& out_infos) const
	{
		out_infos.resize(0);
		if (_edit_world.is_null() || entities.size == 0)
			return false;

		world_t& world = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		out_infos.reserve(entities.size);
		for (size_t i = 0; i < entities.size; ++i)
			out_infos.push_back(editor_commands_entity_info_t::read(world, entities.data[i]));
		return true;
	}

	bool editor_widget_inspector_t::is_selection_prefab_referenced() const
	{
		world_t&					 world		  = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);

		for (entity_id_t entity : _display_entities)
		{
			if (prefab_table.has(entity))
				return true;
		}

		return false;
	}

	bool editor_widget_inspector_t::is_selection_prefab_child() const
	{
		const editor_world_t* editor_world = editor_world_controller_t::get().get_editor_world(_edit_world);
		const world_t&		  world		   = editor_world->get_world();
		for (entity_id_t entity : _display_entities)
		{
			if (!editor_world->get_edit_context().is_entity_mutation_allowed(world, entity))
				return true;
		}
		return false;
	}

	void editor_widget_inspector_t::break_prefabs()
	{
		world_t&					 world		  = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		const ecs_component_table_t& prefab_table = world.get_component_table(type_id_t<component_prefab_reference_t>::value);
		frame_vector_t<entity_id_t>	 roots		  = {};
		roots.reserve(_display_entities.size());

		for (entity_id_t entity : _display_entities)
		{
			if (!prefab_table.has(entity))
				continue;

			for (entity_id_t current = entity; current != NULL_ENTITY_ID; current = world.get_entity_parent(current))
			{
				const component_prefab_reference_t* ref = prefab_table.find_as_const<component_prefab_reference_t>(current);

				if (ref != nullptr && ref->is_root)
				{
					if (std::find(roots.begin(), roots.end(), current) == roots.end())
						roots.push_back(current);
					break;
				}
			}
		}

		for (entity_id_t root : roots)
			world_cooker_t::break_prefab_chain(world, root);

		if (!roots.empty())
		{
			editor_world_controller_t::get().mark_world_dirty(_edit_world);
			refresh_display();

			if (editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::entities))
				static_cast<editor_panel_entities_t*>(panel)->refresh_entities();
		}
	}

	void editor_widget_inspector_t::begin_entity_info_edit()
	{
		clear_entity_info_edit();
		if (_allow_prefab_blocks && is_selection_prefab_child())
			return;
		_entity_info_edit_entities.assign(_display_entities.begin(), _display_entities.end());
		if (!read_entity_infos({.data = _entity_info_edit_entities.data(), .size = _entity_info_edit_entities.size()}, _entity_info_edit_prev_infos))
		{
			clear_entity_info_edit();
			return;
		}
		_entity_info_edit_active = true;
	}

	void editor_widget_inspector_t::submit_entity_info_edit()
	{
		if (!_entity_info_edit_active)
			return;

		vector_t<editor_entity_info_data_t> post_infos;
		if (read_entity_infos({.data = _entity_info_edit_entities.data(), .size = _entity_info_edit_entities.size()}, post_infos))
		{
			editor_commands_entity_info_t::edit(_edit_world,
												{.data = _entity_info_edit_entities.data(), .size = _entity_info_edit_entities.size()},
												{.data = _entity_info_edit_prev_infos.data(), .size = _entity_info_edit_prev_infos.size()},
												{.data = post_infos.data(), .size = post_infos.size()});
		}
		clear_entity_info_edit();
	}

	void editor_widget_inspector_t::clear_entity_info_edit()
	{
		_entity_info_edit_entities.resize(0);
		_entity_info_edit_prev_infos.resize(0);
		_entity_info_edit_active = false;
	}

	void editor_widget_inspector_t::begin_component_edit(sid_t component_type)
	{
		clear_component_edit();
		if (_allow_prefab_blocks && is_selection_prefab_child())
			return;
		_component_edit_entities.assign(_display_entities.begin(), _display_entities.end());
		if (!serialize_component_streams(component_type, {.data = _component_edit_entities.data(), .size = _component_edit_entities.size()}, _component_edit_prev_streams))
		{
			clear_component_edit();
			return;
		}
		_component_edit_type   = component_type;
		_component_edit_active = true;
	}

	void editor_widget_inspector_t::submit_component_edit(sid_t component_type)
	{
		if (!_component_edit_active || _component_edit_type != component_type)
			return;

		vector_t<ostream_t> post_streams = {};

		if (serialize_component_streams(component_type, {.data = _component_edit_entities.data(), .size = _component_edit_entities.size()}, post_streams))
		{
			editor_command_component_edit_t::edit(_edit_world,
												  {.data = _component_edit_entities.data(), .size = _component_edit_entities.size()},
												  component_type,
												  {.data = _component_edit_prev_streams.data(), .size = _component_edit_prev_streams.size()},
												  {.data = post_streams.data(), .size = post_streams.size()});
		}

		clear_component_edit();

		if (component_type == type_id_t<component_skinned_mesh_renderer_t>::value)
			refresh_component_reflection(type_id_t<component_animation_player_t>::value);
	}

	void editor_widget_inspector_t::clear_component_edit()
	{
		_component_edit_entities.resize(0);
		_component_edit_prev_streams.resize(0);
		_component_edit_type   = 0;
		_component_edit_active = false;
	}

	bool editor_widget_inspector_t::is_displaying_any_entity(span_t<const entity_id_t> entities) const
	{
		for (size_t i = 0; i < entities.size; ++i)
		{
			if (std::find(_display_entities.begin(), _display_entities.end(), entities.data[i]) != _display_entities.end())
				return true;
		}
		return false;
	}

	editor_widget_inspector_t::component_display_t* editor_widget_inspector_t::find_component_display(sid_t type_id)
	{
		for (component_display_t& display : _component_displays)
		{
			if (display.type_id == type_id)
				return &display;
		}
		return nullptr;
	}

	editor_widget_inspector_t::component_display_state_t* editor_widget_inspector_t::find_component_display_state(sid_t type_id)
	{
		for (component_display_state_t& state : _component_states)
		{
			if (state.type_id == type_id)
				return &state;
		}
		return nullptr;
	}

}
