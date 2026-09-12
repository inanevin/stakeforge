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

#include "script_compiler.hpp"
#include "ui/editor_modal_progress_bar.hpp"

namespace sfg
{
	class editor_script_manager_t final
	{
	public:
		editor_script_manager_t()										   = default;
		~editor_script_manager_t()										   = default;
		editor_script_manager_t(const editor_script_manager_t&)			   = delete;
		editor_script_manager_t& operator=(const editor_script_manager_t&) = delete;

		inline static editor_script_manager_t& get()
		{
			static editor_script_manager_t instance = {};

			return instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void compile_scripts();

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		inline bool is_initial_activation_completed() const
		{
			return _initial_activation_completed;
		}

		inline bool is_compile_idle() const
		{
			return _compile_state == compile_state_e::idle;
		}

		inline bool is_active_assembly_current() const
		{
			return _active_assembly_current;
		}

	private:
		enum class compile_state_e : u8
		{
			idle,
			compiling,
		};

		void complete_compile(bool succeeded);
		bool activate_staged_scripts();

	private:
		script_compile_result_t		_compile_result				  = {};
		editor_modal_progress_bar_t _progress_modal				  = {};
		string_t					_compile_project_path		  = {};
		string_t					_compile_publish_directory	  = {};
		compile_state_e				_compile_state				  = compile_state_e::idle;
		bool						_initial_activation_completed = false;
		bool						_active_assembly_current	  = false;
	};
}
