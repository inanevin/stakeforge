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

#include "editor_play_edits.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "commands/editor_command_component_edit.hpp"
#include "commands/editor_commands_component.hpp"
#include "commands/editor_commands_entity_info.hpp"
#include "world/editor_world.hpp"
#include <sfg/core/data/istream.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/memory/frame_allocator.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/runtime/world/engine_components.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>
#include <sfg/core/vendor/util/json_utils.hpp>

namespace sfg
{
	namespace
	{
		void apply_component_changes(sid_t component_type, const nlohmann::json& before, const nlohmann::json& after, nlohmann::json& target)
		{
			if (before == after)
				return;

			if (component_type == type_id_t<quat_t>::value)
			{
				target = after;
				return;
			}

			json_util::apply_changes(before, after, target);

			const reflection_registry_t& registry = reflection_registry_t::get();
			const reflected_type_t&		 type	  = *registry.find_type(component_type);

			for (u32 i = type.fields.start; i < type.fields.end; ++i)
			{
				const reflected_field_t& field = *registry.get_field(i);

				if (!before.contains(field.name) || !after.contains(field.name) || !target.contains(field.name))
					continue;

				if (field.value_type == reflected_value_type_e::object)
					apply_component_changes(field.sub_type_id, before.at(field.name), after.at(field.name), target[field.name]);
				else if (field.value_type == reflected_value_type_e::container && field.container_ops.element_value_type == reflected_value_type_e::object)
				{
					const nlohmann::json& old_items = before.at(field.name);
					const nlohmann::json& new_items = after.at(field.name);
					nlohmann::json&		  items		= target[field.name];

					if (old_items.size() != new_items.size())
						continue;

					for (size_t item = 0; item < new_items.size() && item < items.size(); ++item)
						apply_component_changes(field.container_ops.element_sub_type_id, old_items[item], new_items[item], items[item]);
				}
			}
		}

		bool read_component_edit_stream(editor_command_system_t& system, const reflected_type_t& type, chunk_handle32_t stream_handle, nlohmann::json& data)
		{
			if (type.size == 0)
				return true;

			void*				   component = frame_allocator_tls_t::allocate(type.size, type.alignment);
			reflection_registry_t& registry	 = reflection_registry_t::get();

			registry.initialize_type(type.type_id, component);

			istream_t stream(system.get_aux_data().get<u8>(stream_handle), stream_handle.size);

			if (!registry.type_from_stream(type.type_id, component, nullptr, stream))
			{
				SFG_ERR("failed to read play edit snapshot for type {0}", type.type_id);
				return false;
			}

			return registry.type_to_json(type.type_id, component, nullptr, data);
		}
	}

	void editor_play_edits_t::begin(editor_world_handle_t world)
	{
		SFG_ASSERT(_listener.is_null() && _components.empty());

		_world	  = world;
		_listener = editor_command_system_t::get().add_listener(on_command, this, true);
	}

	void editor_play_edits_t::end()
	{
		editor_command_system_t::get().remove_listener(_listener);
		_listener = {};
		_world	  = {};
	}

	void editor_play_edits_t::clear()
	{
		_components.resize(0);
		_pending.resize(0);
		_failed = false;
	}

	bool editor_play_edits_t::capture_before(editor_world_handle_t world_handle, span_t<const entity_id_t> entities, sid_t component_type)
	{
		try
		{
			if (world_handle != _world)
				return true;

			if (_failed)
				return false;

			world_t&					 world = editor_world_controller_t::get().get_editor_world(_world)->get_world();
			const ecs_component_table_t& table = world.get_component_table(component_type);

			for (entity_id_t entity : entities)
			{
				const entity_guid_t guid	= world.get_entity_guid(entity);
				nlohmann::json		data	= nlohmann::json::object();
				const bool			written = reflection_registry_t::get().type_to_json(component_type, table.get(entity), nullptr, data);

				if (!written)
				{
					_failed = true;
					return false;
				}

				auto pending = std::find_if(_pending.begin(), _pending.end(), [&](const pending_edit_t& edit) { return edit.entity == guid && edit.component_type == component_type; });

				if (pending == _pending.end())
				{
					const string_t snapshot = data.dump();

					_pending.push_back({.original = snapshot, .before = snapshot, .authored = snapshot, .entity = guid, .component_type = component_type});
				}
				else
					pending->before = data.dump();
			}

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			_failed = true;
			return false;
		}
	}

