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
#include "tools/scripting/editor_native_manager.hpp"
#include "tools/assets/editor_asset_path.hpp"
#include "tools/editor_app.hpp"
#include "tools/editor_file_watch_controller.hpp"
#include "tools/editor_project.hpp"
#include "tools/editor_surface_controller.hpp"
#include "tools/editor_world_controller.hpp"
#include "tools/world/editor_world.hpp"
#include "tools/ui/panels/editor_panel_inspector.hpp"
#include "tools/ui/panels/editor_primary_base.hpp"
#include "tools/ui/panels/editor_secondary_base.hpp"
#include "tools/ui/editor_action_menu_controller.hpp"

#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/time.hpp>

namespace sfg
{
#define NATIVE_RETRY_INTERVAL_US 250000
#define NATIVE_RETRY_TIMEOUT_US	 5000000

	void editor_native_manager_t::init(const editor_project_runtime_t& project)
	{
		SFG_ASSERT(!plugin_runtime_t::get().is_loaded() && _work.is_null());

		_build_directory   = project.native_intermediate_path;
		_output_dll		   = file_system_t::get_absolute_path((project.native_library_path + SFG_CONFIGURATION + "/stakeforge_game.dll").c_str());
		_shadow_directory  = project.native_intermediate_path + "Loaded/";
		_output_path_id	   = editor_asset_path_t::hash_path(_output_dll.c_str());
		_observed_stamp	   = {};
		_compile_result	   = {};
		_stage_result	   = {};
		_ide_session	   = {};
		_state			   = state_e::startup;
		_stopping		   = false;
		_reload_pending	   = false;
		_force_reload	   = false;
		_error_pending	   = false;
		_modal_open		   = false;
		_retry_time_us	   = 0;
		_retry_deadline_us = 0;
	}

	void editor_native_manager_t::prepare_startup(editor_work_context_t& context)
	{
		context.set_progress(0.1f, "Generating C++ project");

		if (!editor_project_t::get().ensure_native_project())
		{
			_stage_result.diagnostics = "Could not generate the C++ project. See the editor log and native generation.log for details.";
			return;
		}

		if (!file_system_t::exists(_output_dll.c_str()))
		{
			context.set_progress(0.4f, "Building C++ project for the first time");
			_compile_result = native_compiler_t::compile(_build_directory.c_str(), _output_dll.c_str());

			if (!_compile_result.success)
			{
				_stage_result.diagnostics = _compile_result.diagnostics;
				return;
			}
		}

		context.set_progress(0.9f, "Preparing game DLL");
		_stage_result = native_compiler_t::stage(_output_dll.c_str(), _shadow_directory.c_str(), {});
	}

	void editor_native_manager_t::complete_startup()
	{
		_state = state_e::idle;

		if (_stopping)
			return;

		if (_stage_result.state == native_stage_state_e::ready)
			activate_staged_module();
		else if (_stage_result.state == native_stage_state_e::retry)
			_reload_pending = true;
		else
		{
			_diagnostics   = _stage_result.diagnostics;
			_error_pending = true;
			SFG_ERR("{0}", _diagnostics);
		}
	}

	void editor_native_manager_t::begin_shutdown()
	{
		_stopping		= true;
		_reload_pending = false;
	}

	void editor_native_manager_t::uninit()
	{
		SFG_ASSERT(_work.is_null());

		if (plugin_runtime_t::get().is_staged())
			plugin_runtime_t::get().discard_staged(_diagnostics);

		if (plugin_runtime_t::get().is_loaded())
			plugin_runtime_t::get().unload(_diagnostics);

		_state		   = state_e::idle;
		_ide_session   = {};
		_error_pending = false;
		_modal_open	   = false;
	}

	bool editor_native_manager_t::is_operation_allowed() const
	{
		if (_stopping || _state != state_e::idle || !editor_world_controller_t::is_initialized() || editor_surface_controller_t::get().is_any_modal_active())
			return false;

		const editor_world_controller_t& worlds		= editor_world_controller_t::get();
		const editor_world_handle_t		 main_world = worlds.get_main_world_handle();

		if (main_world.is_null())
			return true;

		const editor_play_session_t& session = worlds.get_play_session();

		return !session.is_playing() && !session.is_play_requested();
	}

	void editor_native_manager_t::compile_project()
	{
		if (!is_operation_allowed())
			return;

		_force_reload	   = true;
		_retry_deadline_us = 0;
		start_work(state_e::compiling);
	}

