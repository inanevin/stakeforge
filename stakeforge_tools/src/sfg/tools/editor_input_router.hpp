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

#include "tools/tools_exports.hpp"
#include "tools/common_editor.hpp"
#include "tools/world/editor_world_handle.hpp"
#include <sfg/core/math/vec2f.hpp>

namespace sfg
{
	struct editor_surface_t;
	struct window_event_t;
	struct window_runtime_t;
	class world_t;

	class SFG_TOOLS_API editor_input_router_t final
	{
	public:
		editor_input_router_t()										   = default;
		~editor_input_router_t()									   = default;
		editor_input_router_t(const editor_input_router_t&)			   = delete;
		editor_input_router_t& operator=(const editor_input_router_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void on_window_event(window_runtime_t& runtime, const window_event_t& ev);

		void release_game_input(surface_handle_t surface = {});
		void resume_game_cursor(editor_world_handle_t world, surface_handle_t surface);
		void update_game_cursor(editor_world_handle_t world, surface_handle_t surface);
		void reset_game_cursor(surface_handle_t surface = {});

	private:
		void release_game_key(surface_handle_t surface, const window_event_t& ev);
		void dispatch_game_event(editor_world_handle_t world_handle, surface_handle_t surface, window_runtime_t& runtime, const window_event_t& ev);
		void dispatch_editor_event(editor_surface_t& surface, const window_event_t& ev);
		void dispatch_ui_event(editor_surface_t& surface, const window_event_t& ev);
		bool handle_editor_shortcut(editor_surface_t& surface, const window_event_t& ev);
		bool handle_reserved_shortcut(editor_surface_t& surface, const window_event_t& ev);
		void handle_play_shortcut(editor_surface_t& surface, i64 now_us);

	private:
		u64					  _game_keys[4]			   = {};
		i64					  _last_play_shortcut_us   = -1;
		vec2f_t				  _game_mouse_position	   = vec2f_t::zero;
		editor_world_handle_t _game_world			   = {};
		surface_handle_t	  _game_surface			   = {};
		surface_handle_t	  _released_cursor_surface = {};
		u8					  _game_mouse_buttons	   = 0;
		bool				  _play_shortcut_down	   = false;
	};
}