	bool editor_play_edits_t::capture_after(editor_world_handle_t world_handle, span_t<const entity_id_t> entities, sid_t component_type)
	{
		try
		{
			if (world_handle != _world)
				return true;

			if (_failed)
				return false;

			world_t&					 world = editor_world_controller_t::get().get_editor_world(_world)->get_world();
			const ecs_component_table_t& table = world.get_component_table(component_type);

			for (entity_id_t entity : entities)
			{
				const entity_guid_t guid	= world.get_entity_guid(entity);
				auto				pending = std::find_if(_pending.begin(), _pending.end(), [&](const pending_edit_t& edit) { return edit.entity == guid && edit.component_type == component_type; });

				SFG_ASSERT(pending != _pending.end());

				nlohmann::json after   = nlohmann::json::object();
				const bool	   written = reflection_registry_t::get().type_to_json(component_type, table.get(entity), nullptr, after);

				if (!written)
				{
					_failed = true;
					return false;
				}

				const nlohmann::json before	  = nlohmann::json::parse(pending->before, nullptr, false);
				nlohmann::json		 authored = nlohmann::json::parse(pending->authored, nullptr, false);

				if (before.is_discarded() || authored.is_discarded())
				{
					SFG_ERR("invalid pending play edit JSON");
					_failed = true;
					return false;
				}

				apply_component_changes(component_type, before, after, authored);
				pending->authored = authored.dump();
			}

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			_failed = true;
			return false;
		}
	}

	bool editor_play_edits_t::apply_pending(editor_world_handle_t world_handle, entity_id_t entity, sid_t component_type, void* component, bool& applied) const
	{
		try
		{
			applied = false;

			if (_failed)
				return false;

			if (world_handle != _world)
				return true;

			const world_t&		world	= editor_world_controller_t::get().get_editor_world(_world)->get_world();
			const entity_guid_t guid	= world.get_entity_guid(entity);
			const auto			pending = std::find_if(_pending.begin(), _pending.end(), [&](const pending_edit_t& edit) { return edit.entity == guid && edit.component_type == component_type; });

			if (pending == _pending.end())
				return true;

			reflection_registry_t& registry = reflection_registry_t::get();
			nlohmann::json		   data		= nlohmann::json::object();
			const bool			   written	= registry.type_to_json(component_type, component, nullptr, data);

			if (!written)
				return false;

			const nlohmann::json original = nlohmann::json::parse(pending->original, nullptr, false);
			const nlohmann::json authored = nlohmann::json::parse(pending->authored, nullptr, false);

			if (original.is_discarded() || authored.is_discarded())
			{
				SFG_ERR("invalid pending play edit JSON");
				return false;
			}

			apply_component_changes(component_type, original, authored, data);

			const bool read = registry.type_from_json(component_type, component, nullptr, data);

			if (!read)
				return false;

			applied = true;

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			return false;
		}
	}

	void editor_play_edits_t::clear_pending(editor_world_handle_t world_handle, sid_t component_type)
	{
		if (world_handle != _world)
			return;

		std::erase_if(_pending, [&](const pending_edit_t& edit) { return edit.component_type == component_type; });
	}

	void editor_play_edits_t::on_command(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_play_edits_t& edits = *static_cast<editor_play_edits_t*>(user_data);

		if (command.world != edits._world || edits._failed)
			return;

		bool	   recorded = false;
		const bool undone	= command.state == editor_command_state_e::undone;

		for (component_edit_t& edit : edits._components)
		{
			if (edit.sequence != command.sequence)
				continue;

			edit.active = edit.initially_undone == undone;
			recorded	= true;
		}

		if (recorded)
			return;

		switch (command.type)
		{
		case editor_command_type_e::component_edit:
		case editor_command_type_e::component_add:
		case editor_command_type_e::component_remove:
		case editor_command_type_e::component_reset:
		case editor_command_type_e::component_paste:
			edits._failed = !edits.record_component_command(system, command);
			break;
		case editor_command_type_e::entity_info_edit:
		case editor_command_type_e::entity_info_paste:
			edits._failed = !edits.record_entity_info_command(system, command);
			break;
		default:
			break;
		}
	}

	bool editor_play_edits_t::record_fields(const editor_command_t& command, entity_guid_t entity, sid_t component_type, const nlohmann::json& before, const nlohmann::json& after)
	{
		try
		{
			auto				 pending = std::find_if(_pending.begin(), _pending.end(), [&](const pending_edit_t& edit) { return edit.entity == entity && edit.component_type == component_type; });
			const nlohmann::json changes =
				pending != _pending.end() ? nlohmann::json{{"before", nlohmann::json::parse(pending->original, nullptr, false)}, {"after", nlohmann::json::parse(pending->authored, nullptr, false)}} : nlohmann::json{{"before", before}, {"after", after}};

			if (changes.at("before").is_discarded() || changes.at("after").is_discarded())
			{
				SFG_ERR("invalid recorded play edit JSON");
				return false;
			}

			_components.push_back({
				.data			  = changes.dump(),
				.entity			  = entity,
				.component_type	  = component_type,
				.sequence		  = command.sequence,
				.operation		  = component_edit_e::fields,
				.initially_undone = command.state == editor_command_state_e::undone,
			});

			if (pending != _pending.end())
				_pending.erase(pending);

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			return false;
		}
	}

