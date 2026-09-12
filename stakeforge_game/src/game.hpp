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

#include "game_renderer.hpp"
#include "game_world_controller.hpp"

#include <sfg/data/string.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/engine/engine_runtime_config.hpp>
#include <sfg/runtime/project/project_package_meta.hpp>

namespace sfg
{
	enum class script_cursor_lock_mode_e : u8;

	struct game_config_t
	{
		engine_runtime_config_t engine					= {};
		game_renderer_config_t	renderer				= {};
		size_t					main_frame_budget_bytes = 4ull * 1024ull * 1024ull;
	};

	class game_t final
	{
	public:
		game_t()						 = default;
		~game_t()						 = default;
		game_t(const game_t&)			 = delete;
		game_t& operator=(const game_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(const game_config_t& config = {});
		void uninit();
		void run();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const char* get_init_failure_reason() const
		{
			return _init_failure_reason.c_str();
		}

	private:
		static void on_window_event(void* window_handle, const window_event_t& event, void* user_data);
		static u8	get_script_game_render_resolution(vec2u16_t& out_resolution);
		static u8	set_script_game_render_resolution(const vec2u16_t& resolution);
		static u8	load_script_game_world(sid_t world_name_hash);
		static u8	restart_script_game_world();
		static void quit_script_game();
		static void lock_script_cursor(script_cursor_lock_mode_e mode);

		bool load_package_resources();
		bool load_project_scripts();
		bool apply_pending_world_load();
		bool fail_init(const char* reason);
		void cleanup();

		static inline game_t* s_instance = nullptr;

		game_config_t			_config				 = {};
		project_package_meta_t	_package_meta		 = {};
		window_runtime_t		_window				 = {};
		string_t				_init_failure_reason = {};
		string_t				_package_directory	 = {};
		game_world_controller_t _world_controller;
		game_renderer_t			_renderer;
		bool					_renderer_initialized		  = false;
		bool					_world_controller_initialized = false;
		bool					_window_initialized			  = false;
		bool					_frame_allocator_initialized  = false;
		bool					_runtime_initialized		  = false;
		bool					_backend_initialized		  = false;
		bool					_globals_initialized		  = false;
		bool					_script_api_bound			  = false;
		bool					_initialized				  = false;
	};
}
