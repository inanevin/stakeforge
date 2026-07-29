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

#include "script_component_schema.hpp"

#include <sfg/common/size_definitions.hpp>
namespace sfg
{
	struct script_api_animation_t;
	struct script_api_audio_t;
	struct script_api_physics_t;
	struct script_api_platform_t;
	struct script_api_game_t;
	struct script_api_resource_t;
	struct script_api_world_t;
	struct physics_contact_event_t;

	typedef void (*script_host_log_fn)(const char* message);

	struct script_host_native_api_t
	{
		u32							  size			 = 0;
		u32							  version		 = 0;
		script_host_log_fn			  log_info		 = nullptr;
		script_host_log_fn			  log_error		 = nullptr;
		const script_api_platform_t*  platform		 = nullptr;
		const script_api_game_t*	  game			 = nullptr;
		const script_api_resource_t*  resource		 = nullptr;
		const script_api_world_t*	  world			 = nullptr;
		const script_api_audio_t*	  audio			 = nullptr;
		const script_api_physics_t*	  physics		 = nullptr;
		const script_api_animation_t* animation		 = nullptr;
		script_host_log_fn			  game_log_info	 = nullptr;
		script_host_log_fn			  game_log_error = nullptr;
		script_host_log_fn			  game_log_warn	 = nullptr;
		script_host_log_fn			  game_log_trace = nullptr;
	};

	typedef i32 (*script_host_shutdown_fn)();
	typedef i32 (*script_host_stage_project_assembly_fn)(const char* assembly_path);
	typedef i32 (*script_host_get_staged_project_schema_fn)(char* buffer, u32 capacity);
	typedef i32 (*script_host_activate_staged_project_assembly_fn)();
	typedef i32 (*script_host_discard_staged_project_assembly_fn)();
	typedef void* (*script_host_create_world_script_fn)(sid_t type_id, void* world);
	typedef i32 (*script_host_world_script_lifecycle_fn)(void* instance);
	typedef i32 (*script_host_world_script_tick_fn)(void* instance, f32 delta_time);
	typedef i32 (*script_host_world_script_key_event_fn)(void* instance, u16 key, u16 scan_code, u8 action);
	typedef i32 (*script_host_world_script_mouse_button_event_fn)(void* instance, u8 button, u8 action, f32 position_x, f32 position_y);
	typedef i32 (*script_host_world_script_mouse_move_event_fn)(void* instance, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y);
	typedef i32 (*script_host_world_script_mouse_wheel_event_fn)(void* instance, f32 position_x, f32 position_y, f32 delta);
	typedef i32 (*script_host_world_script_physics_contact_fn)(void* instance, const physics_contact_event_t* contact, u8 contact_type, u8 is_sensor);

	class script_runtime_t final
	{
	public:
		static script_runtime_t& get();

		script_runtime_t()									 = default;
		~script_runtime_t()									 = default;
		script_runtime_t(const script_runtime_t&)			 = delete;
		script_runtime_t& operator=(const script_runtime_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		bool stage_project_assembly(const char* assembly_path);
		bool activate_staged_project_assembly();
		void discard_staged_project_assembly();

		// -----------------------------------------------------------------------------
		// world script
		// -----------------------------------------------------------------------------

		void* create_world_script(sid_t type_id, void* world);
		void  destroy_world_script(void* instance);
		bool  begin_play_world_script(void* instance);
		bool  end_play_world_script(void* instance);
		bool  tick_world_script(void* instance, f32 delta_time);
		bool  post_tick_world_script(void* instance, f32 delta_time);
		bool  post_physics_tick_world_script(void* instance, f32 delta_time);
		bool  post_animation_tick_world_script(void* instance, f32 delta_time);
		bool  draw_debug_world_script(void* instance);
		bool  key_event_world_script(void* instance, u16 key, u16 scan_code, u8 action);
		bool  mouse_button_event_world_script(void* instance, u8 button, u8 action, f32 position_x, f32 position_y);
		bool  mouse_move_event_world_script(void* instance, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y);
		bool  mouse_wheel_event_world_script(void* instance, f32 position_x, f32 position_y, f32 delta);
		bool  physics_contact_world_script(void* instance, const physics_contact_event_t& contact);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		inline bool is_initialized() const
		{
			return _is_initialized;
		}

		inline bool is_project_assembly_loaded() const
		{
			return _is_project_assembly_loaded;
		}

		inline bool is_project_assembly_staged() const
		{
			return _is_project_assembly_staged;
		}

		inline const script_component_schema_t& get_component_schema() const
		{
			return _component_schema;
		}

		inline const script_component_schema_t& get_staged_component_schema() const
		{
			return _staged_component_schema;
		}

	private:
		script_component_schema_t						_component_schema					 = {};
		script_component_schema_t						_staged_component_schema			 = {};
		script_host_native_api_t						_native_api							 = {};
		void*											_hostfxr_library					 = nullptr;
		script_host_shutdown_fn							_shutdown							 = nullptr;
		script_host_stage_project_assembly_fn			_fn_stage_project_assembly			 = nullptr;
		script_host_get_staged_project_schema_fn		_fn_get_staged_project_schema		 = nullptr;
		script_host_activate_staged_project_assembly_fn _fn_activate_staged_project_schema	 = nullptr;
		script_host_discard_staged_project_assembly_fn	_fn_discard_staged_project_schema	 = nullptr;
		script_host_create_world_script_fn				_fn_create_world_script				 = nullptr;
		script_host_world_script_lifecycle_fn			_fn_destroy_world_script			 = nullptr;
		script_host_world_script_lifecycle_fn			_fn_begin_play_world_script			 = nullptr;
		script_host_world_script_lifecycle_fn			_fn_end_play_world_script			 = nullptr;
		script_host_world_script_tick_fn				_fn_tick_world_script				 = nullptr;
		script_host_world_script_tick_fn				_fn_post_tick_world_script			 = nullptr;
		script_host_world_script_tick_fn				_fn_post_physics_tick_world_script	 = nullptr;
		script_host_world_script_tick_fn				_fn_post_animation_tick_world_script = nullptr;
		script_host_world_script_lifecycle_fn			_fn_draw_debug_world_script			 = nullptr;
		script_host_world_script_key_event_fn			_fn_key_event_world_script			 = nullptr;
		script_host_world_script_mouse_button_event_fn	_fn_mouse_button_event_world_script	 = nullptr;
		script_host_world_script_mouse_move_event_fn	_fn_mouse_move_event_world_script	 = nullptr;
		script_host_world_script_mouse_wheel_event_fn	_fn_mouse_wheel_event_world_script	 = nullptr;
		script_host_world_script_physics_contact_fn		_fn_physics_contact_world_script	 = nullptr;
		bool											_is_initialized						 = false;
		bool											_is_project_assembly_loaded			 = false;
		bool											_is_project_assembly_staged			 = false;
	};
}
