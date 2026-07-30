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
#include <sfg/vendor/taskflow/taskflow.hpp>

namespace sfg
{
	void editor_script_manager_t::init()
	{
		SFG_ASSERT(!_initialized);

		_compile_result = {};
		_compile_state.store(compile_state_e::idle, std::memory_order_relaxed);
		_initialized				  = true;
		_compile_requested			  = false;
		_modal_open					  = false;
		_initial_activation_completed = false;
		_active_assembly_current	  = false;
	}

	void editor_script_manager_t::uninit()
	{
		SFG_ASSERT(_initialized);

		if (_compile_state.load(std::memory_order_acquire) == compile_state_e::compiling)
			editor_app_t::get().get_editor_work_executor().wait_for_all();

		script_runtime_t& script_runtime = script_runtime_t::get();

		if (script_runtime.is_project_assembly_staged())
			script_runtime.discard_staged_project_assembly();

		_compile_result = {};
		_compile_state.store(compile_state_e::idle, std::memory_order_relaxed);
		_initialized				  = false;
		_compile_requested			  = false;
		_modal_open					  = false;
		_initial_activation_completed = false;
		_active_assembly_current	  = false;
	}

	void editor_script_manager_t::tick()
	{
		SFG_ASSERT(_initialized);

		const compile_state_e compile_state = _compile_state.load(std::memory_order_acquire);

		if (compile_state == compile_state_e::idle)
		{
			if (_compile_requested)
			{
				_compile_requested = false;
				start_compile();
			}

			return;
		}

		if (compile_state == compile_state_e::compiling)
			return;

		if (_compile_requested)
		{
			_compile_requested = false;
			_compile_state.store(compile_state_e::idle, std::memory_order_relaxed);

			start_compile();
			return;
		}

		editor_modal_controller_t& modal	 = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		bool					   succeeded = compile_state == compile_state_e::succeeded;

		if (succeeded)
		{
			_progress_modal.set_progress(0.75f);
			modal.set_body_text("Loading the compiled C# assembly.");

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
		_modal_open = false;
		_compile_state.store(compile_state_e::idle, std::memory_order_relaxed);

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
		SFG_ASSERT(_initialized);

		if (_compile_state.load(std::memory_order_acquire) != compile_state_e::idle)
		{
			_compile_requested = true;
			return;
		}

		start_compile();
	}

	void editor_script_manager_t::start_compile()
	{
		SFG_ASSERT(_compile_state.load(std::memory_order_relaxed) == compile_state_e::idle);

		_compile_result			 = {};
		_active_assembly_current = false;
		_progress_modal.set_progress(0.1f);

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		if (_modal_open)
			modal.set_body_text("Building the C# script project.");
		else
		{
			const editor_modal_content_desc_t content = _progress_modal.get_content_desc();
			modal.request_modal("Compiling C# Scripts", "Building the C# script project.", false, nullptr, 0, &content);
			_modal_open = true;
		}

		const editor_project_runtime_t& project_runtime	  = editor_project_t::get()._runtime;
		const string_t					project_path	  = project_runtime.script_project_path;
		const string_t					publish_directory = project_runtime.script_library_path;

		_compile_state.store(compile_state_e::compiling, std::memory_order_relaxed);
		editor_app_t::get().get_editor_work_executor().silent_async([this, project_path, publish_directory]() {
			script_compile_result_t result = script_compiler_t::compile(project_path.c_str(), script_build_configuration_e::debug, publish_directory.c_str());
			const compile_state_e	state  = result.success ? compile_state_e::succeeded : compile_state_e::failed;

			_compile_result = std::move(result);
			_compile_state.store(state, std::memory_order_release);
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

		if (!editor_surface_controller_t::get().is_empty())
		{
			editor_panel_t* inspector_panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::inspector);

			if (inspector_panel != nullptr)
				static_cast<editor_panel_inspector_t*>(inspector_panel)->refresh_display();
		}

		SFG_INFO("activated C# scripts. Components: {0} added, {1} removed, {2} migrated, {3} reflection-only changes.", delta.added.size(), delta.removed.size(), delta.layout_changed.size(), delta.reflection_changed.size());
		return true;
	}
}
