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
#include "commands/editor_command_project_settings.hpp"
#include "editor_command_system.hpp"
#include "editor_project.hpp"
#include "editor_world_controller.hpp"
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		template <typename T> chunk_handle32_t copy_reflected_to_aux(editor_command_system_t& system, const T& value)
		{
			nlohmann::json json = nlohmann::json::object();
			reflection_registry_t::get().type_to_json(type_id_t<T>::value, const_cast<T*>(&value), nullptr, json);
			const string_t		   text	  = json.dump();
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(text.size(), alignof(char));
			SFG_MEMCPY(system.get_aux_data().get<char>(handle), text.data(), text.size());
			return handle;
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
			editor_command_project_settings_t::apply(settings);

			const bool project_saved = editor_project_t::get().save(editor_project_t::get()._runtime.path.c_str());
			const bool editor_saved	 = editor_settings_t::get().save();
			return project_saved && editor_saved;
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

		const physics_runtime_config_t physics_config = project.settings.project_settings.physics.make_runtime_config(project.settings.project_settings.world_physics_rate, project.settings.project_settings.max_sim_steps);
		editor_world_controller_t::get().update_physics_settings(
			physics_config.collision_masks,
			physics_config.active_collision_layers,
			physics_config.physics_rate,
			physics_config.max_sub_steps,
			physics_config.kinematic_sensors_collide_with_non_dynamic);
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
