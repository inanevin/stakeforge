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

#include "editor_input_router.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/editor_panel_world.hpp"
#include "world/editor_world.hpp"

#include <sfg/core/input/input_mappings.hpp>
#include <sfg/core/platform/common_window.hpp>
#include <sfg/core/platform/process.hpp>
#include <sfg/core/platform/time.hpp>
#include <sfg/core/runtime/ui/ui_context.hpp>

namespace sfg
{
#define EDITOR_RAW_WHEEL_DELTA				 120.0f
#define EDITOR_PLAY_SHORTCUT_DOUBLE_PRESS_US 400000

	void editor_input_router_t::on_window_event(window_runtime_t& runtime, const window_event_t& ev)
	{
		editor_surface_controller_t& surfaces		= editor_surface_controller_t::get();
		const surface_handle_t		 surface_handle = surfaces.get_surface_handle_by_runtime(runtime);
		editor_surface_t&			 surface		= surfaces.get_surface(surface_handle);

		// dont bother with anything else during these
		if (surface.type == editor_surface_type_e::splash || surface.type == editor_surface_type_e::project_creator)
		{
			dispatch_ui_event(surface, ev);
			return;
		}

		if (!editor_world_controller_t::is_initialized())
			return;

		editor_world_controller_t& worlds		   = editor_world_controller_t::get();
		const bool				   game_controlled = worlds.get_play_session().get_state() == editor_play_state_e::playing_game_controlled;
		ui::ui_context&			   ui			   = *surface.ui;
		const bool				   blocked		   = surfaces.is_any_modal_active() || ui.get_input().is_popup_scope_active();

		if (!_game_world.is_null() && (!worlds.get_play_session().is_playing() || blocked || _game_world != worlds.get_main_world_handle()))
			release_game_input();

		if (ev.type == window_event_type_e::focus)
		{
			if (ev.sub_type == window_event_sub_type_e::release)
			{
				_play_shortcut_down = false;

				if (_game_surface == surface_handle)
					release_game_input();
			}

			editor_widget_world_view_t::on_window_event(runtime, ev);
			return;
		}

		if (!runtime.has_flag(window_runtime_flags_e::has_focus))
			return;

		if (handle_reserved_shortcut(surface, ev))
			return;

		const bool pointer_event = ev.type == window_event_type_e::mouse || ev.type == window_event_type_e::delta || ev.type == window_event_type_e::wheel;

		if (game_controlled && !blocked && (pointer_event || ev.type == window_event_type_e::key))
		{
			editor_panel_world_t* const panel = static_cast<editor_panel_world_t*>(surfaces.find_panel_on_surface(editor_panel_type_e::world, surface_handle));

			if (panel != nullptr)
			{
				const ui::widget_id_t view		  = panel->get_world_view().get_view_widget();
				const vec2f_t		  position	  = {static_cast<f32>(runtime.mouse_position.x), static_cast<f32>(runtime.mouse_position.y)};
				const vec4f_t		  bounds	  = ui.get_tree().bounds(view);
				const bool			  inside_view = position.x >= bounds.x && position.x < bounds.x + bounds.z && position.y >= bounds.y && position.y < bounds.y + bounds.w;
				const bool			  captured	  = runtime.cursor_confinement != window_cursor_confinement_e::none || (_game_surface == surface_handle && _game_mouse_buttons != 0);

				if (ev.type == window_event_type_e::mouse && ev.sub_type == window_event_sub_type_e::press && inside_view)
					ui.get_input().set_focus(view, false);

				if (ui.get_input().get_focused() == view && (!pointer_event || inside_view || captured))
				{
					const editor_world_handle_t handle = worlds.get_main_world_handle();
					SFG_ASSERT(!handle.is_null());

					dispatch_game_event(handle, surface_handle, runtime, ev);
					return;
				}
			}
		}

		if (game_controlled && _game_surface == surface_handle)
			release_game_input();

		dispatch_editor_event(surface, ev);

		if (_game_surface == surface_handle)
		{
			editor_panel_world_t* panel = static_cast<editor_panel_world_t*>(surfaces.find_panel_on_surface(editor_panel_type_e::world, surface_handle));

			if (panel == nullptr || ui.get_input().get_focused() != panel->get_world_view().get_view_widget())
				release_game_input();
		}
	}

