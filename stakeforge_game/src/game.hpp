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

#include "game_renderer.hpp"
#include "game_world_controller.hpp"

#include <sfg/data/string.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/engine/engine_runtime_config.hpp>
#include <sfg/runtime/project/project_package_meta.hpp>

namespace sfg
{
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

		bool load_package_resources();
		bool fail_init(const char* reason);
		void cleanup();

		game_config_t			_config				 = {};
		project_package_meta_t	_package_meta		 = {};
		window_runtime_t		_window				 = {};
		string_t				_init_failure_reason = {};
		game_world_controller_t _world_controller;
		game_renderer_t			_renderer;
		u8						_atlas_frame_slot			  = 0;
		bool					_renderer_initialized		  = false;
		bool					_world_controller_initialized = false;
		bool					_window_initialized			  = false;
		bool					_frame_allocator_initialized  = false;
		bool					_runtime_initialized		  = false;
		bool					_backend_initialized		  = false;
		bool					_globals_initialized		  = false;
		bool					_initialized				  = false;
	};
}
