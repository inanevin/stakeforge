// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_editor.hpp"
#include "editor_renderer.hpp"
#include "editor_surface.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/resource_pack.hpp>

#include <thread>

namespace sfg
{
	class editor_app_t
	{
	public:
		editor_app_t()								 = default;
		~editor_app_t()								 = default;
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

		surface_handle_t create_surface(const vec2i16_t& pos, const vec2u16_t& size, u16 settings_idx);
		void			 destroy_surface(surface_handle_t handle);

		inline resource_pack_t& get_resources()
		{
			return _resource_pack;
		}
		inline const resource_pack_t& get_resources() const
		{
			return _resource_pack;
		}

		inline resource_manager_t& get_resource_manager()
		{
			return resource_manager_t::get();
		}

	private:
		static constexpr size_t RENDER_FRAME_ALLOC_SIZE = 1024ull * 1024ull * 4ull;
		static constexpr size_t MAIN_FRAME_ALLOC_SIZE	= 1024ull * 1024ull * 4ull;

		void		init_surface_ui(editor_surface_t& surface);
		void		start_render();
		void		end_render();
		void		ensure_render_thread();
		void		render_loop();
		static void on_window_event(void* hwnd, const struct window_event_t& ev, void* user_data);

	private:
		editor_renderer_t												_renderer;
		resource_pack_t													_resource_pack;
		dynamic_gen_pool_t<editor_surface_t, u16, editor_surface_tag_t> _surfaces;
		vector_t<surface_render_target_t>								_render_targets;
		std::thread														_render_thread;
		atomic_t<bool>													_render_thread_active = false;
		f32																_render_delta_time	  = 0.0f;
		i64																_last_tick_us		  = 0;
	};
}