	void editor_input_router_t::release_game_input(surface_handle_t surface)
	{
		if (_game_world.is_null())
			return;

		if (!surface.is_null() && _game_surface != surface)
			return;

		editor_world_controller_t& worlds = editor_world_controller_t::get();

		if (worlds.is_world_valid(_game_world))
		{
			world_t& world = worlds.get_editor_world(_game_world)->get_world();

			if (world.is_playing())
			{
				for (u8 button = 0; button < 5; ++button)
				{
					if ((_game_mouse_buttons & (1u << button)) != 0)
						world.mouse_button_event(button, static_cast<u8>(window_event_sub_type_e::release), _game_mouse_position.x, _game_mouse_position.y);
				}

				world.focus_event(false);
			}
		}

		for (u64& keys : _game_keys)
			keys = 0;

		_game_world			 = {};
		_game_surface		 = {};
		_game_mouse_buttons	 = 0;
		_game_mouse_position = vec2f_t::zero;
	}

	void editor_input_router_t::release_game_key(surface_handle_t surface, const window_event_t& ev)
	{
		if (_game_surface != surface || ev.button >= 256 || (_game_keys[ev.button / 64] & (1ull << (ev.button % 64))) == 0)
			return;

		window_event_t release	  = ev;
		release.sub_type		  = window_event_sub_type_e::release;
		window_runtime_t& runtime = *editor_surface_controller_t::get().get_surface(surface).runtime;

		dispatch_game_event(_game_world, surface, runtime, release);
	}

	bool editor_input_router_t::handle_reserved_shortcut(editor_surface_t& surface, const window_event_t& ev)
	{
		if (ev.type == window_event_type_e::key && ev.button == static_cast<u16>(input_code::key_f1))
		{
			const bool ctrl = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

			if (_play_shortcut_down || ctrl)
			{
				if (ev.sub_type == window_event_sub_type_e::release)
					_play_shortcut_down = false;
				else if (ev.sub_type == window_event_sub_type_e::press && !_play_shortcut_down)
				{
					_play_shortcut_down = true;
					handle_play_shortcut(surface, time_t::get_cpu_microseconds());
				}

				return true;
			}
		}

		if (ev.type != window_event_type_e::key || ev.button != static_cast<u16>(input_code::key_f3))
			return false;

		if (ev.sub_type == window_event_sub_type_e::press)
		{
			editor_app_t& app = editor_app_t::get();
			app.set_debug_mode(!app.is_debug_mode_enabled());
		}

		return true;
	}

	void editor_input_router_t::handle_play_shortcut(editor_surface_t& surface, i64 now_us)
	{
		editor_world_controller_t& worlds  = editor_world_controller_t::get();
		editor_play_session_t&	   session = worlds.get_play_session();

		if (!session.is_playing())
			return;

		if (_last_play_shortcut_us >= 0 && now_us - _last_play_shortcut_us <= EDITOR_PLAY_SHORTCUT_DOUBLE_PRESS_US)
		{
			_last_play_shortcut_us = -1;
			session.request_play(false);
			return;
		}

		_last_play_shortcut_us = now_us;

		editor_surface_controller_t& surfaces		= editor_surface_controller_t::get();
		const surface_handle_t		 surface_handle = surfaces.get_surface_handle_by_runtime(*surface.runtime);

		if (surfaces.is_editor_camera_cursor_captured(surface_handle) || (surface.runtime->cursor_confinement == window_cursor_confinement_e::none && surface.runtime->cursor_visible))
			return;

		editor_panel_world_t* const panel = static_cast<editor_panel_world_t*>(surfaces.find_panel_on_surface(editor_panel_type_e::world, surface_handle));

		if (panel == nullptr || panel->get_world_view().get_edit_world() != worlds.get_main_world_handle())
			return;

		world_screen_t& screen = worlds.get_editor_world(worlds.get_main_world_handle())->get_world().get_screen();

		if (screen.get_cursor_confinement() == window_cursor_confinement_e::none && screen.is_cursor_visible())
			return;

		_released_cursor_surface = surface_handle;
		screen.apply_cursor(*surface.runtime, false);
		surface.ui->get_input().set_focus(NULL_WIDGET, false);
		release_game_input(surface_handle);
	}

	void editor_input_router_t::resume_game_cursor(editor_world_handle_t world, surface_handle_t surface)
	{
		if (_released_cursor_surface != surface)
			return;

		editor_world_controller_t& worlds = editor_world_controller_t::get();

		if (!worlds.get_play_session().is_playing() || world != worlds.get_main_world_handle())
			return;

		_released_cursor_surface = {};
		update_game_cursor(world, surface);
	}

