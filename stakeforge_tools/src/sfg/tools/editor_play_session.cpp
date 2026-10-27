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

#include "editor_play_session.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
#include "world/editor_world.hpp"

#include <sfg/core/common/hashing.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/runtime/engine/engine_threads.hpp>
#include <sfg/core/runtime/resources/world_cook.hpp>
#include <sfg/core/runtime/scripting/api/script_api_platform.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_play_session_t::request_play(bool playing)
	{
		if (!playing)
		{
			_requested_state = editor_play_state_e::not_playing;
			_pause_requested = false;
			_step_requested	 = false;
			return;
		}

		_requested_state = _edit_mode ? editor_play_state_e::playing_editor_controlled : editor_play_state_e::playing_game_controlled;
	}

	void editor_play_session_t::set_edit_mode(bool enabled)
	{
		_edit_mode = enabled;

		if (is_play_requested())
			_requested_state = enabled ? editor_play_state_e::playing_editor_controlled : editor_play_state_e::playing_game_controlled;
	}

	void editor_play_session_t::request_pause(bool paused)
	{
		SFG_ASSERT(is_playing());

		_pause_requested = paused;

		if (!paused)
			_step_requested = false;
	}

	void editor_play_session_t::request_step()
	{
		SFG_ASSERT(is_playing() && _paused);

		_step_requested = true;
	}

	void editor_play_session_t::apply_requests()
	{
		if (_requested_state == editor_play_state_e::not_playing)
		{
			stop();
			return;
		}

		if (!is_playing())
			enter_play(_requested_state);
		else if (_state != _requested_state)
			change_controls(_requested_state);

		if (_paused != _pause_requested)
			change_pause(_pause_requested);
	}

	void editor_play_session_t::enter_play(editor_play_state_e state)
	{
		editor_world_controller_t& worlds = editor_world_controller_t::get();

		_world = worlds.get_main_world_handle();
		SFG_ASSERT(!_world.is_null());

		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		editor_world_t& editor_world = *worlds.get_editor_world(_world);

		editor_world.reset_input();
		capture_snapshot(editor_world);
		_saved_world_dirty = worlds.is_main_world_dirty();

		editor_world.set_editor_control_enabled(state == editor_play_state_e::playing_editor_controlled);

		_edits.begin(_world);
		_state = state;
		editor_world.get_world().begin_play();

		if (state == editor_play_state_e::playing_game_controlled)
			editor_surface_controller_t::get().request_focus_main_world_view();
	}

	void editor_play_session_t::stop()
	{
		_requested_state		  = editor_play_state_e::not_playing;
		_pause_requested		  = false;
		_step_requested			  = false;
		_game_request			  = game_request_e::none;
		_pending_world_asset_guid = NULL_SID;

		if (!is_playing())
			return;

		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		editor_world_controller_t& worlds		= editor_world_controller_t::get();
		editor_world_t&			   editor_world = *worlds.get_editor_world(_world);

		editor_surface_controller_t::get().get_input_router().release_game_input();
		editor_world.reset_input();

		_edits.end();

		_state	= editor_play_state_e::not_playing;
		_paused = false;
		editor_world.get_world().end_play();

		const bool kept_edits = restore_snapshot(editor_world);

		editor_world.produce_snapshot();
		reset_script_api_platform_cursor_state();
		editor_surface_controller_t::get().get_input_router().reset_game_cursor();

		worlds.set_main_world_dirty(_saved_world_dirty || kept_edits);
		_saved_world_dirty = false;
		_world			   = {};
		refresh_panels();
	}

	void editor_play_session_t::change_controls(editor_play_state_e state)
	{
		SFG_ASSERT(is_playing() && state != editor_play_state_e::not_playing);

		editor_app_t::get().stop_render();
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		editor_world_t& editor_world = *editor_world_controller_t::get().get_editor_world(_world);

		editor_surface_controller_t::get().get_input_router().release_game_input();
		editor_world.reset_input();

		editor_world.set_editor_control_enabled(state == editor_play_state_e::playing_editor_controlled);

		if (state == editor_play_state_e::playing_game_controlled)
			editor_surface_controller_t::get().request_focus_main_world_view();
		else
		{
			reset_script_api_platform_cursor_state();
			editor_surface_controller_t::get().get_input_router().reset_game_cursor();
		}

		_state = state;
		editor_world.get_world().update_world_transforms();
		editor_world.get_world().tick_post(0.0f);
		editor_world.produce_snapshot();
	}

	void editor_play_session_t::change_pause(bool paused)
	{
		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();

		if (paused)
			world.pause_audio();
		else
			world.resume_audio();

		_paused = paused;
	}

	void editor_play_session_t::capture_snapshot(editor_world_t& editor_world)
	{
		nlohmann::json snapshot = nlohmann::json::object();

		world_cooker_t::world_to_json(editor_world.get_world(), snapshot);
		editor_world.serialize_camera(snapshot["editor_camera"]);

		_world_snapshot = snapshot.dump();
	}

	bool editor_play_session_t::restore_snapshot(editor_world_t& editor_world)
	{
		SFG_ASSERT(!_world_snapshot.empty());

		const nlohmann::json snapshot = nlohmann::json::parse(_world_snapshot, nullptr, false);
		SFG_ASSERT(!snapshot.is_discarded());

		nlohmann::json camera = snapshot.value<nlohmann::json>("editor_camera", nlohmann::json::object());

		editor_world.serialize_camera(camera);
		editor_world.get_edit_context().apply_entity_selection({}, NULL_ENTITY_ID);
		editor_command_system_t::get().clear_world(_world);

		// reset
		world_t& world = editor_world.get_world();

		world.reset_world_state();
		world_cooker_t::world_from_json(world, snapshot);

		const bool kept_edits = _save_edits && _edits.apply(world);
		_edits.clear();

		// cam
		editor_world.recreate_camera_entity();
		editor_world.deserialize_camera(camera);

		// reset
		world.update_world_transforms();
		_world_snapshot.resize(0);
		return kept_edits;
	}

	void editor_play_session_t::tick_world(editor_world_t& editor_world, f32 dt_seconds)
	{
		world_t& world = editor_world.get_world();

		if (!is_playing())
		{
			editor_world.tick_camera(dt_seconds);
			world.update_world_transforms_for_tick();

			world.tick_animation_logic(dt_seconds);
			world.tick_post(dt_seconds);
			return;
		}

		if (_paused)
		{
			world.tick_post(0.0f);
			return;
		}

		tick_simulation(editor_world, dt_seconds);
	}

	void editor_play_session_t::tick_simulation(editor_world_t& editor_world, f32 dt_seconds)
	{
		world_t& world = editor_world.get_world();

		if (_state == editor_play_state_e::playing_editor_controlled)
			editor_world.tick_camera(dt_seconds);

		world.tick_logic(dt_seconds);
		world.update_world_transforms_for_tick();
		world.tick_physics(dt_seconds);
		world.update_world_transforms();

		world.tick_animation_logic(dt_seconds);
		world.update_world_transforms();
		world.tick_post(dt_seconds);
	}

	void editor_play_session_t::step(f32 dt_seconds)
	{
		if (!_step_requested)
			return;

		SFG_ASSERT(is_playing() && _paused);
		_step_requested = false;

		editor_world_t& editor_world = *editor_world_controller_t::get().get_editor_world(_world);

		tick_simulation(editor_world, dt_seconds);
	}

	bool editor_play_session_t::queue_world_load(sid_t world_name_hash)
	{
		if (!is_playing() || world_name_hash == 0 || world_name_hash == NULL_SID || _game_request != game_request_e::none)
			return false;

		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		sid_t						  asset_guid	= NULL_SID;

		for (const auto& [candidate_guid, asset] : asset_manager.get_assets())
		{
			if (asset.asset_type != editor_asset_type_e::world)
				continue;

			const editor_asset_node_t* node = asset_manager.find_asset_node(candidate_guid);
			SFG_ASSERT(node != nullptr);

			if (TO_SID(node->name) != world_name_hash)
				continue;

			if (asset_guid != NULL_SID)
			{
				SFG_ERR("multiple world assets have the requested name hash {0}", world_name_hash);
				return false;
			}

			asset_guid = candidate_guid;
		}

		if (asset_guid == NULL_SID)
			return false;

		_pending_world_asset_guid = asset_guid;
		_game_request			  = game_request_e::load_world;
		return true;
	}

	bool editor_play_session_t::queue_world_restart()
	{
		if (!is_playing() || _game_request != game_request_e::none)
			return false;

		_game_request = game_request_e::restart;
		return true;
	}

	void editor_play_session_t::queue_quit()
	{
		if (!is_playing())
			return;

		_game_request			  = game_request_e::quit;
		_pending_world_asset_guid = NULL_SID;
	}

	void editor_play_session_t::process_game_requests()
	{
		const game_request_e request = _game_request;

		if (request == game_request_e::none)
			return;

		const sid_t asset_guid = _pending_world_asset_guid;

		stop();

		if (request == game_request_e::quit)
			return;

		if (request == game_request_e::load_world && !editor_world_controller_t::get().load_main_world_now(asset_guid))
			return;

		request_play(true);
		apply_requests();
	}

	void editor_play_session_t::refresh_panels()
	{
		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::entities))
			static_cast<editor_panel_entities_t*>(panel)->refresh_entities();

		if (editor_panel_t* panel = surfaces.find_panel(editor_panel_type_e::inspector))
			static_cast<editor_panel_inspector_t*>(panel)->refresh_from_selection();
	}
}
