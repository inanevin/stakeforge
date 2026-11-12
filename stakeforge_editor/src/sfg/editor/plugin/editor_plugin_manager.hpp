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
#pragma once

#include "plugin_compiler.hpp"
#include "editor/editor_work_controller.hpp"
#include "editor/ui/editor_modal_progress_bar.hpp"

#include <sfg/core/data/span.hpp>
#include <sfg/core/runtime/plugin/game_plugin_runtime.hpp>

namespace sfg
{
	struct editor_project_runtime_t;
	struct editor_file_change_t;

	class SFG_EDITOR_API editor_plugin_manager_t final
	{
	public:
		editor_plugin_manager_t()												 = default;
		~editor_plugin_manager_t()												 = default;
		editor_plugin_manager_t(const editor_plugin_manager_t& other)			 = delete;
		editor_plugin_manager_t& operator=(const editor_plugin_manager_t& other) = delete;

		static editor_plugin_manager_t& get()
		{
			static editor_plugin_manager_t instance = {};

			return instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const editor_project_runtime_t& project);
		void prepare_startup(editor_work_context_t& context);
		void complete_startup();
		void begin_shutdown();
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void compile_project();
		void reload_project();
		void open_project();
		void set_debugging(bool enabled);
		void process_file_changes(span_t<const editor_file_change_t> changes);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool is_operation_allowed() const;

		bool is_idle() const
		{
			return _state == state_e::idle;
		}

		bool is_loaded() const
		{
			return game_plugin_runtime_t::get().is_loaded();
		}

		bool is_debugger_attached() const
		{
			return _debugger_attached;
		}

		const plugin_ide_session_t& get_ide_session() const
		{
			return _ide_session;
		}

	private:
		enum class state_e : u8
		{
			idle,
			startup,
			compiling,
			staging,
			opening_ide,
			attaching_debugger,
			detaching_debugger,
		};

		void start_work(state_e operation);
		void complete_work(bool succeeded);
		void activate_staged_module();
		bool resolve_solution_path();

	private:
		plugin_compile_result_t		_compile_result = {};
		plugin_stage_result_t		_stage_result	= {};
		editor_work_status_t		_work_status	= {};
		editor_modal_progress_bar_t _progress_modal = {};
		string_t					_build_directory;
		string_t					_output_game_dll;
		string_t					_shadow_directory;
		string_t					_solution_path;
		string_t					_diagnostics;
		string_t					_displayed_status;
		plugin_binary_stamp_t		_observed_stamp		  = {};
		plugin_binary_stamp_t		_stage_previous_stamp = {};
		plugin_ide_session_t		_ide_session		  = {};
		plugin_ide_session_t		_opened_ide_session	  = {};
		plugin_ide_session_t		_debugger_session	  = {};
		sid_t						_output_path_id		  = NULL_SID;
		i64							_retry_time_us		  = 0;
		i64							_retry_deadline_us	  = 0;
		editor_work_handle_t		_work				  = {};
		state_e						_state				  = state_e::idle;
		bool						_reload_pending		  = false;
		bool						_force_reload		  = false;
		bool						_error_pending		  = false;
		bool						_modal_open			  = false;
		bool						_stopping			  = false;
		bool						_debugger_attached	  = false;
	};
}
