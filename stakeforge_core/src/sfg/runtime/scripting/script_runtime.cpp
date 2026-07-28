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
#include <sfg/runtime/scripting/api/script_api_animation.hpp>
#include <sfg/runtime/scripting/api/script_api_audio.hpp>
#include <sfg/runtime/scripting/api/script_api_physics.hpp>
#include <sfg/runtime/scripting/api/script_api_platform.hpp>
#include <sfg/runtime/scripting/api/script_api_render.hpp>
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
		const load_assembly_and_get_function_pointer_fn load_managed_function = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(load_assembly_delegate);
		const char_t* const								managed_type		  = L"Stakeforge.ScriptHost.NativeEntryPoints, Stakeforge.ScriptHost";
		void*											initialize_function	  = nullptr;
		void*											shutdown_function	  = nullptr;

		const i32 load_initialize_result = load_managed_function(assembly_path_wide.c_str(), managed_type, L"Initialize", UNMANAGEDCALLERSONLY_METHOD, nullptr, &initialize_function);
		const i32 load_shutdown_result	 = load_managed_function(assembly_path_wide.c_str(), managed_type, L"Shutdown", UNMANAGEDCALLERSONLY_METHOD, nullptr, &shutdown_function);

		if (load_initialize_result != 0 || initialize_function == nullptr || load_shutdown_result != 0 || shutdown_function == nullptr)
		{
			SFG_ERR("could not load managed entry points, errors: {0}, {1}", load_initialize_result, load_shutdown_result);
			unload_hostfxr(hostfxr_library);
			return false;
		}

		const script_host_initialize_fn initialize = reinterpret_cast<script_host_initialize_fn>(initialize_function);
		const script_host_shutdown_fn	shutdown   = reinterpret_cast<script_host_shutdown_fn>(shutdown_function);

		_native_api = {
			.size	   = static_cast<u32>(sizeof(script_host_native_api_t)),
			.version   = 3,
			.log_info  = script_host_log_info,
			.log_error = script_host_log_error,
			.platform  = &get_script_api_platform(),
			.render	   = &get_script_api_render(),
			.resource  = &get_script_api_resource(),
			.world	   = &get_script_api_world(),
			.audio	   = &get_script_api_audio(),
			.physics   = &get_script_api_physics(),
			.animation = &get_script_api_animation(),
		};

		const i32 managed_initialize_result = initialize(&_native_api);

		if (managed_initialize_result != 0)
		{
			SFG_ERR("managed scripting host initialization failed, error: {0}", managed_initialize_result);
			unload_hostfxr(hostfxr_library);
			return false;
		}

		_hostfxr_library = hostfxr_library;
		_shutdown		 = shutdown;
		_is_initialized	 = true;

		return true;
#else
		SFG_ERR("managed scripting is not implemented on this platform.");
		return false;
#endif
	}

	void script_runtime_t::uninit()
	{
		SFG_ASSERT(_is_initialized);

#ifdef SFG_PLATFORM_WINDOWS
		const i32 shutdown_result = _shutdown();

		if (shutdown_result != 0)
			SFG_ERR("managed scripting host shutdown failed, error: {0}", shutdown_result);

		unload_hostfxr(static_cast<HMODULE>(_hostfxr_library));
#endif

		_native_api		 = {};
		_hostfxr_library = nullptr;
		_shutdown		 = nullptr;
		_is_initialized	 = false;
	}
}
