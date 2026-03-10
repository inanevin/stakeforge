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

#include "common/size_definitions.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "data/atomic.hpp"
#include "data/bitmask.hpp"
#include "math/vector2ui16.hpp"
#include <thread>

namespace SFG
{
	struct window_event;
	class window;
	class world;
	class renderer;
	class game;
	class editor;
	class gameplay;

	class app
	{
	public:
		enum class game_app_flags
		{
			should_exit = 1 << 0,
		};

		enum class init_status
		{
			ok,
			working_directory_dont_exist,
			renderer_failed,
			window_failed,
			backend_failed,
			engine_resources_failed,
			packages_failed,
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		init_status init(const vector2ui16& render_target_size);
		void		uninit();
		void		tick();
		void		join_render();
		void		kick_off_render();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bool get_should_exit() const
		{
			return _flags.is_set(static_cast<u8>(game_app_flags::should_exit));
		}

		inline window& get_main_window()
		{
			return *_main_window;
		}

		inline renderer& get_renderer() const
		{
			return *_renderer;
		}

		inline world& get_world() const
		{
			return *_world;
		}

		const vector2ui16& get_game_resolution() const;

		void set_game_resolution(const vector2ui16& size);

		inline gameplay& get_gameplay() const
		{
			return *_gameplay;
		}

#ifdef SFG_TOOLMODE
		inline editor* get_editor() const
		{
			return _editor;
		}
#else
		inline game& get_game() const
		{
			return *_game;
		}
#endif

	private:
		void		render_loop();
		static void on_window_event(const window_event& ev, void* user_data);

	private:
		window*	  _main_window = nullptr;
		renderer* _renderer	   = nullptr;
		world*	  _world	   = nullptr;
		gameplay* _gameplay	   = nullptr;

#ifdef SFG_TOOLMODE
		editor* _editor = nullptr;
#else
		game* _game = nullptr;
#endif

		render_event_stream _render_stream;
		std::thread			_render_thread;
		atomic<u8>			_should_close;
		atomic<u8>			_render_joined;
		bitmask<u8>			_flags = 0;
	};
}
