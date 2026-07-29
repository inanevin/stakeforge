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

#include "script_runtime.hpp"

#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/physics/physics_types.hpp>
#include <sfg/runtime/scripting/api/script_api_animation.hpp>
#include <sfg/runtime/scripting/api/script_api_audio.hpp>
#include <sfg/runtime/scripting/api/script_api_game.hpp>
#include <sfg/runtime/scripting/api/script_api_physics.hpp>
#include <sfg/runtime/scripting/api/script_api_platform.hpp>
#include <sfg/runtime/scripting/api/script_api_resource.hpp>
#include <sfg/runtime/scripting/api/script_api_world.hpp>

#ifdef SFG_PLATFORM_WINDOWS
#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <nethost.h>
#endif

namespace sfg
{
	namespace
	{
		void script_host_log_info(const char* message)
		{
			SFG_INFO("{0}", message);
		}

		void script_host_log_error(const char* message)
		{
			SFG_ERR("{0}", message);
		}

		void script_host_game_log_info(const char* message)
		{
			SFG_GAME_INFO("{0}", message);
		}

		void script_host_game_log_error(const char* message)
		{
			SFG_GAME_ERR("{0}", message);
		}

		void script_host_game_log_warn(const char* message)
		{
			SFG_GAME_WARN("{0}", message);
		}

		void script_host_game_log_trace(const char* message)
		{
			SFG_GAME_TRACE("{0}", message);
		}

#ifdef SFG_PLATFORM_WINDOWS
		void unload_hostfxr(HMODULE hostfxr_library)
		{
			const BOOL unload_result = FreeLibrary(hostfxr_library);

			if (unload_result == 0)
				SFG_ERR("could not unload hostfxr, error: {0}", GetLastError());
		}
#endif
	}

	script_runtime_t& script_runtime_t::get()
	{
		static script_runtime_t s_instance;

		return s_instance;
	}

	bool script_runtime_t::init()
	{
		SFG_ASSERT(!_is_initialized);

#ifdef SFG_PLATFORM_WINDOWS
		typedef i32 (*script_host_initialize_fn)(const script_host_native_api_t* api);

		string_t managed_directory = file_system_t::get_running_directory();
		managed_directory += "managed/";

		const string_t	runtime_config_path		 = managed_directory + "Stakeforge.ScriptHost.runtimeconfig.json";
		const string_t	assembly_path			 = managed_directory + "Stakeforge.ScriptHost.dll";
		const wstring_t runtime_config_path_wide = string_util::to_wstr(runtime_config_path);
		const wstring_t assembly_path_wide		 = string_util::to_wstr(assembly_path);

		char_t	  hostfxr_path[MAX_PATH] = {};
		size_t	  hostfxr_path_size		 = MAX_PATH;
		const i32 get_hostfxr_result	 = get_hostfxr_path(hostfxr_path, &hostfxr_path_size, nullptr);

		if (get_hostfxr_result != 0)
		{
			SFG_ERR("could not locate hostfxr, error: {0}", get_hostfxr_result);
			return false;
		}

		HMODULE hostfxr_library = LoadLibraryW(hostfxr_path);

		if (hostfxr_library == nullptr)
		{
			SFG_ERR("could not load hostfxr, error: {0}", GetLastError());
			return false;
		}

		// hostfxr functors
		const hostfxr_initialize_for_runtime_config_fn hostfxr_initialize	= reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(GetProcAddress(hostfxr_library, "hostfxr_initialize_for_runtime_config"));
		const hostfxr_get_runtime_delegate_fn		   hostfxr_get_delegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddress(hostfxr_library, "hostfxr_get_runtime_delegate"));
		const hostfxr_close_fn						   hostfxr_close		= reinterpret_cast<hostfxr_close_fn>(GetProcAddress(hostfxr_library, "hostfxr_close"));

