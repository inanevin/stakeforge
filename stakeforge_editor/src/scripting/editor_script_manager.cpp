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

#include "editor_script_manager.hpp"
#include "editor_app.hpp"
#include "editor_project.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/editor_panel_inspector.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/scripting/script_runtime.hpp>

namespace sfg
{
	void editor_script_manager_t::init()
	{
		_compile_result = {};
		_compile_project_path.resize(0);
		_compile_publish_directory.resize(0);
		_compile_state				  = compile_state_e::idle;
		_initial_activation_completed = false;
		_active_assembly_current	  = false;
	}

	void editor_script_manager_t::uninit()
	{
		script_runtime_t& script_runtime = script_runtime_t::get();

		if (script_runtime.is_project_assembly_staged())
			script_runtime.discard_staged_project_assembly();

		_compile_result = {};
		_compile_project_path.resize(0);
		_compile_publish_directory.resize(0);
		_compile_state				  = compile_state_e::idle;
		_initial_activation_completed = false;
		_active_assembly_current	  = false;
	}

	void editor_script_manager_t::complete_compile(bool succeeded)
	{
		if (editor_surface_controller_t::get().is_empty())
		{
			_compile_state = compile_state_e::idle;
			return;
		}

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		if (succeeded)
		{
			script_runtime_t& script_runtime = script_runtime_t::get();

			if (script_runtime.is_project_assembly_staged())
				script_runtime.discard_staged_project_assembly();

			const editor_project_runtime_t& project_runtime		 = editor_project_t::get()._runtime;
			const string_t					script_assembly_path = project_runtime.script_library_path + project_runtime.name + ".dll";

			if (!script_runtime.stage_project_assembly(script_assembly_path.c_str()))
			{
				_compile_result.diagnostics = "Compilation succeeded, but the C# project assembly could not be staged.";
				SFG_ERR("could not stage the compiled C# project assembly.");
				succeeded = false;
			}
			else if (!activate_staged_scripts())
			{
				_compile_result.diagnostics = "Compilation succeeded, but the C# project assembly could not be activated.";
				succeeded					= false;
			}
		}

		_progress_modal.set_progress(1.0f);
		modal.close_modal();
		_compile_state = compile_state_e::idle;

		if (succeeded)
		{
			_initial_activation_completed = true;
			_active_assembly_current	  = true;
			return;
		}

		if (_compile_result.diagnostics.empty())
			_compile_result.diagnostics = "The C# script project could not be compiled.";

		SFG_ERR("could not compile the C# script project. Exit code: {0}\n{1}", _compile_result.exit_code, _compile_result.diagnostics);

		const editor_modal_button_desc_t buttons[] = {
			{.text = "Close"},
		};

		modal.request_modal("C# Compilation Failed", _compile_result.diagnostics.c_str(), buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
	}

	void editor_script_manager_t::compile_scripts()
	{
		SFG_ASSERT(_compile_state == compile_state_e::idle);

		_compile_result			 = {};
		_active_assembly_current = false;
		_progress_modal.set_progress(0.1f);

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		const editor_modal_content_desc_t content = _progress_modal.get_content_desc();
		modal.request_modal("Compiling C# Scripts", "Building the C# script project.", false, nullptr, 0, &content);

		const editor_project_runtime_t& project_runtime = editor_project_t::get()._runtime;

		_compile_project_path	   = project_runtime.script_project_path;
		_compile_publish_directory = project_runtime.script_library_path;
		_compile_state			   = compile_state_e::compiling;

		editor_app_t::get().get_work_controller().submit_work({
			.fn =
				[](editor_work_context_t& context, void* user_data) {
					editor_script_manager_t& script_manager = *static_cast<editor_script_manager_t*>(user_data);

					script_manager._compile_result = script_compiler_t::compile(script_manager._compile_project_path.c_str(), script_build_configuration_e::debug, script_manager._compile_publish_directory.c_str());

					return script_manager._compile_result.success;
				},
			.completed =
				[](editor_work_handle_t handle, editor_work_state_e state, void* user_data) {
					editor_script_manager_t& script_manager = *static_cast<editor_script_manager_t*>(user_data);

					script_manager.complete_compile(state == editor_work_state_e::succeeded);
				},
			.user_data		= this,
			.initial_status = "Building the C# script project",
		});
	}

	bool editor_script_manager_t::activate_staged_scripts()
	{
		script_runtime_t&					  script_runtime		   = script_runtime_t::get();
		const script_component_schema_t		  current_schema		   = script_runtime.get_component_schema();
		const script_component_schema_t&	  candidate_schema		   = script_runtime.get_staged_component_schema();
		const script_component_schema_delta_t delta					   = current_schema.compare(candidate_schema);
		const bool							  component_layout_changed = !delta.added.empty() || !delta.removed.empty() || !delta.layout_changed.empty();

		if (editor_world_controller_t::is_initialized())
			editor_world_controller_t::get().prepare_script_assembly_reload(component_layout_changed);

		if (!script_runtime.activate_staged_project_assembly())
		{
			script_runtime.discard_staged_project_assembly();

			if (editor_world_controller_t::is_initialized())
				editor_world_controller_t::get().complete_script_assembly_reload();

			SFG_ERR("could not activate the compiled C# project assembly.");
			return false;
		}

		const script_component_schema_t& active_schema		 = script_runtime.get_component_schema();
		reflection_registry_t&			 reflection_registry = reflection_registry_t::get();

		reflection_registry.remove_script_types();
		active_schema.register_reflection_types();

		if (editor_world_controller_t::is_initialized())
		{
			editor_world_controller_t::get().apply_script_component_schema(current_schema, active_schema, delta);
			editor_world_controller_t::get().complete_script_assembly_reload();
		}

		editor_panel_t* inspector_panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::inspector);

		if (inspector_panel != nullptr)
			static_cast<editor_panel_inspector_t*>(inspector_panel)->refresh_display();

		SFG_INFO("activated C# scripts. Components: {0} added, {1} removed, {2} migrated, {3} reflection-only changes.", delta.added.size(), delta.removed.size(), delta.layout_changed.size(), delta.reflection_changed.size());
		return true;
	}
}
