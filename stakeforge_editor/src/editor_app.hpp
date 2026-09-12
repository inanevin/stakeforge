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

#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"
#include "editor_file_watch_controller.hpp"
#include "editor_renderer.hpp"
#include "editor_work_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/editor_payload_controller.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_runtime_config.hpp>
#include <sfg/runtime/resources/resource_preload.hpp>

namespace sfg
{
	enum class script_cursor_lock_mode_e : u8;

	enum class editor_app_mode_e : u8
	{
		none,
		normal,
		splash,
		project_creator,
	};

	struct editor_app_config_t
	{
		engine_runtime_config_t		   engine				   = {};
		editor_command_system_config_t command_system		   = {};
		editor_renderer_config_t	   renderer				   = {};
		size_t						   main_frame_budget_bytes = 4ull * 1024ull * 1024ull;
	};

	class editor_app_t final
	{
	public:
		editor_app_t();
		~editor_app_t();
		editor_app_t(const editor_app_t&)			 = delete;
		editor_app_t& operator=(const editor_app_t&) = delete;

		inline static editor_app_t& get()
		{
			static editor_app_t s_instance;
			return s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(const editor_app_config_t& config = {});
		void uninit();
		void tick();
		void stop_render();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void switch_mode(editor_app_mode_e mode);
		void request_switch_mode(editor_app_mode_e mode);
		void set_debug_mode(bool enabled);
		void set_text_subpixel_enabled(bool enabled);
		void create_payload(const char* text, editor_payload_type_e type, void* user_ptr, vec2u16_t size_value = {});

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bool is_debug_mode_enabled() const
		{
			return _debug_mode;
		}

		inline editor_world_controller_t& get_world_controller()
		{
			return _world_controller;
		}

		inline editor_work_controller_t& get_work_controller()
		{
			return _work_controller;
		}

		inline editor_command_system_t& get_command_system()
		{
			return _command_system;
		}

	private:
		bool		init_normal_mode();
		void		uninit_normal_mode();
		static u8	get_script_game_render_resolution(vec2u16_t& out_resolution);
		static u8	set_script_game_render_resolution(const vec2u16_t& resolution);
		static u8	load_script_game_world(sid_t world_name_hash);
		static u8	restart_script_game_world();
		static void quit_script_game();
		static void lock_script_cursor(script_cursor_lock_mode_e mode);

	private:
		editor_app_config_t			   _config = {};
		editor_asset_manager_t		   _asset_manager;
		editor_renderer_t			   _renderer;
		editor_command_system_t		   _command_system;
		editor_world_controller_t	   _world_controller;
		editor_work_controller_t	   _work_controller;
		editor_file_watch_controller_t _file_watch_controller;
		resource_preload_t			   _editor_resource_preload;
		resource_preload_t			   _engine_resource_preload;
		editor_payload_controller_t	   _payload_controller;
		editor_modal_progress_bar_t	   _debug_progress_modal;
		editor_work_status_t		   _splash_work_status = {};
		string_t					   _splash_displayed_progress_text;
		i64							   _last_tick_us			  = 0;
		f32							   _debug_modal_progress	  = 0.0f;
		editor_work_handle_t		   _splash_work				  = {};
		atomic_t<editor_app_mode_e>	   _pending_mode			  = editor_app_mode_e::none;
		editor_app_mode_e			   _mode					  = editor_app_mode_e::none;
		bool						   _debug_mode				  = false;
		bool						   _normal_world_load_pending = false;
	};
}