		if (hostfxr_initialize == nullptr || hostfxr_get_delegate == nullptr || hostfxr_close == nullptr)
		{
			SFG_ERR("could not resolve the required hostfxr functions.");
			unload_hostfxr(hostfxr_library);
			return false;
		}

		// init host
		hostfxr_handle host_context		 = nullptr;
		const i32	   initialize_result = hostfxr_initialize(runtime_config_path_wide.c_str(), nullptr, &host_context);

		if (initialize_result < 0 || host_context == nullptr)
		{
			SFG_ERR("could not initialize the .NET runtime, error: {0}", initialize_result);
			unload_hostfxr(hostfxr_library);
			return false;
		}

		// get bridge load
		void*	  load_assembly_delegate = nullptr;
		const i32 delegate_result		 = hostfxr_get_delegate(host_context, hdt_load_assembly_and_get_function_pointer, &load_assembly_delegate);
		const i32 close_result			 = hostfxr_close(host_context);

		if (close_result != 0)
			SFG_WARN("could not close the hostfxr context cleanly, error: {0}", close_result);

		if (delegate_result != 0 || load_assembly_delegate == nullptr)
		{
			SFG_ERR("could not get the managed assembly loader, error: {0}", delegate_result);
			unload_hostfxr(hostfxr_library);
			return false;
		}

		// load bridge assmbly functions
		const load_assembly_and_get_function_pointer_fn fn_load_managed						= reinterpret_cast<load_assembly_and_get_function_pointer_fn>(load_assembly_delegate);
		const char_t* const								managed_type						= L"SFG.ScriptHost.NativeEntryPoints, Stakeforge.ScriptHost";
		void*											fn_init								= nullptr;
		void*											fn_shtdown							= nullptr;
		void*											fn_stage_project_assembly			= nullptr;
		void*											fn_get_staged_project_schema		= nullptr;
		void*											fn_activate_staged_project_assembly = nullptr;
		void*											fn_discard_staged_project_assembly	= nullptr;
		void*											fn_create_world_script				= nullptr;
		void*											fn_destroy_world_script				= nullptr;
		void*											fn_begin_play_world_script			= nullptr;
		void*											fn_end_play_world_script			= nullptr;
		void*											fn_tick_world_script				= nullptr;
		void*											fn_post_tick_world_script			= nullptr;
		void*											fn_post_physics_tick_world_script	= nullptr;
		void*											fn_post_animation_tick_world_script = nullptr;
		void*											fn_draw_debug_world_script			= nullptr;
		void*											fn_key_event_world_script			= nullptr;
		void*											fn_mouse_button_event_world_script	= nullptr;
		void*											fn_mouse_move_event_world_script	= nullptr;
		void*											fn_mouse_wheel_event_world_script	= nullptr;
		void*											fn_physics_contact_world_script		= nullptr;