	bool editor_play_edits_t::record_component_command(editor_command_system_t& system, const editor_command_t& command)
	{
		try
		{
			chunk_handle32_t entities_handle  = {};
			chunk_handle32_t previous_streams = {};
			chunk_handle32_t post_streams	  = {};
			sid_t			 component_type	  = 0;
			u32				 count			  = 0;

			switch (command.type)
			{
			case editor_command_type_e::component_edit: {
				const auto& payload = system.get_payload_as<editor_command_component_edit_payload_t>(command);
				entities_handle		= payload.entities;
				component_type		= payload.component_type;
				count				= payload.count;
				previous_streams	= payload.previous_streams;
				post_streams		= payload.post_streams;
				break;
			}
			case editor_command_type_e::component_add: {
				const auto& payload = system.get_payload_as<editor_command_add_component_payload_t>(command);
				entities_handle		= payload.entities;
				component_type		= payload.component_type;
				count				= payload.count;
				break;
			}
			case editor_command_type_e::component_remove: {
				const auto& payload = system.get_payload_as<editor_command_remove_component_payload_t>(command);
				entities_handle		= payload.entities;
				component_type		= payload.component_type;
				count				= payload.count;
				break;
			}
			case editor_command_type_e::component_reset: {
				const auto& payload = system.get_payload_as<editor_command_reset_component_payload_t>(command);
				entities_handle		= payload.entities;
				component_type		= payload.component_type;
				count				= payload.count;
				break;
			}
			case editor_command_type_e::component_paste: {
				const auto& payload = system.get_payload_as<editor_command_paste_component_payload_t>(command);
				entities_handle		= payload.entities;
				component_type		= payload.component_type;
				count				= payload.count;
				break;
			}
			default:
				SFG_ASSERT(false);
				return false;
			}

			world_t&				world	 = editor_world_controller_t::get().get_editor_world(_world)->get_world();
			reflection_registry_t&	registry = reflection_registry_t::get();
			const reflected_type_t& type	 = *registry.find_type(component_type);

			if (component_type != type_id_t<component_transform_t>::value && type.flags.is_set(reflected_type_flag_no_serialization))
				return true;

			const entity_id_t*			 entities = system.get_aux_data().get<entity_id_t>(entities_handle);
			const ecs_component_table_t& table	  = world.get_component_table(component_type);
			const bool					 undone	  = command.state == editor_command_state_e::undone;

			for (u32 i = 0; i < count; ++i)
			{
				const entity_guid_t entity = world.get_entity_guid(entities[i]);

				if (command.type == editor_command_type_e::component_edit)
				{
					const auto*	   previous = system.get_aux_data().get<chunk_handle32_t>(previous_streams);
					const auto*	   post		= system.get_aux_data().get<chunk_handle32_t>(post_streams);
					nlohmann::json before	= nlohmann::json::object();
					nlohmann::json after	= nlohmann::json::object();

					if (!read_component_edit_stream(system, type, undone ? post[i] : previous[i], before) || !read_component_edit_stream(system, type, undone ? previous[i] : post[i], after))
						return false;

					if (!record_fields(command, entity, component_type, before, after))
						return false;

					continue;
				}

				const bool	   removed = !table.has(entities[i]);
				nlohmann::json data	   = nlohmann::json::object();

				if (!removed && type.size != 0)
				{
					const bool written = registry.type_to_json(component_type, table.get(entities[i]), nullptr, data);

					if (!written)
						return false;
				}

				_components.push_back({
					.data			  = data.dump(),
					.entity			  = entity,
					.component_type	  = component_type,
					.sequence		  = command.sequence,
					.operation		  = removed ? component_edit_e::remove : component_edit_e::replace,
					.initially_undone = undone,
				});
			}

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			return false;
		}
	}

