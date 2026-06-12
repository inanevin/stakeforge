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

#include "common_editor.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/panels/editor_panel_types.hpp"
#include "editor_renderer.hpp"
#include "editor_surface.hpp"
#include "editor_world_controller.hpp"
#include <sfg/data/unique.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/resources/resource_pack.hpp>
#include <sfg/vendor/taskflow/core/declarations.hpp>

namespace sfg
{
	class editor_panel_t;

	class editor_app_t
	{
	public:
		editor_app_t();
		~editor_app_t();
		editor_app_t(const editor_app_t&)			 = delete;
		editor_app_t& operator=(const editor_app_t&) = delete;

		inline static editor_app_t& get()
		{
			static editor_app_t s_instance;
			return s_instance;
		}

		bool init();
		void uninit();
		void tick();

		void destroy_surface(surface_handle_t handle);
		bool create_project(const char* path);
		bool load_project(const char* path);
		void save_layout();
		void apply_default_layout();
		void set_debug_mode(bool enabled);
		void set_text_subpixel_enabled(bool enabled);
		void create_payload(const char* text, editor_payload_type_e type, void* user_ptr, vec2u16_t size_value = {});

		editor_panel_t*	  find_panel(editor_panel_type_e type, surface_handle_t surface_handle = {});
		void			  show_panel(editor_panel_type_e type, surface_handle_t surface_handle = {});
		void			  set_main_world_to_panel();
		editor_surface_t& get_main_surface();
		tf::Executor&	  get_editor_work_executor();

		inline bool is_debug_mode_enabled() const
		{
			return _debug_mode;
		}

	private:
		static constexpr size_t MAIN_FRAME_ALLOC_SIZE = 1024ull * 1024ull * 4ull;

		void			  load_surface_dock_layout(editor_surface_t& surface, const string_t& dock_layout);
		void			  unload_current_project();
		editor_surface_t& get_surface_by_runtime(window_runtime_t& runtime);
		surface_handle_t  create_surface(const vec2i16_t& pos, const vec2u16_t& size, editor_surface_type_e type);
		static void		  on_window_event(void* hwnd, const struct window_event_t& ev, void* user_data);
		static bool		  on_window_client_hit_test(window_runtime_t& runtime, const vec2i16_t& pos, void* user_data);
		static void		  on_payload_unhandled(const editor_payload_t& payload, void* user_data);

	private:
		editor_renderer_t												_renderer;
		engine_runtime_t												_runtime;
		editor_world_controller_t										_world_controller;
		resource_pack_t													_resource_pack;
		editor_asset_manager_t											_asset_manager;
		dynamic_gen_pool_t<editor_surface_t, u16, editor_surface_tag_t> _surfaces;
		unique_t<tf::Executor>											_editor_work_executor;
		editor_payload_controller_t										_payload_controller;
		editor_modal_progress_bar_t										_debug_progress_modal;
		i64																_last_tick_us			 = 0;
		f32																_debug_modal_progress	 = 0.0f;
		u8																_atlas_upload_frame_slot = 0;
		bool															_debug_mode				 = false;
		bool															_close					 = false;
	};
}
