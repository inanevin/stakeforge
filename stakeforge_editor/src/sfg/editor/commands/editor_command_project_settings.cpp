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
#include "editor/commands/editor_command_project_settings.hpp"
#include "editor/editor_command_system.hpp"
#include "editor/editor_directories.hpp"
#include "editor/editor_project.hpp"
#include "editor/editor_world_controller.hpp"
#include <sfg/core/io/log.hpp>
#include <sfg/core/memory/memory.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/runtime/engine/engine_runtime.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		template <typename T> chunk_handle32_t copy_reflected_to_aux(editor_command_system_t& system, const T& value)
		{
			try
			{
				nlohmann::json json = nlohmann::json::object();

				if (!reflection_registry_t::get().type_to_json(type_id_t<T>::value, const_cast<T*>(&value), nullptr, json))
					return {};

				const string_t		   text	  = json.dump();
				const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(text.size(), alignof(char));

				SFG_MEMCPY(system.get_aux_data().get<char>(handle), text.data(), text.size());

				return handle;
			}
			catch (const nlohmann::json::exception& error)
			{
				SFG_ERR("failed to serialize project settings snapshot: {0}", error.what());
				return {};
			}
		}

		template <typename T> bool read_reflected_from_aux(editor_command_system_t& system, chunk_handle32_t handle, T& value)
		{
			const char*			 text = system.get_aux_data().get<char>(handle);
			const nlohmann::json json = nlohmann::json::parse(text, text + handle.size, nullptr, false);

			if (json.is_discarded())
				return false;

			return reflection_registry_t::get().type_from_json(type_id_t<T>::value, &value, nullptr, json);
		}

		bool apply_and_save_project_settings(const editor_command_project_settings_data_t& settings)
		{
			editor_command_project_settings_data_t normalized		= settings;
			const editor_project_t&				   project			= editor_project_t::get();
			const string_t&						   editor_path		= editor_directories_t::get_editor_settings();
			string_t							   project_contents = {};
			string_t							   editor_contents	= {};

			normalized.project.project_settings.normalize();
			normalized.editor.normalize();

			try
			{
				nlohmann::json project_json = nlohmann::json::object();
				nlohmann::json editor_json	= editor_settings_t::get();

				if (!reflection_registry_t::get().type_to_json(type_id_t<editor_project_settings_data_t>::value, &normalized.project, nullptr, project_json["settings"]) ||
					!reflection_registry_t::get().type_to_json(type_id_t<editor_settings_configurable_t>::value, &normalized.editor, nullptr, editor_json["configurable"]))
				{
					SFG_ERR("failed to serialize settings for project {0}", project._runtime.path);
					return false;
				}

				project_json["layout"] = project.layout;
				project_contents	   = project_json.dump(4);
				editor_contents		   = editor_json.dump(4);
			}
			catch (const nlohmann::json::exception& error)
			{
				SFG_ERR("failed to serialize settings for project {0}: {1}", project._runtime.path, error.what());
				return false;
			}

			const serialized_file_t files[] = {
				{
					.contents = project_contents,
					.path	  = project._runtime.path.c_str(),
				},
				{
					.contents = editor_contents,
					.path	  = editor_path.c_str(),
				},
			};

			if (!serializer_t::write_files_transactional({.data = files, .size = std::size(files)}))
				return false;

			editor_command_project_settings_t::apply(normalized);

			return true;
		}

		bool project_settings_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_edit_project_settings_payload_t& payload  = system.get_payload_as<editor_command_edit_project_settings_payload_t>(command);
			editor_command_project_settings_data_t				  settings = editor_command_project_settings_t::read();
			settings.project.last_world_guid							   = payload.previous_last_world_guid;

			if (!read_reflected_from_aux(system, payload.previous_project_settings, settings.project.project_settings))
				return false;

			if (!read_reflected_from_aux(system, payload.previous_editor_settings, settings.editor))
				return false;

			return apply_and_save_project_settings(settings);
		}

		bool project_settings_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_edit_project_settings_payload_t& payload  = system.get_payload_as<editor_command_edit_project_settings_payload_t>(command);
			editor_command_project_settings_data_t				  settings = editor_command_project_settings_t::read();
			settings.project.last_world_guid							   = payload.post_last_world_guid;

			if (!read_reflected_from_aux(system, payload.post_project_settings, settings.project.project_settings))
				return false;

			if (!read_reflected_from_aux(system, payload.post_editor_settings, settings.editor))
				return false;

			return apply_and_save_project_settings(settings);
		}

		bool project_settings_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_edit_project_settings_payload_t& payload = system.get_payload_as<editor_command_edit_project_settings_payload_t>(command);
			system.get_aux_data().free(payload.previous_project_settings);
			system.get_aux_data().free(payload.post_project_settings);
			system.get_aux_data().free(payload.previous_editor_settings);
			system.get_aux_data().free(payload.post_editor_settings);
			payload.previous_project_settings = {};
			payload.post_project_settings	  = {};
			payload.previous_editor_settings  = {};
			payload.post_editor_settings	  = {};
			return true;
		}
	}

	editor_command_project_settings_data_t editor_command_project_settings_t::read()
	{
		return {
			.project = editor_project_t::get().settings,
			.editor	 = editor_settings_t::get().configurable,
		};
	}

	void editor_command_project_settings_t::apply(const editor_command_project_settings_data_t& settings)
	{
		editor_project_t& project = editor_project_t::get();
		project.settings		  = settings.project;
		project.settings.project_settings.normalize();
		engine_runtime_t::get().update_project_settings(project.settings.project_settings);

		editor_settings_t::get().configurable = settings.editor;
		editor_settings_t::get().configurable.normalize();

		const world_physics_config_t physics_config = project.settings.project_settings.physics.make_runtime_config(project.settings.project_settings.world_physics_rate, project.settings.project_settings.max_sim_steps);
		editor_world_controller_t::get().update_physics_settings(physics_config.collision_masks, physics_config.active_collision_layers, physics_config.physics_rate, physics_config.max_sub_steps, physics_config.kinematic_sensors_collide_with_non_dynamic);
	}

	bool editor_command_project_settings_t::edit(const editor_command_project_settings_data_t& previous, const editor_command_project_settings_data_t& post)
	{
		if (previous == post)
			return true;

		editor_command_system_t&					   system = editor_command_system_t::get();
		editor_command_edit_project_settings_payload_t payload{
			.previous_project_settings = copy_reflected_to_aux(system, previous.project.project_settings),
			.post_project_settings	   = copy_reflected_to_aux(system, post.project.project_settings),
			.previous_editor_settings  = copy_reflected_to_aux(system, previous.editor),
			.post_editor_settings	   = copy_reflected_to_aux(system, post.editor),
			.previous_last_world_guid  = previous.project.last_world_guid,
			.post_last_world_guid	   = post.project.last_world_guid,
		};

		if (!payload.previous_project_settings || !payload.post_project_settings || !payload.previous_editor_settings || !payload.post_editor_settings)
		{
			const chunk_handle32_t snapshots[] = {payload.previous_project_settings, payload.post_project_settings, payload.previous_editor_settings, payload.post_editor_settings};

			for (chunk_handle32_t snapshot : snapshots)
			{
				if (snapshot)
					system.get_aux_data().free(snapshot);
			}

			SFG_ERR("failed to capture project settings command snapshots");
			return false;
		}

		const editor_command_issue_desc_t desc{
			.undo		= project_settings_undo,
			.redo		= project_settings_redo,
			.cleanup	= project_settings_cleanup,
			.debug_name = "Edit Project Settings",
			.type		= editor_command_type_e::project_settings_edit,
			.notify		= false,
		};

		const editor_command_handle_t handle = system.issue_command(desc, payload);

		if (handle.is_null())
		{
			system.get_aux_data().free(payload.previous_project_settings);
			system.get_aux_data().free(payload.post_project_settings);
			system.get_aux_data().free(payload.previous_editor_settings);
			system.get_aux_data().free(payload.post_editor_settings);
			SFG_ERR("failed to issue project settings edit command");
			return false;
		}

		return true;
	}
}
