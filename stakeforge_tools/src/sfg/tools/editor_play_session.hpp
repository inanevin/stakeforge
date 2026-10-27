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
#include "tools/world/editor_world_handle.hpp"
#include "editor_play_edits.hpp"
#include <sfg/core/data/string.hpp>

namespace sfg
{
	class editor_world_t;

	enum class editor_play_state_e : u8
	{
		not_playing,
		playing_editor_controlled,
		playing_game_controlled,
	};

	class SFG_TOOLS_API editor_play_session_t final
	{
	public:
		editor_play_session_t()										   = default;
		~editor_play_session_t()									   = default;
		editor_play_session_t(const editor_play_session_t&)			   = delete;
		editor_play_session_t& operator=(const editor_play_session_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void stop();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_play(bool playing);
		void set_edit_mode(bool enabled);
		void request_pause(bool paused);
		void request_step();
		void apply_requests();
		void tick_world(editor_world_t& editor_world, f32 dt_seconds);
		void step(f32 dt_seconds);
		bool queue_world_load(sid_t world_name_hash);
		bool queue_world_restart();
		void queue_quit();
		void process_game_requests();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_play_state_e get_state() const
		{
			return _state;
		}

		inline editor_play_state_e get_requested_state() const
		{
			return _requested_state;
		}

		inline bool is_playing() const
		{
			return _state != editor_play_state_e::not_playing;
		}

		inline bool is_play_requested() const
		{
			return _requested_state != editor_play_state_e::not_playing;
		}

		inline bool is_edit_mode_enabled() const
		{
			return _edit_mode;
		}

		inline editor_play_edits_t& get_edits()
		{
			return _edits;
		}

		inline void set_save_edits(bool enabled)
		{
			_save_edits = enabled;
		}

		inline bool is_save_edits_enabled() const
		{
			return _save_edits;
		}

		inline bool is_paused() const
		{
			return _paused;
		}

	private:
		enum class game_request_e : u8
		{
			none,
			load_world,
			restart,
			quit,
		};

		void enter_play(editor_play_state_e state);
		void change_controls(editor_play_state_e state);
		void change_pause(bool paused);
		void capture_snapshot(editor_world_t& editor_world);
		bool restore_snapshot(editor_world_t& editor_world);
		void tick_simulation(editor_world_t& editor_world, f32 dt_seconds);
		void refresh_panels();

	private:
		editor_play_edits_t	  _edits					= {};
		string_t			  _world_snapshot			= {};
		sid_t				  _pending_world_asset_guid = NULL_SID;
		editor_world_handle_t _world					= {};
		editor_play_state_e	  _state					= editor_play_state_e::not_playing;
		editor_play_state_e	  _requested_state			= editor_play_state_e::not_playing;
		game_request_e		  _game_request				= game_request_e::none;
		bool				  _edit_mode				= false;
		bool				  _save_edits				= false;
		bool				  _saved_world_dirty		= false;
		bool				  _paused					= false;
		bool				  _pause_requested			= false;
		bool				  _step_requested			= false;
	};
}
