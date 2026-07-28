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

#include <sfg/common/size_definitions.hpp>
namespace sfg
{
	struct script_api_animation_t;
	struct script_api_audio_t;
	struct script_api_physics_t;
	struct script_api_platform_t;
	struct script_api_render_t;
	struct script_api_resource_t;
	struct script_api_world_t;

	typedef void (*script_host_log_fn)(const char* message);

	struct script_host_native_api_t
	{
		u32							  size		= 0;
		u32							  version	= 0;
		script_host_log_fn			  log_info	= nullptr;
		script_host_log_fn			  log_error = nullptr;
		const script_api_platform_t*  platform	= nullptr;
		const script_api_render_t*	  render	= nullptr;
		const script_api_resource_t*  resource	= nullptr;
		const script_api_world_t*	  world		= nullptr;
		const script_api_audio_t*	  audio		= nullptr;
		const script_api_physics_t*	  physics	= nullptr;
		const script_api_animation_t* animation = nullptr;
	};

	typedef i32 (*script_host_shutdown_fn)();
	typedef i32 (*script_host_load_project_assembly_fn)(const char* assembly_path);
	typedef i32 (*script_host_unload_project_assembly_fn)();

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
		bool load_project_assembly(const char* assembly_path);
		bool reload_project_assembly(const char* assembly_path);
		bool unload_project_assembly();

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

	private:
		script_host_native_api_t			   _native_api				   = {};
		void*								   _hostfxr_library			   = nullptr;
		script_host_shutdown_fn				   _shutdown				   = nullptr;
		script_host_load_project_assembly_fn   _load_project_assembly	   = nullptr;
		script_host_unload_project_assembly_fn _unload_project_assembly	   = nullptr;
		bool								   _is_initialized			   = false;
		bool								   _is_project_assembly_loaded = false;
	};
}
