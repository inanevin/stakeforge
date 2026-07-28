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

#pragma once

#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"
#include "editor_renderer.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/editor_payload_controller.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_runtime_config.hpp>
#include <sfg/runtime/resources/resource_preload.hpp>
#include <sfg/vendor/taskflow/core/declarations.hpp>
#include <sfg/data/mutex.hpp>

namespace sfg
{
	enum class editor_app_mode_e : u8
	{
		none,
		normal,
		splash,
		project_creator,
	};

	struct editor_app_config_t
	{
		engine_runtime_config_t		   engine							 = {};
		editor_command_system_config_t command_system					 = {};
		editor_renderer_config_t	   renderer							 = {};
		size_t						   main_frame_budget_bytes			 = 4ull * 1024ull * 1024ull;
		u32							   editor_work_executor_thread_count = 4;
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

		inline tf::Executor& get_editor_work_executor()
		{
			return *_editor_work_executor;
		}

		inline bool is_debug_mode_enabled() const
		{
			return _debug_mode;
		}

		inline editor_world_controller_t& get_world_controller()
		{
			return _world_controller;
		}

		inline editor_command_system_t& get_command_system()
		{
			return _command_system;
		}

	private:
		bool		init_normal_mode();
		void		uninit_normal_mode();
		static void on_project_assets_progress(void* user_data, f32 progress, const char* progress_text);
		static u8	get_script_render_resolution(vec2u16_t& out_resolution);
		static u8	set_script_render_resolution(const vec2u16_t& resolution);

	private:
		editor_app_config_t			_config = {};
		editor_asset_manager_t		_asset_manager;
		editor_renderer_t			_renderer;
		editor_command_system_t		_command_system;
		editor_world_controller_t	_world_controller;
		resource_preload_t			_editor_resource_preload;
		resource_preload_t			_engine_resource_preload;
		editor_payload_controller_t _payload_controller;
		editor_modal_progress_bar_t _debug_progress_modal;
		unique_t<tf::Executor>		_editor_work_executor;
		string_t					_splash_progress_text;
		mutex_t						_splash_progress_text_mutex;
		i64							_last_tick_us				= 0;
		f32							_debug_modal_progress		= 0.0f;
		atomic_t<f32>				_splash_progress			= 0.0f;
		atomic_t<bool>				_splash_progress_text_dirty = false;
		atomic_t<editor_app_mode_e> _pending_mode				= editor_app_mode_e::none;
		editor_app_mode_e			_mode						= editor_app_mode_e::none;
		u8							_atlas_upload_frame_slot	= 0;
		bool						_debug_mode					= false;
	};
}