	bool editor_play_edits_t::record_entity_info_command(editor_command_system_t& system, const editor_command_t& command)
	{
		try
		{
			chunk_handle32_t				 entities_handle = {};
			const editor_entity_info_data_t* previous		 = nullptr;
			const editor_entity_info_data_t* post			 = nullptr;
			u32								 count			 = 0;
			const bool						 pasted			 = command.type == editor_command_type_e::entity_info_paste;

			if (pasted)
			{
				const auto& payload = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
				entities_handle		= payload.entities;
				previous			= system.get_aux_data().get<editor_entity_info_data_t>(payload.old_infos);
				post				= &payload.info;
				count				= payload.count;
			}
			else
			{
				const auto& payload = system.get_payload_as<editor_command_edit_entity_info_payload_t>(command);
				entities_handle		= payload.entities;
				previous			= system.get_aux_data().get<editor_entity_info_data_t>(payload.previous_infos);
				post				= system.get_aux_data().get<editor_entity_info_data_t>(payload.post_infos);
				count				= payload.count;
			}

			world_t&			   world	= editor_world_controller_t::get().get_editor_world(_world)->get_world();
			reflection_registry_t& registry = reflection_registry_t::get();
			const entity_id_t*	   entities = system.get_aux_data().get<entity_id_t>(entities_handle);
			const bool			   undone	= command.state == editor_command_state_e::undone;

			for (u32 i = 0; i < count; ++i)
			{
				const editor_entity_info_data_t& old_info = previous[i];
				const editor_entity_info_data_t& new_info = post[pasted ? 0 : i];
				component_transform_t			 old_transform{.pos = old_info.pos, .rot = old_info.rot, .scale = old_info.scale};
				component_transform_t			 new_transform{.pos = new_info.pos, .rot = new_info.rot, .scale = new_info.scale};
				nlohmann::json					 before			= nlohmann::json::object();
				nlohmann::json					 after			= nlohmann::json::object();
				const sid_t						 component_type = type_id_t<component_transform_t>::value;
				const bool						 wrote_before	= registry.type_to_json(component_type, &old_transform, nullptr, before);
				const bool						 wrote_after	= registry.type_to_json(component_type, &new_transform, nullptr, after);

				if (!wrote_before || !wrote_after)
					return false;

				if (pasted)
				{
					_components.push_back({
						.data			  = (undone ? before : after).dump(),
						.entity			  = world.get_entity_guid(entities[i]),
						.component_type	  = component_type,
						.sequence		  = command.sequence,
						.operation		  = component_edit_e::replace,
						.initially_undone = undone,
					});
				}
				else if (!record_fields(command, world.get_entity_guid(entities[i]), component_type, undone ? after : before, undone ? before : after))
					return false;
			}

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			return false;
		}
	}

	bool editor_play_edits_t::apply(world_t& world, bool& changed)
	{
		try
		{
			changed = false;

			if (_failed)
			{
				SFG_ERR("play edits could not be captured completely");
				return false;
			}

			reflection_registry_t&		registry		 = reflection_registry_t::get();
			frame_vector_t<entity_id_t> changed_entities = {};

			for (const component_edit_t& edit : _components)
			{
				if (!edit.active)
					continue;

				const entity_id_t entity = world.find_by_guid(edit.entity);

				if (entity == NULL_ENTITY_ID)
					continue;

				ecs_component_table_t& table   = world.get_component_table(edit.component_type);
				const bool			   existed = table.has(entity);

				if (edit.operation == component_edit_e::remove)
				{
					if (!existed)
						continue;

					table.remove(entity);
				}
				else
				{
					if (!existed && edit.operation == component_edit_e::fields)
						continue;

					nlohmann::json before = nlohmann::json::object();

					if (existed && table.get_type_desc().size != 0)
					{
						const bool written = registry.type_to_json(edit.component_type, table.get(entity), nullptr, before);

						if (!written)
							return false;
					}

					nlohmann::json data = nlohmann::json::parse(edit.data, nullptr, false);

					if (data.is_discarded())
					{
						SFG_ERR("invalid saved play edit JSON");
						return false;
					}

					if (edit.operation == component_edit_e::fields)
					{
						nlohmann::json merged = before;
						apply_component_changes(edit.component_type, data.at("before"), data.at("after"), merged);
						data = std::move(merged);
					}

					if (existed && data == before)
						continue;

					void* component = existed ? table.get(entity) : table.add(entity);

					if (table.get_type_desc().size != 0)
					{
						changed = true;
						registry.initialize_type(edit.component_type, component);
						const bool read = registry.type_from_json(edit.component_type, component, nullptr, data);

						if (!read)
							return false;
					}
				}

				changed = true;

				if (std::find(changed_entities.begin(), changed_entities.end(), entity) == changed_entities.end())
					changed_entities.push_back(entity);
			}

			for (entity_id_t entity : changed_entities)
			{
				world.mark_entity_teleported(entity);
				world.scan_for_resources(entity, true);
			}

			return true;
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to process play edit JSON: {0}", error.what());
			return false;
		}
	}
}