	void editor_input_router_t::update_game_cursor(editor_world_handle_t world, surface_handle_t surface)
	{
		editor_world_controller_t& worlds = editor_world_controller_t::get();

		if (worlds.get_play_session().get_state() != editor_play_state_e::playing_game_controlled || world != worlds.get_main_world_handle())
			return;

		window_runtime_t& runtime = *editor_surface_controller_t::get().get_surface(surface).runtime;
		worlds.get_editor_world(world)->get_world().get_screen().apply_cursor(runtime, _released_cursor_surface != surface);
	}

	void editor_input_router_t::reset_game_cursor(surface_handle_t surface_handle)
	{
		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();

		for (editor_surface_t& surface : surfaces)
		{
			const surface_handle_t handle = surfaces.get_surface_handle_by_runtime(*surface.runtime);

			if ((!surface_handle.is_null() && handle != surface_handle) || surfaces.is_editor_camera_cursor_captured(handle))
				continue;

			process::set_cursor_confinement(surface.runtime->window_handle, window_cursor_confinement_e::none);
			process::set_cursor_visible(surface.runtime->window_handle, true);
		}

		if (surface_handle.is_null() || surface_handle == _released_cursor_surface)
			_released_cursor_surface = {};

		if (surface_handle.is_null())
			_last_play_shortcut_us = -1;
	}

	void editor_input_router_t::dispatch_game_event(editor_world_handle_t world_handle, surface_handle_t surface, window_runtime_t& runtime, const window_event_t& ev)
	{
		world_t& world = editor_world_controller_t::get().get_editor_world(world_handle)->get_world();

		if (_game_world != world_handle || _game_surface != surface)
		{
			release_game_input();
			_game_world	  = world_handle;
			_game_surface = surface;
			world.focus_event(true);
		}

		_game_mouse_position = {static_cast<f32>(runtime.mouse_position.x), static_cast<f32>(runtime.mouse_position.y)};

		switch (ev.type)
		{
		case window_event_type_e::key:
			if (ev.button < 256)
			{
				const u64 key_mask = 1ull << (ev.button % 64);

				if (ev.sub_type == window_event_sub_type_e::press)
					_game_keys[ev.button / 64] |= key_mask;
				else if (ev.sub_type == window_event_sub_type_e::release)
					_game_keys[ev.button / 64] &= ~key_mask;
			}

			world.key_event(ev.button, static_cast<u16>(ev.value.x), static_cast<u8>(ev.sub_type));
			break;
		case window_event_type_e::delta:
			world.mouse_move_event(_game_mouse_position.x, _game_mouse_position.y, static_cast<f32>(ev.value.x), static_cast<f32>(ev.value.y));
			break;
		case window_event_type_e::mouse: {
			u8 button = UINT8_MAX;

			switch (static_cast<input_code>(ev.button))
			{
			case input_code::mouse_0:
				button = 0;
				break;
			case input_code::mouse_1:
				button = 1;
				break;
			case input_code::mouse_2:
				button = 2;
				break;
			case input_code::mouse_3:
				button = 3;
				break;
			case input_code::mouse_4:
				button = 4;
				break;
			default:
				return;
			}

			if (ev.sub_type == window_event_sub_type_e::press)
				_game_mouse_buttons |= static_cast<u8>(1u << button);
			else if (ev.sub_type == window_event_sub_type_e::release)
				_game_mouse_buttons &= static_cast<u8>(~(1u << button));

			world.mouse_button_event(button, static_cast<u8>(ev.sub_type), _game_mouse_position.x, _game_mouse_position.y);
			break;
		}
		case window_event_type_e::wheel: {
			const f32 delta = ev.flags.is_set(static_cast<u8>(wef_high_freq)) ? static_cast<f32>(ev.value.y) / EDITOR_RAW_WHEEL_DELTA : static_cast<f32>(ev.value.y);
			world.mouse_wheel_event(_game_mouse_position.x, _game_mouse_position.y, delta);
			break;
		}
		default:
			break;
		}
	}

