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
	enum class editor_main_toolbar_world_view_e : u8
	{
		invalid,
		final,
		gbuffer_albedo,
		gbuffer_orm,
		gbuffer_normal,
		gbuffer_emissive,
		lighting,
		post_process,
	};

	enum class editor_play_mode_e : u8
	{
		none,
		play,
		play_physics,
		play_paused,
		play_physics_paused,
	};

	class editor_global_toolbar_t final
	{
	public:
		inline static editor_global_toolbar_t& get()
		{
			static editor_global_toolbar_t s_instance;
			return s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------
		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		inline void set_world_view(editor_main_toolbar_world_view_e view)
		{
			_world_view = view;
		}

		inline editor_main_toolbar_world_view_e get_world_view() const
		{
			return _world_view;
		}

		inline void set_play_mode(editor_play_mode_e mode)
		{
			_play_mode = mode;
		}

		inline editor_play_mode_e get_play_mode() const
		{
			return _play_mode;
		}

		inline void set_do_step(bool do_step)
		{
			_do_step = do_step;
		}

		inline bool is_do_step() const
		{
			return _do_step;
		}

		inline bool is_inited() const
		{
			return _inited;
		}

	private:
		editor_global_toolbar_t()										   = default;
		~editor_global_toolbar_t()										   = default;
		editor_global_toolbar_t(const editor_global_toolbar_t&)			   = delete;
		editor_global_toolbar_t& operator=(const editor_global_toolbar_t&) = delete;

		editor_main_toolbar_world_view_e _world_view = editor_main_toolbar_world_view_e::final;
		editor_play_mode_e				 _play_mode	 = editor_play_mode_e::none;
		bool							 _inited	 = false;
		bool							 _do_step	 = false;
	};
}