		const i32 res_load_init					   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"Initialize", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_init);
		const i32 res_load_shutdown				   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"Shutdown", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_shtdown);
		const i32 res_stage_project_assembly	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"StageProjectAssembly", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_stage_project_assembly);
		const i32 res_get_staged_project_schema	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"GetStagedProjectSchema", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_get_staged_project_schema);
		const i32 res_activate_staged_schema	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"ActivateStagedProjectAssembly", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_activate_staged_project_assembly);
		const i32 res_discard_staged_schema		   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"DiscardStagedProjectAssembly", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_discard_staged_project_assembly);
		const i32 res_create_world_script		   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"CreateWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_create_world_script);
		const i32 res_destroy_world_script		   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"DestroyWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_destroy_world_script);
		const i32 res_begin_play_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"BeginPlayWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_begin_play_world_script);
		const i32 res_end_play_world_script		   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"EndPlayWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_end_play_world_script);
		const i32 res_tick_world_script			   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"TickWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_tick_world_script);
		const i32 res_post_tick_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"PostTickWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_post_tick_world_script);
		const i32 res_post_physics_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"PostPhysicsTickWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_post_physics_tick_world_script);
		const i32 res_post_animation_world_script  = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"PostAnimationTickWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_post_animation_tick_world_script);
		const i32 res_draw_debug_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"DrawDebugWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_draw_debug_world_script);
		const i32 res_key_event_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"KeyEventWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_key_event_world_script);
		const i32 res_mouse_button_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"MouseButtonEventWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_mouse_button_event_world_script);
		const i32 res_mouse_move_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"MouseMoveEventWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_mouse_move_event_world_script);
		const i32 res_mouse_wheel_world_script	   = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"MouseWheelEventWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_mouse_wheel_event_world_script);
		const i32 res_physics_contact_world_script = fn_load_managed(assembly_path_wide.c_str(), managed_type, L"PhysicsContactWorldScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn_physics_contact_world_script);

		if (res_load_init != 0 || fn_init == nullptr || res_load_shutdown != 0 || fn_shtdown == nullptr || res_stage_project_assembly != 0 || fn_stage_project_assembly == nullptr || res_get_staged_project_schema != 0 ||
			fn_get_staged_project_schema == nullptr || res_activate_staged_schema != 0 || fn_activate_staged_project_assembly == nullptr || res_discard_staged_schema != 0 || fn_discard_staged_project_assembly == nullptr || res_create_world_script != 0 ||
			fn_create_world_script == nullptr || res_destroy_world_script != 0 || fn_destroy_world_script == nullptr || res_begin_play_world_script != 0 || fn_begin_play_world_script == nullptr || res_end_play_world_script != 0 ||
			fn_end_play_world_script == nullptr || res_tick_world_script != 0 || fn_tick_world_script == nullptr || res_post_tick_world_script != 0 || fn_post_tick_world_script == nullptr || res_post_physics_world_script != 0 ||
			fn_post_physics_tick_world_script == nullptr || res_post_animation_world_script != 0 || fn_post_animation_tick_world_script == nullptr || res_draw_debug_world_script != 0 || fn_draw_debug_world_script == nullptr ||
			res_key_event_world_script != 0 || fn_key_event_world_script == nullptr || res_mouse_button_world_script != 0 || fn_mouse_button_event_world_script == nullptr || res_mouse_move_world_script != 0 || fn_mouse_move_event_world_script == nullptr ||
			res_mouse_wheel_world_script != 0 || fn_mouse_wheel_event_world_script == nullptr || res_physics_contact_world_script != 0 || fn_physics_contact_world_script == nullptr)
		{
			SFG_ERR("could not load one or more managed scripting entry points.");
			unload_hostfxr(hostfxr_library);
			return false;
		}

		const script_host_initialize_fn						  initialize					   = reinterpret_cast<script_host_initialize_fn>(fn_init);
		const script_host_shutdown_fn						  shutdown						   = reinterpret_cast<script_host_shutdown_fn>(fn_shtdown);
		const script_host_stage_project_assembly_fn			  stage_project_assembly		   = reinterpret_cast<script_host_stage_project_assembly_fn>(fn_stage_project_assembly);
		const script_host_get_staged_project_schema_fn		  get_staged_project_schema		   = reinterpret_cast<script_host_get_staged_project_schema_fn>(fn_get_staged_project_schema);
		const script_host_activate_staged_project_assembly_fn activate_staged_project_assembly = reinterpret_cast<script_host_activate_staged_project_assembly_fn>(fn_activate_staged_project_assembly);
		const script_host_discard_staged_project_assembly_fn  discard_staged_project_assembly  = reinterpret_cast<script_host_discard_staged_project_assembly_fn>(fn_discard_staged_project_assembly);
		const script_host_create_world_script_fn			  create_world_script			   = reinterpret_cast<script_host_create_world_script_fn>(fn_create_world_script);
		const script_host_world_script_lifecycle_fn			  destroy_world_script			   = reinterpret_cast<script_host_world_script_lifecycle_fn>(fn_destroy_world_script);
		const script_host_world_script_lifecycle_fn			  begin_play_world_script		   = reinterpret_cast<script_host_world_script_lifecycle_fn>(fn_begin_play_world_script);
		const script_host_world_script_lifecycle_fn			  end_play_world_script			   = reinterpret_cast<script_host_world_script_lifecycle_fn>(fn_end_play_world_script);
		const script_host_world_script_tick_fn				  tick_world_script				   = reinterpret_cast<script_host_world_script_tick_fn>(fn_tick_world_script);
		const script_host_world_script_tick_fn				  post_tick_world_script		   = reinterpret_cast<script_host_world_script_tick_fn>(fn_post_tick_world_script);
		const script_host_world_script_tick_fn				  post_physics_tick_world_script   = reinterpret_cast<script_host_world_script_tick_fn>(fn_post_physics_tick_world_script);
		const script_host_world_script_tick_fn				  post_animation_tick_world_script = reinterpret_cast<script_host_world_script_tick_fn>(fn_post_animation_tick_world_script);
		const script_host_world_script_lifecycle_fn			  draw_debug_world_script		   = reinterpret_cast<script_host_world_script_lifecycle_fn>(fn_draw_debug_world_script);
		const script_host_world_script_key_event_fn			  key_event_world_script		   = reinterpret_cast<script_host_world_script_key_event_fn>(fn_key_event_world_script);
		const script_host_world_script_mouse_button_event_fn  mouse_button_event_world_script  = reinterpret_cast<script_host_world_script_mouse_button_event_fn>(fn_mouse_button_event_world_script);
		const script_host_world_script_mouse_move_event_fn	  mouse_move_event_world_script	   = reinterpret_cast<script_host_world_script_mouse_move_event_fn>(fn_mouse_move_event_world_script);
		const script_host_world_script_mouse_wheel_event_fn	  mouse_wheel_event_world_script   = reinterpret_cast<script_host_world_script_mouse_wheel_event_fn>(fn_mouse_wheel_event_world_script);
		const script_host_world_script_physics_contact_fn	  physics_contact_world_script	   = reinterpret_cast<script_host_world_script_physics_contact_fn>(fn_physics_contact_world_script);

		_native_api = {
			.size			= static_cast<u32>(sizeof(script_host_native_api_t)),
			.version		= 5,
			.log_info		= script_host_log_info,
			.log_error		= script_host_log_error,
			.platform		= &get_script_api_platform(),
			.game			= &get_script_api_game(),
			.resource		= &get_script_api_resource(),
			.world			= &get_script_api_world(),
			.audio			= &get_script_api_audio(),
			.physics		= &get_script_api_physics(),
			.animation		= &get_script_api_animation(),
			.game_log_info	= script_host_game_log_info,
			.game_log_error = script_host_game_log_error,
			.game_log_warn	= script_host_game_log_warn,
			.game_log_trace = script_host_game_log_trace,
		};

		const i32 res_managed_initialize = initialize(&_native_api);

		if (res_managed_initialize != 0)
		{
			SFG_ERR("managed scripting host initialization failed, error: {0}", res_managed_initialize);
			unload_hostfxr(hostfxr_library);
			return false;
		}

		_hostfxr_library					 = hostfxr_library;
		_shutdown							 = shutdown;
		_fn_stage_project_assembly			 = stage_project_assembly;
		_fn_get_staged_project_schema		 = get_staged_project_schema;
		_fn_activate_staged_project_schema	 = activate_staged_project_assembly;
		_fn_discard_staged_project_schema	 = discard_staged_project_assembly;
		_fn_create_world_script				 = create_world_script;
		_fn_destroy_world_script			 = destroy_world_script;
		_fn_begin_play_world_script			 = begin_play_world_script;
		_fn_end_play_world_script			 = end_play_world_script;
		_fn_tick_world_script				 = tick_world_script;
		_fn_post_tick_world_script			 = post_tick_world_script;
		_fn_post_physics_tick_world_script	 = post_physics_tick_world_script;
		_fn_post_animation_tick_world_script = post_animation_tick_world_script;
		_fn_draw_debug_world_script			 = draw_debug_world_script;
		_fn_key_event_world_script			 = key_event_world_script;
		_fn_mouse_button_event_world_script	 = mouse_button_event_world_script;
		_fn_mouse_move_event_world_script	 = mouse_move_event_world_script;
		_fn_mouse_wheel_event_world_script	 = mouse_wheel_event_world_script;
		_fn_physics_contact_world_script	 = physics_contact_world_script;
		_is_initialized						 = true;

		reflection_registry_t::get().reserve_script_capacity();

		return true;
#else
		SFG_ERR("managed scripting is not implemented on this platform.");
		return false;
#endif
	}

	void script_runtime_t::uninit()
	{

#ifdef SFG_PLATFORM_WINDOWS
		const i32 shutdown_result = _shutdown();

		if (shutdown_result != 0)
			SFG_ERR("managed scripting host shutdown failed, error: {0}", shutdown_result);

		unload_hostfxr(static_cast<HMODULE>(_hostfxr_library));
#endif

		reflection_registry_t::get().remove_script_types();

		_component_schema					 = {};
		_staged_component_schema			 = {};
		_native_api							 = {};
		_hostfxr_library					 = nullptr;
		_shutdown							 = nullptr;
		_fn_stage_project_assembly			 = nullptr;
		_fn_get_staged_project_schema		 = nullptr;
		_fn_activate_staged_project_schema	 = nullptr;
		_fn_discard_staged_project_schema	 = nullptr;
		_fn_create_world_script				 = nullptr;
		_fn_destroy_world_script			 = nullptr;
		_fn_begin_play_world_script			 = nullptr;
		_fn_end_play_world_script			 = nullptr;
		_fn_tick_world_script				 = nullptr;
		_fn_post_tick_world_script			 = nullptr;
		_fn_post_physics_tick_world_script	 = nullptr;
		_fn_post_animation_tick_world_script = nullptr;
		_fn_draw_debug_world_script			 = nullptr;
		_fn_key_event_world_script			 = nullptr;
		_fn_mouse_button_event_world_script	 = nullptr;
		_fn_mouse_move_event_world_script	 = nullptr;
		_fn_mouse_wheel_event_world_script	 = nullptr;
		_fn_physics_contact_world_script	 = nullptr;
		_is_initialized						 = false;
		_is_project_assembly_loaded			 = false;
		_is_project_assembly_staged			 = false;
	}

	bool script_runtime_t::stage_project_assembly(const char* assembly_path)
	{
		SFG_ASSERT(!_is_project_assembly_staged);

		const i32 stage_result = _fn_stage_project_assembly(assembly_path);

		if (stage_result != 0)
		{
			SFG_ERR("managed C# project assembly staging failed, error: {0}", stage_result);
			return false;
		}

		const i32 schema_size = _fn_get_staged_project_schema(nullptr, 0);

		if (schema_size <= 1)
		{
			SFG_ERR("managed C# project assembly returned an invalid component schema size: {0}", schema_size);
			_fn_discard_staged_project_schema();
			return false;
		}

		string_t  schema_json(static_cast<size_t>(schema_size), '\0');
		const i32 schema_result = _fn_get_staged_project_schema(schema_json.data(), static_cast<u32>(schema_json.size()));

		if (schema_result != schema_size)
		{
			SFG_ERR("managed C# project assembly schema transfer failed, error: {0}", schema_result);
			_fn_discard_staged_project_schema();
			return false;
		}

		script_component_schema_t candidate_schema = {};

		if (!candidate_schema.parse(schema_json.c_str()))
		{
			_fn_discard_staged_project_schema();
			return false;
		}

		if (!reflection_registry_t::get().is_script_capacity_valid(candidate_schema.get_components().size(), candidate_schema.get_field_count()))
		{
			SFG_ERR("managed component schema exceeds the script reflection capacity.");
			_fn_discard_staged_project_schema();
			return false;
		}

		_staged_component_schema	= std::move(candidate_schema);
		_is_project_assembly_staged = true;
		return true;
	}

	bool script_runtime_t::activate_staged_project_assembly()
	{
		SFG_ASSERT(_is_project_assembly_staged);

		const i32 result = _fn_activate_staged_project_schema();

		if (result != 0)
		{
			SFG_ERR("managed C# project assembly activation failed, error: {0}", result);
			return false;
		}

		_component_schema			= std::move(_staged_component_schema);
		_staged_component_schema	= {};
		_is_project_assembly_staged = false;
		_is_project_assembly_loaded = true;
		return true;
	}

	void script_runtime_t::discard_staged_project_assembly()
	{
		SFG_ASSERT(_is_project_assembly_staged);

		const i32 result = _fn_discard_staged_project_schema();

		if (result != 0)
			SFG_ERR("managed C# project assembly discard failed, error: {0}", result);

		_staged_component_schema	= {};
		_is_project_assembly_staged = false;
	}

	void* script_runtime_t::create_world_script(sid_t type_id, void* world)
	{
		SFG_ASSERT(_is_project_assembly_loaded);

		return _fn_create_world_script(type_id, world);
	}

	void script_runtime_t::destroy_world_script(void* instance)
	{
		SFG_ASSERT(instance != nullptr);

		const i32 result = _fn_destroy_world_script(instance);

		if (result != 0)
			SFG_ERR("managed C# world script destruction failed, error: {0}", result);
	}

	bool script_runtime_t::begin_play_world_script(void* instance)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_begin_play_world_script(instance) == 0;
	}

	bool script_runtime_t::end_play_world_script(void* instance)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_end_play_world_script(instance) == 0;
	}

	bool script_runtime_t::tick_world_script(void* instance, f32 delta_time)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_tick_world_script(instance, delta_time) == 0;
	}

	bool script_runtime_t::post_tick_world_script(void* instance, f32 delta_time)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_post_tick_world_script(instance, delta_time) == 0;
	}

	bool script_runtime_t::post_physics_tick_world_script(void* instance, f32 delta_time)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_post_physics_tick_world_script(instance, delta_time) == 0;
	}

	bool script_runtime_t::post_animation_tick_world_script(void* instance, f32 delta_time)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_post_animation_tick_world_script(instance, delta_time) == 0;
	}

	bool script_runtime_t::draw_debug_world_script(void* instance)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_draw_debug_world_script(instance) == 0;
	}

	bool script_runtime_t::key_event_world_script(void* instance, u16 key, u16 scan_code, u8 action)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_key_event_world_script(instance, key, scan_code, action) == 0;
	}

	bool script_runtime_t::mouse_button_event_world_script(void* instance, u8 button, u8 action, f32 position_x, f32 position_y)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_mouse_button_event_world_script(instance, button, action, position_x, position_y) == 0;
	}

	bool script_runtime_t::mouse_move_event_world_script(void* instance, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_mouse_move_event_world_script(instance, position_x, position_y, delta_x, delta_y) == 0;
	}

	bool script_runtime_t::mouse_wheel_event_world_script(void* instance, f32 position_x, f32 position_y, f32 delta)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_mouse_wheel_event_world_script(instance, position_x, position_y, delta) == 0;
	}

	bool script_runtime_t::physics_contact_world_script(void* instance, const physics_contact_event_t& contact)
	{
		SFG_ASSERT(instance != nullptr);

		return _fn_physics_contact_world_script(instance, &contact, static_cast<u8>(contact.type), contact.is_sensor ? 1 : 0) == 0;
	}

}