	void editor_input_router_t::dispatch_editor_event(editor_surface_t& surface, const window_event_t& ev)
	{
		window_runtime_t&			 runtime		= *surface.runtime;
		editor_surface_controller_t& surfaces		= editor_surface_controller_t::get();
		const surface_handle_t		 surface_handle = surfaces.get_surface_handle_by_runtime(runtime);
		const bool					 blocked		= surfaces.is_any_modal_active() || surface.ui->get_input().is_popup_scope_active();

		if (blocked)
			editor_widget_world_view_t::reset_camera_input(runtime);
		else if (editor_widget_world_view_t::on_window_event(runtime, ev))
		{
			if (ev.type == window_event_type_e::key)
				release_game_key(surface_handle, ev);

			return;
		}

		if (!blocked && handle_editor_shortcut(surface, ev))
		{
			release_game_key(surface_handle, ev);
			return;
		}

		editor_world_controller_t& worlds			= editor_world_controller_t::get();
		const bool				   edit_play		= worlds.get_play_session().get_state() == editor_play_state_e::playing_editor_controlled;
		const bool				   focus_navigation = ev.type == window_event_type_e::key && (ev.button == static_cast<u16>(input_code::key_tab) || ev.button == static_cast<u16>(input_code::key_up) || ev.button == static_cast<u16>(input_code::key_down));

		if (edit_play && !blocked && ev.type == window_event_type_e::key && !focus_navigation)
		{
			editor_panel_world_t* panel = static_cast<editor_panel_world_t*>(surfaces.find_panel_on_surface(editor_panel_type_e::world, surface_handle));

			if (panel != nullptr && surface.ui->get_input().get_focused() == panel->get_world_view().get_view_widget())
			{
				if (ev.sub_type == window_event_sub_type_e::press && panel->get_world_view().key_press(ev.button))
				{
					release_game_key(surface_handle, ev);
					return;
				}

				const bool game_key_held = ev.button < 256 && (_game_keys[ev.button / 64] & (1ull << (ev.button % 64))) != 0;

				if (ev.sub_type == window_event_sub_type_e::press || game_key_held)
					dispatch_game_event(worlds.get_main_world_handle(), surface_handle, runtime, ev);

				return;
			}
		}

		dispatch_ui_event(surface, ev);
	}

	bool editor_input_router_t::handle_editor_shortcut(editor_surface_t& surface, const window_event_t& ev)
	{
		if (ev.type != window_event_type_e::key)
			return false;

		const bool					ctrl	   = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
		editor_world_controller_t&	worlds	   = editor_world_controller_t::get();
		const editor_world_handle_t main_world = worlds.get_main_world_handle();

		if (ctrl && ev.button == static_cast<u16>(input_code::key_s) && ev.sub_type == window_event_sub_type_e::press && !main_world.is_null() && !worlds.get_play_session().is_playing() && !worlds.get_play_session().is_play_requested())
		{
			worlds.save_main_world();
			return true;
		}

		if (ctrl && (ev.button == static_cast<u16>(input_code::key_z) || ev.button == static_cast<u16>(input_code::key_r)) && (ev.sub_type == window_event_sub_type_e::press || ev.sub_type == window_event_sub_type_e::repeat))
			surface.ui->get_input().set_focus(NULL_WIDGET, false);

		return editor_command_system_t::get().on_window_event(ev);
	}

	void editor_input_router_t::dispatch_ui_event(editor_surface_t& surface, const window_event_t& ev)
	{
		window_runtime_t& runtime = *surface.runtime;
		ui::ui_context&	  ui	  = *surface.ui;

		switch (ev.type)
		{
		case window_event_type_e::delta:
		case window_event_type_e::mouse:
			ui.on_mouse_move({static_cast<f32>(runtime.mouse_position.x), static_cast<f32>(runtime.mouse_position.y)});

			if (ev.type == window_event_type_e::mouse)
			{
				if (ev.sub_type == window_event_sub_type_e::press)
					ui.on_mouse_button(ui::input_router_t::map_button(ev.button), true);
				else if (ev.sub_type == window_event_sub_type_e::release)
					ui.on_mouse_button(ui::input_router_t::map_button(ev.button), false);
			}

			break;
		case window_event_type_e::wheel: {
			const f32 delta = ev.flags.is_set(static_cast<u8>(wef_high_freq)) ? static_cast<f32>(ev.value.y) / EDITOR_RAW_WHEEL_DELTA : static_cast<f32>(ev.value.y);
			ui.on_wheel(delta);
			break;
		}
		case window_event_type_e::key: {
			if (!runtime.has_flag(window_runtime_flags_e::has_focus))
				return;

			const ui::key_event_t key{
				.key	   = ev.button,
				.scan_code = static_cast<u16>(ev.value.x),
				.action	   = ev.sub_type == window_event_sub_type_e::press ? ui::key_action_e::press : (ev.sub_type == window_event_sub_type_e::release ? ui::key_action_e::release : ui::key_action_e::repeat),
				.shift	   = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift)),
			};
			ui.on_key(key);
			break;
		}
		default:
			break;
		}
	}
}