	void editor_native_manager_t::reload_project()
	{
		if (!is_operation_allowed())
			return;

		_reload_pending	   = true;
		_force_reload	   = true;
		_retry_time_us	   = 0;
		_retry_deadline_us = 0;
	}

	void editor_native_manager_t::open_project()
	{
		if (_stopping || !is_idle())
			return;

		_solution_path = _build_directory + "stakeforge_game.slnx";

		if (!file_system_t::exists(_solution_path.c_str()))
			_solution_path = _build_directory + "stakeforge_game.sln";

		if (!file_system_t::exists(_solution_path.c_str()))
		{
			_diagnostics   = "Could not find the C++ solution in " + _build_directory + ". Recompile C++ Project to regenerate it.";
			_error_pending = true;
			SFG_ERR("{0}", _diagnostics);
			return;
		}

		start_work(state_e::opening_ide);
	}

	void editor_native_manager_t::process_file_changes(span_t<const editor_file_change_t> changes)
	{
		for (const editor_file_change_t& change : changes)
		{
			if (change.root != editor_file_watch_root_e::native || (change.path_id != _output_path_id && change.path_id != NULL_SID))
				continue;

			_reload_pending	   = true;
			_retry_time_us	   = 0;
			_retry_deadline_us = 0;
		}
	}

