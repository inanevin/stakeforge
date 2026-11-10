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

#include "game_world_controller.hpp"

#include <sfg/core/data/ostream.hpp>
#include <sfg/core/gfx/backend/backend.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/time.hpp>
#include <sfg/core/runtime/project/project_package_meta.hpp>
#include <sfg/core/runtime/engine/engine_runtime.hpp>
#include <sfg/core/runtime/engine/perf_metrics.hpp>
#include <sfg/core/runtime/engine/engine_threads.hpp>
#include <sfg/core/runtime/project/project_settings.hpp>
#include <sfg/core/runtime/plugin/game_plugin_runtime.hpp>
#include <sfg/core/runtime/world_renderer/stock_world_renderer_context.hpp>
#include <sfg/core/runtime/world_renderer/world_renderer_view.hpp>
#include <sfg/core/runtime/world_renderer/world_renderer_camera.hpp>
#include <sfg/core/runtime/resources/world_cook.hpp>
#include <sfg/core/runtime/resources/common_resources.hpp>
#include <sfg/core/runtime/resources/resource_manager.hpp>
#include <sfg/core/runtime/world/world_init_config.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define GAME_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT	   (8192 * 4)
#define GAME_WORLD_DEBUG_LINE_INDEX_MAX_COUNT	   (8192 * 6)
#define GAME_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT 164000
#define GAME_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT  164000
#define GAME_WORLD_DEBUG_TEXT_COMMAND_MAX_COUNT	   256
#define GAME_WORLD_DEBUG_TEXT_BUDGET_BYTES		   32768
#define GAME_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT	   16384
#define GAME_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT	   24576
#define GAME_WORLD_DEBUG_TEXTURE_MAX_COUNT		   8192
#define GAME_WORLD_LIGHT_MAX_COUNT				   1024
#define GAME_WORLD_REFLECTION_PROBE_MAX_COUNT	   256
#define GAME_WORLD_ENTITY_MAX_COUNT				   (1024 * 10)
#define GAME_WORLD_SPRITE_MAX_COUNT				   256
#define GAME_WORLD_PARTICLE_MAX_COUNT			   8192
#define GAME_WORLD_BONE_MAX_COUNT				   4096

	bool game_world_controller_t::init(vec2u16_t render_resolution, const project_package_meta_t& package_meta)
	{
		SFG_ASSERT(!_initialized);

		const project_settings_t& project_settings = engine_runtime_t::get().get_project_settings();
		world_physics_config_t	  physics_config   = project_settings.physics.make_runtime_config(project_settings.world_physics_rate, project_settings.max_sim_steps);

		physics_config.physics_enabled = true;

		const world_init_config_t world_config{
			.physics = physics_config,
			.debug_draw =
				{
					.line_vertex_max_count	   = GAME_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT,
					.line_index_max_count	   = GAME_WORLD_DEBUG_LINE_INDEX_MAX_COUNT,
					.triangle_vertex_max_count = GAME_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT,
					.triangle_index_max_count  = GAME_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT,
					.text_command_max_count	   = GAME_WORLD_DEBUG_TEXT_COMMAND_MAX_COUNT,
					.text_budget_bytes		   = GAME_WORLD_DEBUG_TEXT_BUDGET_BYTES,
					.text_vertex_max_count	   = GAME_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT,
					.text_index_max_count	   = GAME_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT,
					.texture_max_count		   = GAME_WORLD_DEBUG_TEXTURE_MAX_COUNT,
				},
			.render_resolution = render_resolution,
		};

		const world_render_context_config_t render_context_config{
			.size				  = render_resolution,
			.entity_max			  = GAME_WORLD_ENTITY_MAX_COUNT,
			.sprite_max			  = GAME_WORLD_SPRITE_MAX_COUNT,
			.particle_max		  = GAME_WORLD_PARTICLE_MAX_COUNT,
			.bone_max			  = GAME_WORLD_BONE_MAX_COUNT,
			.light_max			  = GAME_WORLD_LIGHT_MAX_COUNT,
			.reflection_probe_max = GAME_WORLD_REFLECTION_PROBE_MAX_COUNT,
			.shadow_view_max	  = ENGINE_SHADOW_VIEW_MAX,
		};

		_main_world.init(world_config);
		_world_renderer = game_plugin_runtime_t::get().create_world_renderer(_main_world);

		if (_world_renderer == nullptr)
		{
			_main_world.uninit();
			return false;
		}

		_world_renderer->init(render_context_config, world_config.debug_draw);
		_package_meta = &package_meta;

		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);

		_initialized = true;

		if (!load_world(package_meta.main_world.sid))
		{
			uninit();
			return false;
		}

		return true;
	}

	void game_world_controller_t::uninit()
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		if (_main_world.is_playing())
			_main_world.end_play();

		_world_renderer->uninit();
		game_plugin_runtime_t::get().destroy_world_renderer(_world_renderer);
		_world_renderer = nullptr;

		_main_world.unload_all_used_resources();
		_main_world.uninit();

		_package_meta = nullptr;
		_last_fixed_step_us.store(0, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		_previous_time_us = 0;
		_accumulator_us	  = 0;
		_render_alpha	  = 0.0f;
		_initialized	  = false;
	}

	void game_world_controller_t::tick()
	{
		ZoneScoped;

		SFG_ASSERT(_initialized);

		const project_settings_t& settings = engine_runtime_t::get().get_project_settings();
		const i64				  now	   = time_t::get_cpu_microseconds();
		const i64				  delta_us = now - _previous_time_us;
		const i64				  fixed_us = settings.world_tick_rate == 0 ? 0 : 1000000 / static_cast<i64>(settings.world_tick_rate);
		u32						  steps	   = 0;

		_previous_time_us = now;

		if (fixed_us > 0)
		{
			_accumulator_us += delta_us;

			const f32 dt_seconds = 1.0f / static_cast<f32>(settings.world_tick_rate);

			while (_accumulator_us >= fixed_us && steps < settings.max_sim_steps)
			{
				_accumulator_us -= fixed_us;

				_main_world.get_debug_draw().begin_frame();
				_main_world.tick_logic(dt_seconds);
				_main_world.update_world_transforms_for_tick();
				_main_world.tick_physics(dt_seconds);
				_main_world.update_world_transforms();
				_main_world.tick_animation_logic(dt_seconds);
				_main_world.update_world_transforms();
				update_screen_camera();
				_main_world.tick_post(dt_seconds);

				++steps;
			}

			_fixed_step_us.store(fixed_us, std::memory_order_relaxed);
			_last_fixed_step_us.store(now - _accumulator_us, std::memory_order_release);
		}
		else
		{
			_accumulator_us = 0;
			_fixed_step_us.store(0, std::memory_order_relaxed);
			_last_fixed_step_us.store(now, std::memory_order_release);
		}

		if (steps == 0)
		{
			_main_world.update_world_transforms();
			update_screen_camera();
		}

		_main_world.tick_finalize();

		if (steps != 0)
			_main_world.get_debug_draw().debug_draw_missing_resources(_main_world);

		if (steps != 0 && _main_world.get_stats_debug_type() != world_stats_debug_type_e::none)
		{
			_main_world.draw_stats({
				.font							 = DEFAULT_FONT_ASSET_GUID,
				.ram_usage_bytes				 = perf_metrics_t::get_ram_usage_bytes(),
				.vram_usage_bytes				 = perf_metrics_t::get_vram_usage_bytes(),
				.main_thread_cpu_ms				 = perf_metrics_t::get_main_thread_time_ms(),
				.render_thread_cpu_ms			 = perf_metrics_t::get_render_thread_time_ms(),
				.render_thread_work_cpu_ms		 = perf_metrics_t::get_render_thread_work_time_ms(),
				.main_fps						 = perf_metrics_t::get_main_thread_fps(),
				.render_fps						 = perf_metrics_t::get_render_thread_fps(),
				.world_draw_call_count			 = perf_metrics_t::get_world_draw_call_count(),
				.world_visible_renderables_count = perf_metrics_t::get_world_visible_renderables_count(),
				.world_bone_count				 = perf_metrics_t::get_world_bone_count(),
				.world_light_count				 = perf_metrics_t::get_world_light_count(),
				.world_probe_count				 = perf_metrics_t::get_world_probe_count(),
			});
		}

		resource_manager_t::get().flush_material_updates();

		produce_snapshot();
	}

	void game_world_controller_t::resize(vec2u16_t render_resolution)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		if (_world_renderer->get_size() == render_resolution)
			return;

		_world_renderer->resize(render_resolution);
		_main_world.get_screen().set_render_size(render_resolution);
		update_screen_camera();
	}

	bool game_world_controller_t::load_world(sid_t world)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		const world_meta_t* selected_world = nullptr;

		for (const world_meta_t& world_meta : _package_meta->worlds)
		{
			if (world_meta.sid != world)
				continue;

			selected_world = &world_meta;
			break;
		}

		if (selected_world == nullptr)
		{
			SFG_ERR("requested world is not included in the project package: {0}", world);
			return false;
		}

		ostream_t world_source = {};

		if (!engine_runtime_t::get().get_resource_file_system().read_resource(world, 0, 0, world_source))
		{
			SFG_ERR("failed to read the packaged world: {0}", world);
			return false;
		}

		const char* const	 world_source_begin = reinterpret_cast<const char*>(world_source.get_raw());
		const char* const	 world_source_end	= world_source_begin + world_source.get_size();
		const nlohmann::json world_json			= nlohmann::json::parse(world_source_begin, world_source_end, nullptr, false);

		if (!world_json.is_object() || !world_json.value<nlohmann::json>("root_entities", nlohmann::json::array()).is_array())
		{
			SFG_ERR("packaged world JSON is invalid: {0}", world);
			return false;
		}

		if (_main_world.is_playing())
			_main_world.end_play();

		_main_world.clear_used_resources();
		_main_world.reset_world_state();

		if (!world_cooker_t::world_from_json(_main_world, world_json))
		{
			SFG_ERR("failed to deserialize packaged world: {0}", world);
			return false;
		}

		_main_world.set_world_name(selected_world->name_hash);
		_main_world.update_world_transforms();
		update_screen_camera();
		_main_world.begin_play();
		_main_world.tick_finalize();

		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_render_alpha	  = 0.0f;
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);

		resource_manager_t::get().flush_material_updates();

		produce_snapshot();

		return true;
	}

	bool game_world_controller_t::load_world_by_name_hash(sid_t name_hash)
	{
		SFG_ASSERT(_initialized);

		for (const world_meta_t& world : _package_meta->worlds)
		{
			if (world.name_hash == name_hash)
				return load_world(world.sid);
		}

		SFG_ERR("requested world name is not included in the project package: {0}", name_hash);
		return false;
	}

	bool game_world_controller_t::acquire_render_world()
	{
		ZoneScoped;

		SFG_ASSERT(_initialized);
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		_render_alpha = calculate_render_alpha();

		_world_renderer->acquire_snapshot();

		return true;
	}

	bool game_world_controller_t::render_world(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		SFG_ASSERT(_initialized);
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		_world_renderer->render_scene(_render_alpha, frame_index, global_cbv_index, global_layout);
		_world_renderer->render_post_process(frame_index, global_cbv_index, global_layout);
		_world_renderer->render_finalize(frame_index, global_cbv_index, global_layout);

		gfx_backend::get().queue_signal(queue, &signal, &signal_value, 1);

		return true;
	}

	void game_world_controller_t::update_screen_camera()
	{
		world_renderer_camera_t camera = {};
		world_screen_t&			screen = _main_world.get_screen();

		if (!_main_world.refresh_main_camera(camera))
		{
			screen.clear_camera();
			return;
		}

		world_renderer_view_t view = {};

		_world_renderer->build_render_view(camera, screen.get_render_size(), 1.0f, view);
		screen.update_camera(view);
	}

	void game_world_controller_t::produce_snapshot()
	{
		_world_renderer->produce_snapshot(_main_world, engine_runtime_t::get().get_project_settings());
		_world_renderer->publish_snapshot();
	}

	f32 game_world_controller_t::calculate_render_alpha() const
	{
		const i64 fixed_us = _fixed_step_us.load(std::memory_order_relaxed);

		if (fixed_us <= 0)
			return 0.0f;

		const i64 last_fixed_step_us = _last_fixed_step_us.load(std::memory_order_acquire);
		const i64 now				 = time_t::get_cpu_microseconds();
		const f32 alpha				 = static_cast<f32>(static_cast<double>(now - last_fixed_step_us) / static_cast<double>(fixed_us));

		if (alpha < 0.0f)
			return 0.0f;

		if (alpha > 1.0f)
			return 1.0f;

		return alpha;
	}
}
