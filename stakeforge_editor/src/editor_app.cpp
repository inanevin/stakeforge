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
#include "editor_app.hpp"
#include "assets/editor_asset_manager_util.hpp"
#include "assets/thumbnail/editor_asset_thumbnail_manager.hpp"
#include "assets/thumbnail/editor_thumbnail_render_service.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "editor_project_cooker.hpp"
#include "editor_settings.hpp"
#include "editor_surface_controller.hpp"
#include "scripting/editor_script_manager.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_primary_base.hpp"
#include "ui/widgets/editor_splash_screen.hpp"
#include "world/editor_world.hpp"
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/perf_metrics.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/scripting/api/script_api_game.hpp>
#include <sfg/runtime/scripting/api/script_api_platform.hpp>
#include <sfg/vendor/taskflow/taskflow.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define EDITOR_RAW_WHEEL_DELTA		120.0f
#define EDITOR_SPLASH_ASPECT_X		16.0f
#define EDITOR_SPLASH_ASPECT_Y		9.0f
#define EDITOR_SPLASH_MONITOR_SCALE 0.25f

	editor_app_t::editor_app_t()  = default;
	editor_app_t::~editor_app_t() = default;

	void editor_app_t::on_project_assets_progress(void* user_data, f32 progress, const char* progress_text)
	{
		editor_app_t& app = *static_cast<editor_app_t*>(user_data);

		app._splash_progress.store(progress, std::memory_order_release);
		{
			LOCK_GUARD(app._splash_progress_text_mutex);
			app._splash_progress_text = progress_text;
		}

		app._splash_progress_text_dirty.store(true, std::memory_order_release);
		if (progress >= 1.0f)
			app.request_switch_mode(editor_app_mode_e::normal);
	}

	u8 editor_app_t::get_script_game_render_resolution(vec2u16_t& out_resolution)
	{
		out_resolution = vec2u16_t::zero;

		editor_world_controller_t&	world_controller = editor_app_t::get().get_world_controller();
		const editor_world_handle_t main_world		 = world_controller.get_main_world_handle();

		if (main_world.is_null())
			return 0;

		out_resolution = world_controller.get_editor_world(main_world)->get_render_resolution();
		return 1;
	}

	u8 editor_app_t::set_script_game_render_resolution(const vec2u16_t& resolution)
	{
		editor_world_controller_t&	world_controller = editor_app_t::get().get_world_controller();
		const editor_world_handle_t main_world		 = world_controller.get_main_world_handle();

		if (main_world.is_null())
			return 0;

		world_controller.resize_world(main_world, resolution);
		return 1;
	}

	u8 editor_app_t::load_script_game_world(sid_t world_name_hash)
	{
		return editor_app_t::get().get_world_controller().queue_game_world_load(world_name_hash) ? 1 : 0;
	}

	u8 editor_app_t::restart_script_game_world()
	{
		return editor_app_t::get().get_world_controller().queue_game_world_restart() ? 1 : 0;
	}

	void editor_app_t::quit_script_game()
	{
		editor_app_t::get().get_world_controller().queue_game_quit();
	}

	bool editor_app_t::init(const editor_app_config_t& config)
	{
		SFG_ASSERT(config.main_frame_budget_bytes != 0);
		SFG_ASSERT(config.renderer.frame_budget_bytes != 0);
		SFG_ASSERT(config.editor_work_executor_thread_count != 0);

		_config = config;

		g_window_api_enabled = false;

		engine_runtime_t& runtime = engine_runtime_t::get();

		editor_text_rasterization_t::set_subpixel_enabled(true);
		editor_directories_t::init_paths();
		if (!editor_settings_t::get().ensure_loaded())
			return false;

		/* engine globals, init backend & engine & editor managers */
		runtime.init_globals(config.engine.global);

		if (!runtime.init_backend(config.engine.backend))
		{
			runtime.uninit_globals();
			return false;
		}

		if (!runtime.init())
		{
			runtime.uninit_globals();
			runtime.uninit_backend();
			return false;
		}

		_asset_manager.init();

		/* resources & renderers init */

		runtime.get_resource_file_system().set_mode_directory("", editor_directories_t::get_editor_resource_cache().c_str());

		const resource_preload_t::init_params_t engine_preload_params{
			.manifest_path = editor_directories_t::get_engine_manifest(),
			.assets_dir	   = editor_directories_t::get_editor_assets(),
			.cache_dir	   = editor_directories_t::get_editor_resource_cache(),
		};
		if (!_engine_resource_preload.init(resource_manager_t::get(), engine_preload_params))
		{
			_world_controller.uninit();
			runtime.uninit();
			runtime.uninit_globals();
			runtime.uninit_backend();
			return false;
		}

		const resource_preload_t::init_params_t editor_preload_params{
			.manifest_path = editor_directories_t::get_editor_manifest(),
			.assets_dir	   = editor_directories_t::get_editor_assets(),
			.cache_dir	   = editor_directories_t::get_editor_resource_cache(),
		};
		if (!_editor_resource_preload.init(resource_manager_t::get(), editor_preload_params))
		{
			_engine_resource_preload.uninit();
			_world_controller.uninit();
			runtime.uninit();
			runtime.uninit_globals();
			runtime.uninit_backend();
			return false;
		}

		render_resources_t::get().drain_requests();
		resource_manager_t::get().flush();
		if (!_renderer.init(config.renderer))
		{
			_editor_resource_preload.uninit();
			_engine_resource_preload.uninit();
			_world_controller.uninit();
			runtime.uninit();
			runtime.uninit_globals();
			runtime.uninit_backend();
			return false;
		}
		editor_surface_controller_t::get().init(_renderer, _payload_controller);

		frame_allocator_tls_t::init(config.main_frame_budget_bytes);
		_editor_work_executor = make_unique<tf::Executor>(config.editor_work_executor_thread_count);

		const string_t& last_project = editor_settings_t::get().last_project_path;
		if (!last_project.empty() && file_system_t::exists(last_project.c_str()))
		{
			if (editor_project_t::get().try_load(last_project.c_str()))
				switch_mode(editor_app_mode_e::splash);
			else
			{
				SFG_ERR("failed loading last project {0}", last_project.c_str());
				switch_mode(editor_app_mode_e::project_creator);
			}
		}
		else
			switch_mode(editor_app_mode_e::project_creator);

		if (editor_surface_controller_t::get().is_empty())
		{
			_editor_work_executor->wait_for_all();
			_editor_work_executor.reset();
			editor_surface_controller_t::get().uninit();
			_renderer.uninit();
			_editor_resource_preload.uninit();
			_engine_resource_preload.uninit();
			runtime.uninit();
			runtime.uninit_globals();
			runtime.uninit_backend();
			frame_allocator_tls_t::uninit();
			return false;
		}

		_last_tick_us = time_t::get_cpu_microseconds();

		tick();
		return true;
	}

	void editor_app_t::uninit()
	{
		engine_runtime_t& runtime = engine_runtime_t::get();

		set_script_api_platform_window_runtime(nullptr);
		set_script_api_game_callbacks(nullptr, nullptr, nullptr, nullptr, nullptr);

		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();
		SFG_ASSERT(surfaces.is_empty());

		stop_render();
		resource_manager_t::get().flush();
		_editor_work_executor->wait_for_all();
		_renderer.uninit();
		_editor_resource_preload.uninit();
		_engine_resource_preload.uninit();

		if (_mode == editor_app_mode_e::normal)
			uninit_normal_mode();

		_asset_manager.uninit();
		_editor_work_executor.reset();
		surfaces.uninit();
		runtime.uninit();
		runtime.uninit_globals();
		runtime.uninit_backend();
		frame_allocator_tls_t::uninit();
		_config = {};
		_mode	= editor_app_mode_e::none;
	}

	void editor_app_t::stop_render()
	{
		_renderer.end_render();
	}

	bool editor_app_t::init_normal_mode()
	{
		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();

		SFG_ASSERT(surfaces.is_empty());

		editor_asset_manager_util_t::ensure_default_meshes();

		const surface_handle_t payload_surface = surfaces.create_surface({0, 0}, {160, 24}, editor_surface_type_e::payload);

		if (payload_surface.is_null())
			return false;

		_payload_controller.init(surfaces.get_surface(payload_surface));
		_payload_controller.set_unhandled_listener(editor_surface_controller_t::on_payload_unhandled, this);

		_command_system.init(_config.command_system);
		_world_controller.init();

		editor_asset_thumbnail_manager_t::get().init();
		editor_thumbnail_render_service_t::get().init();

		_asset_manager.initialize_cooked_resource_tracking();
		_asset_manager.initialize_source_file_tracking();

		editor_asset_thumbnail_manager_t::get().load_all_ready();

		const auto cleanup = [this]() {
			editor_surface_controller_t::get().destroy_all_surfaces();
			editor_thumbnail_render_service_t::get().uninit();
			editor_asset_thumbnail_manager_t::get().uninit();
			_world_controller.uninit();
			_command_system.uninit();
		};

		const editor_layout_t& layout = editor_settings_t::get().layout;

		if (layout.windows.empty())
		{
			const editor_layout_window_t window = {};
			const vec2u16_t				 size	= (window.size.x == 0 || window.size.y == 0) ? vec2u16_t{1920, 1080} : window.size;
			const surface_handle_t		 handle = surfaces.create_surface(window.pos, size, editor_surface_type_e::primary);

			if (handle.is_null())
			{
				cleanup();
				return false;
			}

			process::set_window_maximized(surfaces.get_surface(handle).runtime->window_handle, window.maximized);
			editor_layout_t::load_surface_default_layout(surfaces.get_surface(handle));
		}
		else
		{
			const editor_layout_window_t* primary_window = nullptr;

			for (const editor_layout_window_t& window : layout.windows)
			{
				if (window.is_primary)
				{
					primary_window = &window;
					break;
				}
			}

			SFG_ASSERT(primary_window != nullptr);

			const vec2u16_t		   primary_size	  = (primary_window->size.x == 0 || primary_window->size.y == 0) ? vec2u16_t{1920, 1080} : primary_window->size;
			const surface_handle_t primary_handle = surfaces.create_surface(primary_window->pos, primary_size, editor_surface_type_e::primary);

			if (primary_handle.is_null())
			{
				cleanup();
				return false;
			}

			process::set_window_maximized(surfaces.get_surface(primary_handle).runtime->window_handle, primary_window->maximized);
			surfaces.load_surface_dock_layout(surfaces.get_surface(primary_handle), primary_window->dock_layout);
			surfaces.load_primary_main_toolbar(surfaces.get_surface(primary_handle), primary_window->main_toolbar);

			for (const editor_layout_window_t& window : layout.windows)
			{
				if (&window == primary_window)
					continue;

				const vec2u16_t		   secondary_size	= (window.size.x == 0 || window.size.y == 0) ? vec2u16_t{1920, 1080} : window.size;
				const surface_handle_t secondary_handle = surfaces.create_surface(window.pos, secondary_size, editor_surface_type_e::secondary);

				if (secondary_handle.is_null())
				{
					cleanup();
					return false;
				}

				process::set_window_maximized(surfaces.get_surface(secondary_handle).runtime->window_handle, window.maximized);
				surfaces.load_surface_dock_layout(surfaces.get_surface(secondary_handle), window.dock_layout);
			}
		}

		editor_project_t& proj = editor_project_t::get();

		surfaces.get_main_surface().primary->set_current_project_name(proj._runtime.name.c_str());

		_world_controller.load_dummy_world();
		_normal_world_load_pending = proj.settings.last_world_guid != NULL_SID;

		set_script_api_platform_window_runtime(surfaces.get_main_surface().runtime.get());
		set_script_api_game_callbacks(get_script_game_render_resolution, set_script_game_render_resolution, load_script_game_world, restart_script_game_world, quit_script_game);

		editor_script_manager_t::get().init();
		_asset_manager.initialize_script_file_tracking();
		editor_project_cooker_t::get().init();
		editor_script_manager_t::get().compile_scripts();

		return true;
	}

	void editor_app_t::uninit_normal_mode()
	{
		_asset_manager.uninitialize_script_file_tracking();
		editor_script_manager_t::get().uninit();
		editor_project_cooker_t::get().uninit();

		_asset_manager.flush_asset_cook_jobs();
		editor_thumbnail_render_service_t::get().uninit();
		editor_asset_thumbnail_manager_t::get().uninit();
		_world_controller.uninit();
		_command_system.uninit();

		_normal_world_load_pending = false;
	}

	void editor_app_t::switch_mode(editor_app_mode_e mode)
	{
		if (_mode == mode)
			return;

		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();

		if (_mode == editor_app_mode_e::normal)
		{
			set_script_api_platform_window_runtime(nullptr);
			set_script_api_game_callbacks(nullptr, nullptr, nullptr, nullptr, nullptr);
		}

		surfaces.destroy_all_surfaces();

		if (_mode == editor_app_mode_e::normal)
			uninit_normal_mode();

		if (mode == editor_app_mode_e::normal)
		{
			if (!init_normal_mode())
			{
				_mode = editor_app_mode_e::none;
				return;
			}
		}
		else if (mode == editor_app_mode_e::splash || mode == editor_app_mode_e::project_creator)
		{
			vector_t<monitor_info_t> monitors;
			process::get_all_monitors(monitors);
			const monitor_info_t& monitor = process::find_primary_monitor(monitors);

			const u16		height = static_cast<u16>(static_cast<f32>(monitor.work_size.y) * EDITOR_SPLASH_MONITOR_SCALE);
			const u16		width  = static_cast<u16>(static_cast<f32>(height) * EDITOR_SPLASH_ASPECT_X / EDITOR_SPLASH_ASPECT_Y);
			const vec2u16_t size   = {width, height};
			const vec2i16_t pos	   = {
				static_cast<i16>(monitor.position.x + static_cast<i16>((monitor.work_size.x - width) / 2)),
				static_cast<i16>(monitor.position.y + static_cast<i16>((monitor.work_size.y - height) / 2)),
			};

			surfaces.create_surface(pos, size, mode == editor_app_mode_e::splash ? editor_surface_type_e::splash : editor_surface_type_e::project_creator);

			if (mode == editor_app_mode_e::splash)
			{
				editor_project_t& proj = editor_project_t::get();
				engine_runtime_t::get().get_resource_file_system().set_mode_directory(proj._runtime.cache_path.c_str(), editor_directories_t::get_editor_resource_cache().c_str());
				_splash_progress.store(0.0f, std::memory_order_release);
				{
					LOCK_GUARD(_splash_progress_text_mutex);
					_splash_progress_text = "Ensuring default assets";
				}
				_splash_progress_text_dirty.store(true, std::memory_order_release);
				editor_asset_manager_util_t::ensure_project_assets_async(_asset_manager, get_editor_work_executor(), on_project_assets_progress, this);
			}
		}

		_mode = mode;
	}

	void editor_app_t::request_switch_mode(editor_app_mode_e mode)
	{
		_pending_mode.store(mode, std::memory_order_release);
	}

	void editor_app_t::set_debug_mode(bool enabled)
	{
		_debug_mode = enabled;
		editor_surface_controller_t::get().set_debug_mode(enabled);
	}

	void editor_app_t::set_text_subpixel_enabled(bool enabled)
	{
		editor_text_rasterization_t::set_subpixel_enabled(enabled);
		editor_surface_controller_t::get().set_text_subpixel_enabled();
	}

	void editor_app_t::create_payload(const char* text, editor_payload_type_e type, void* user_ptr, vec2u16_t size_value)
	{
		_payload_controller.create_payload(text, type, user_ptr, size_value);
	}

	void editor_app_t::tick()
	{
		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();
		bool						 tick	  = true;

#ifdef TRACY_ENABLE
		tracy::SetThreadName("main");
#endif

		while (tick)
		{
			const i64 main_thread_start_us = time_t::get_cpu_microseconds();

			editor_project_t& project = editor_project_t::get();

			frame_allocator_tls_t::reset();
			const editor_app_mode_e pending_mode = _pending_mode.exchange(editor_app_mode_e::none, std::memory_order_acquire);

			if (pending_mode != editor_app_mode_e::none)
			{
				switch_mode(pending_mode);
			}

			process::pump_os_messages();

			_engine_resource_preload.tick();
			_editor_resource_preload.tick();

			resource_manager_t::get().flush();

			if (_mode == editor_app_mode_e::normal)
			{
				editor_script_manager_t& script_manager = editor_script_manager_t::get();

				script_manager.tick();

				if (_normal_world_load_pending && script_manager.is_initial_activation_completed())
				{
					_normal_world_load_pending = false;

					if (!_world_controller.load_main_world(project.settings.last_world_guid))
						_world_controller.load_dummy_world();
				}

				_asset_manager.tick();
				editor_project_cooker_t::get().tick();

				if (!_asset_manager.is_import_in_progress())
					editor_asset_thumbnail_manager_t::get().tick();
			}

			if (_mode == editor_app_mode_e::normal)
			{
				const project_settings_t& settings = engine_runtime_t::get().get_project_settings();

				_world_controller.tick(settings.world_tick_rate, settings.world_physics_rate, settings.max_sim_steps);
			}

			if (_mode == editor_app_mode_e::splash)
			{
				for (editor_surface_t& surface : surfaces)
				{
					if (surface.type != editor_surface_type_e::splash)
						continue;

					surface.splash->update_progress(_splash_progress.load(std::memory_order_acquire));

					if (_splash_progress_text_dirty.exchange(false, std::memory_order_acq_rel))
					{
						LOCK_GUARD(_splash_progress_text_mutex);
						surface.splash->update_progress_text(_splash_progress_text.c_str());
					}
					break;
				}
			}

			if (_mode == editor_app_mode_e::normal)
			{
				_payload_controller.tick();
			}

			const i64 now = time_t::get_cpu_microseconds();
			const f32 dt  = static_cast<f32>(now - _last_tick_us) / 1.0e6f;
			_last_tick_us = now;

			engine_runtime_t::get().tick();
			surfaces.tick_surfaces(dt);

			if (surfaces.is_empty())
			{
				tick = false;
				break;
			}

			_renderer.ensure_render(_world_controller);

			perf_metrics_t::update_main_thread(time_t::get_cpu_microseconds() - main_thread_start_us);
			FrameMarkNamed("main");
		}
	}

}