	void editor_native_manager_t::tick()
	{
		if (_stopping)
			return;

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		if (!_work.is_null())
		{
			editor_app_t::get().get_work_controller().get_work_status(_work, _work_status);
			_progress_modal.set_progress(_work_status.progress);

			if (_displayed_status != _work_status.text)
			{
				_displayed_status = _work_status.text;
				modal.set_body_text(_displayed_status.c_str());
			}

			return;
		}

		if (modal.is_visible())
			return;

		if (_error_pending)
		{
			const editor_modal_button_desc_t buttons[] = {{.text = "Close"}};
			modal.request_modal("C++ Project Failed", _diagnostics.c_str(), buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			_error_pending = false;
			return;
		}

		if (_reload_pending && is_operation_allowed() && time_t::get_cpu_microseconds() >= _retry_time_us)
			start_work(state_e::staging);
	}

	void editor_native_manager_t::start_work(state_e operation)
	{
		SFG_ASSERT(_work.is_null());

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		if (modal.is_visible())
			return;

		_compile_result		  = {};
		_stage_result		  = {};
		_stage_previous_stamp = _force_reload ? native_binary_stamp_t{} : _observed_stamp;
		_error_pending		  = false;
		_state				  = operation;

		if (operation != state_e::opening_ide)
			_reload_pending = false;

		_progress_modal.set_progress(0.05f);
		_displayed_status						  = operation == state_e::opening_ide ? "Opening the C++ solution in Visual Studio" : operation == state_e::compiling ? "Building the C++ project" : "Preparing the game DLL";
		const char*						  title	  = operation == state_e::opening_ide ? "Opening Visual Studio" : operation == state_e::compiling ? "Compiling C++ Project" : "Reloading C++ Project";
		const editor_modal_content_desc_t content = _progress_modal.get_content_desc();
		modal.request_modal(title, _displayed_status.c_str(), false, nullptr, 0, &content);
		_modal_open = true;

		if (_retry_deadline_us == 0 && operation == state_e::staging)
			_retry_deadline_us = time_t::get_cpu_microseconds() + NATIVE_RETRY_TIMEOUT_US;

		_work = editor_app_t::get().get_work_controller().submit_work({
			.fn =
				[](editor_work_context_t& context, void* user_data) {
					editor_native_manager_t& manager = *static_cast<editor_native_manager_t*>(user_data);

					if (manager._state == state_e::opening_ide)
					{
						context.set_progress(0.2f, "Waiting for Visual Studio to open");
						return native_compiler_t::open_project(manager._solution_path.c_str(), manager._opened_ide_session, manager._diagnostics);
					}

					if (manager._state == state_e::compiling)
					{
						context.set_progress(0.1f, "Generating C++ project");

						if (!editor_project_t::get().ensure_native_project())
						{
							manager._compile_result.diagnostics = "Could not generate the C++ project. See generation.log and the editor log.";
							return false;
						}

						context.set_progress(0.3f, "Building C++ project");
						manager._compile_result = native_compiler_t::compile(manager._build_directory.c_str(), manager._output_dll.c_str());

						if (!manager._compile_result.success)
							return false;
					}

					context.set_progress(0.9f, "Copying game DLL and symbols");
					manager._stage_result = native_compiler_t::stage(manager._output_dll.c_str(), manager._shadow_directory.c_str(), manager._stage_previous_stamp);
					return true;
				},
			.completed =
				[](editor_work_handle_t handle, editor_work_state_e state, void* user_data) {
					editor_native_manager_t& manager = *static_cast<editor_native_manager_t*>(user_data);
					manager.complete_work(state == editor_work_state_e::succeeded);
				},
			.user_data		= this,
			.initial_status = _displayed_status.c_str(),
		});
	}

	void editor_native_manager_t::complete_work(bool succeeded)
	{
		const state_e operation = _state;
		_work					= {};
		_state					= state_e::idle;

		if (_modal_open && !editor_surface_controller_t::get().is_empty())
		{
			_progress_modal.set_progress(1.0f);
			editor_surface_controller_t::get().get_main_surface().modal_controller->close_modal();
		}

		_modal_open = false;

		if (_stopping)
			return;

		if (operation == state_e::opening_ide)
		{
			_ide_session   = _opened_ide_session;
			_error_pending = !succeeded;
			return;
		}

		if (succeeded && _retry_deadline_us == 0)
			_retry_deadline_us = time_t::get_cpu_microseconds() + NATIVE_RETRY_TIMEOUT_US;

		if (!succeeded)
		{
			_observed_stamp = _compile_result.stamp;
			_diagnostics	= _compile_result.diagnostics;
			_error_pending	= true;
			_reload_pending = false;
		}
		else if (_stage_result.state == native_stage_state_e::retry && time_t::get_cpu_microseconds() < _retry_deadline_us)
		{
			_reload_pending = true;
			_retry_time_us	= time_t::get_cpu_microseconds() + NATIVE_RETRY_INTERVAL_US;
			return;
		}
		else if (_stage_result.state == native_stage_state_e::ready)
		{
			if (!is_operation_allowed())
			{
				_reload_pending	   = true;
				_retry_deadline_us = 0;
				return;
			}

			activate_staged_module();
		}
		else if (_stage_result.state != native_stage_state_e::unchanged)
		{
			_diagnostics   = _stage_result.diagnostics;
			_error_pending = true;
			SFG_ERR("{0}", _diagnostics);
		}

		_force_reload	   = false;
		_retry_deadline_us = 0;
	}

	void editor_native_manager_t::activate_staged_module()
	{
		plugin_runtime_t& plugin = plugin_runtime_t::get();
		_observed_stamp			 = _stage_result.stamp;

		if (!plugin.stage(_stage_result.path.c_str(), _diagnostics))
		{
			_error_pending = true;
			return;
		}

		editor_app_t::get().stop_render();

		vector_t<editor_panel_inspector_t*> inspectors = {};

		for (editor_surface_t& surface : editor_surface_controller_t::get())
		{
			if (surface.type != editor_surface_type_e::primary && surface.type != editor_surface_type_e::secondary)
				continue;

			surface.action_menu_controller->close_action_menu();
			const dock_widget_t& dock = surface.type == editor_surface_type_e::primary ? surface.primary->get_dock_widget() : surface.secondary->get_dock_widget();

			for (const dock_node_t& node : dock.get_nodes())
			{
				for (editor_panel_t* panel : node.panels)
				{
					if (panel->get_type() != editor_panel_type_e::inspector)
						continue;

					editor_panel_inspector_t* inspector = static_cast<editor_panel_inspector_t*>(panel);
					inspector->prepare_component_reload();
					inspectors.push_back(inspector);
				}
			}
		}

		if (!plugin.activate_staged(_diagnostics, [](const native_component_schema_t& previous, const native_component_schema_t& current, const native_component_schema_delta_t& delta) {
				if (editor_world_controller_t::is_initialized())
					editor_world_controller_t::get().apply_native_component_schema(previous, current, delta);
			}))
		{
			_error_pending				 = true;
			string_t discard_diagnostics = {};

			if (!plugin.discard_staged(discard_diagnostics))
				_diagnostics += "\n" + discard_diagnostics;
		}

		for (editor_panel_inspector_t* inspector : inspectors)
			inspector->refresh_display();
	}
}
