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

#include "common_editor.hpp"
#include "editor_surface.hpp"
#include "ui/editor_payload_type.hpp"
#include "ui/panels/editor_panel_types.hpp"
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>

namespace sfg
{
	class editor_panel_t;
	class editor_payload_controller_t;
	class editor_renderer_t;
	struct editor_payload_t;
	struct window_event_t;
	struct window_runtime_t;

	class editor_surface_controller_t final
	{
	public:
		editor_surface_controller_t()											   = default;
		~editor_surface_controller_t()											   = default;
		editor_surface_controller_t(const editor_surface_controller_t&)			   = delete;
		editor_surface_controller_t& operator=(const editor_surface_controller_t&) = delete;

		static inline editor_surface_controller_t& get()
		{
			static editor_surface_controller_t s_instance;
			return s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(editor_renderer_t& renderer, editor_payload_controller_t& payload_controller);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		surface_handle_t create_surface(const vec2i16_t& pos, const vec2u16_t& size, editor_surface_type_e type);
		void			 destroy_surface(surface_handle_t handle);
		void			 destroy_all_surfaces();
		void			 tick_surfaces(f32 dt);
		void			 save_layout();
		void			 apply_default_layout();
		void			 load_surface_dock_layout(editor_surface_t& surface, const string_t& dock_layout);
		void			 load_primary_main_toolbar(editor_surface_t& surface, const string_t& main_toolbar);
		void			 set_debug_mode(bool enabled);
		void			 set_text_subpixel_enabled();
		void			 request_focus_main_world_view();
		bool			 focus_main_world_view();
		void			 begin_editor_camera_cursor_capture(window_runtime_t& runtime);
		void			 end_editor_camera_cursor_capture(window_runtime_t& runtime);
		editor_panel_t*	 find_panel(editor_panel_type_e type, surface_handle_t surface_handle = {}, sid_t sub_item_id = 0);
		editor_panel_t*	 find_panel_on_surface(editor_panel_type_e type, surface_handle_t surface_handle, sid_t sub_item_id = 0);
		editor_panel_t*	 create_panel_instance(editor_panel_type_e type, surface_handle_t surface_handle = {}, bool prefer_existing_type_dock = true, sid_t sub_item_id = 0);
		editor_panel_t*	 show_panel(editor_panel_type_e type, surface_handle_t surface_handle = {}, sid_t sub_item_id = 0);
		void			 refresh_panel_title(editor_panel_t* panel);
		void			 request_close_panel(editor_panel_t* panel);
		static void		 on_payload_unhandled(const editor_payload_t& payload, void* user_data);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		editor_surface_t& get_main_surface();
		editor_surface_t& get_surface_by_runtime(window_runtime_t& runtime);
		editor_surface_t& get_surface_by_ui(ui::ui_context& ui);
		surface_handle_t  get_surface_handle_by_runtime(window_runtime_t& runtime);

		inline bool is_empty() const
		{
			return _surfaces.empty();
		}

		inline editor_surface_t& get_surface(surface_handle_t handle)
		{
			return _surfaces.get(handle);
		}

		inline const editor_surface_t& get_surface(surface_handle_t handle) const
		{
			return _surfaces.get(handle);
		}

		inline auto begin()
		{
			return _surfaces.begin();
		}

		inline auto end()
		{
			return _surfaces.end();
		}

		inline auto begin() const
		{
			return _surfaces.begin();
		}

		inline auto end() const
		{
			return _surfaces.end();
		}

	private:
		enum class editor_cursor_capture_e : u8
		{
			none,
			editor_camera,
		};

	private:
		bool is_any_modal_active() const;
		void capture_cursor(surface_handle_t surface, editor_cursor_capture_e capture);
		void release_cursor();

		static void on_window_event(void* hwnd, const window_event_t& ev, void* user_data);
		static bool on_window_client_hit_test(window_runtime_t& runtime, const vec2i16_t& pos, void* user_data);

	private:
		dynamic_gen_pool_t<editor_surface_t, u16, editor_surface_tag_t> _surfaces;
		editor_renderer_t*												_renderer				= nullptr;
		editor_payload_controller_t*									_payload_controller		= nullptr;
		surface_handle_t												_cursor_capture_surface = {};
		bool															_debug_mode				= false;
		bool															_close					= false;
		editor_cursor_capture_e											_cursor_capture			= editor_cursor_capture_e::none;
	};
}
