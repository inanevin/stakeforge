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

#include "game_world_controller.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/project/project_package_meta.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/project/project_settings.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define GAME_WORLD_SNAPSHOT_SLOT_COUNT			   3
#define GAME_WORLD_SNAPSHOT_SLOT_MASK			   0x3
#define GAME_WORLD_SNAPSHOT_FRESH_FLAG			   0x80
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
#define GAME_WORLD_DRAW_INITIAL_CAPACITY		   8000
#define GAME_WORLD_ENTITY_MAX_COUNT				   (1024 * 10)
#define GAME_WORLD_SPRITE_MAX_COUNT				   256
#define GAME_WORLD_PARTICLE_MAX_COUNT			   8192
#define GAME_WORLD_BONE_MAX_COUNT				   4096

	bool game_world_controller_t::init(vec2u16_t render_resolution, const project_package_meta_t& package_meta)
	{
		SFG_ASSERT(!_initialized);

		const project_settings_t& project_settings = engine_runtime_t::get().get_project_settings();
		const world_init_config_t world_config{
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
			.physics						   = project_settings.physics.make_runtime_config(project_settings.world_physics_rate, project_settings.max_sim_steps),
			.render_resolution				   = render_resolution,
			.render_entity_max_count		   = GAME_WORLD_ENTITY_MAX_COUNT,
			.render_sprite_max_count		   = GAME_WORLD_SPRITE_MAX_COUNT,
			.render_particle_max_count		   = GAME_WORLD_PARTICLE_MAX_COUNT,
			.render_bone_max_count			   = GAME_WORLD_BONE_MAX_COUNT,
			.render_bone_initial_capacity	   = 1024,
			.animation_graph_budget_bytes	   = 1 * 1024 * 1024,
			.component_table_initial_capacity  = 64,
			.entity_free_list_initial_capacity = 1024,
			.used_resource_initial_capacity	   = 512,
			.text_allocation_initial_capacity  = 1024,
			.text_budget_bytes				   = 64 * 1024,
			.physics_enabled				   = true,
		};

		const world_render_context_config_t render_context_config{
			.size				  = render_resolution,
			.entity_max			  = GAME_WORLD_ENTITY_MAX_COUNT,
			.sprite_max			  = GAME_WORLD_SPRITE_MAX_COUNT,
			.particle_max		  = GAME_WORLD_PARTICLE_MAX_COUNT,
			.bone_max			  = GAME_WORLD_BONE_MAX_COUNT,
			.light_max			  = GAME_WORLD_LIGHT_MAX_COUNT,
			.reflection_probe_max = GAME_WORLD_REFLECTION_PROBE_MAX_COUNT,
			.line_vertex_max	  = GAME_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT,
			.line_index_max		  = GAME_WORLD_DEBUG_LINE_INDEX_MAX_COUNT,
			.triangle_vertex_max  = GAME_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT,
			.triangle_index_max	  = GAME_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT,
			.text_vertex_max	  = GAME_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT,
			.text_index_max		  = GAME_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT,
			.debug_texture_max	  = GAME_WORLD_DEBUG_TEXTURE_MAX_COUNT,
			.shadow_view_max	  = ENGINE_SHADOW_VIEW_MAX,
		};

		const world_render_snapshot_initial_capacity_config_t snapshot_config{
			.material_initial_capacity			 = 256,
			.entity_initial_capacity			 = GAME_WORLD_DRAW_INITIAL_CAPACITY,
			.renderable_initial_capacity		 = GAME_WORLD_DRAW_INITIAL_CAPACITY,
			.draw_initial_capacity				 = GAME_WORLD_DRAW_INITIAL_CAPACITY,
			.sprite_initial_capacity			 = GAME_WORLD_SPRITE_MAX_COUNT,
			.particle_draw_initial_capacity		 = 1024 * 10,
			.particle_initial_capacity			 = GAME_WORLD_PARTICLE_MAX_COUNT,
			.bone_initial_capacity				 = 1024,
			.light_initial_capacity				 = GAME_WORLD_LIGHT_MAX_COUNT,
			.reflection_probe_initial_capacity	 = GAME_WORLD_REFLECTION_PROBE_MAX_COUNT,
			.line_vertex_initial_capacity		 = GAME_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT,
			.line_index_initial_capacity		 = GAME_WORLD_DEBUG_LINE_INDEX_MAX_COUNT,
			.triangle_vertex_initial_capacity	 = GAME_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT,
			.triangle_index_initial_capacity	 = GAME_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT,
			.text_vertex_initial_capacity		 = GAME_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT,
			.text_index_initial_capacity		 = GAME_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT,
			.debug_texture_initial_capacity		 = GAME_WORLD_DEBUG_TEXTURE_MAX_COUNT,
			.canvas_draw_buffer_initial_capacity = 64,
			.canvas_vertex_initial_capacity		 = 4096,
			.canvas_index_initial_capacity		 = 32768,
		};

		const world_render_prep_initial_capacity_config_t render_prep_config{
			.view_initial_capacity				= 65,
			.depth_queue_initial_capacity		= GAME_WORLD_DRAW_INITIAL_CAPACITY * 8,
			.opaque_queue_initial_capacity		= GAME_WORLD_DRAW_INITIAL_CAPACITY * 8,
			.transparent_queue_initial_capacity = GAME_WORLD_DRAW_INITIAL_CAPACITY * 8,
			.shadow_queue_initial_capacity		= GAME_WORLD_DRAW_INITIAL_CAPACITY * ENGINE_SHADOW_VIEW_MAX,
			.visible_queue_initial_capacity		= GAME_WORLD_DRAW_INITIAL_CAPACITY,
			.shadow_view_initial_capacity		= ENGINE_SHADOW_VIEW_MAX,
		};

		_main_world.init(world_config);
		_render_context.init(render_context_config);
		_render_prep_data.reserve(render_prep_config);
		_package_meta = &package_meta;

		for (u32 slot_index = 0; slot_index < GAME_WORLD_SNAPSHOT_SLOT_COUNT; ++slot_index)
		{
			world_render_snapshot_t& snapshot = _snapshot_slots[slot_index];
			snapshot.reserve(snapshot_config);
			snapshot.main_view = {
				.near_plane	 = 0.1f,
				.far_plane	 = 1000.0f,
				.fov_degrees = 60.0f,
			};
		}

		_producer_slot = 0;
		_consumer_slot = 1;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);

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

		_render_context.uninit();
		_main_world.unload_all_used_resources();
		_main_world.uninit();

		for (u32 slot_index = 0; slot_index < GAME_WORLD_SNAPSHOT_SLOT_COUNT; ++slot_index)
			_snapshot_slots[slot_index] = {};

		_render_prep_data = {};
		_package_meta	  = nullptr;
		_render_snapshot  = nullptr;
		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_last_fixed_step_us.store(0, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		_previous_time_us = 0;
		_accumulator_us	  = 0;
		_render_alpha	  = 0.0f;
		_producer_slot	  = 0;
		_consumer_slot	  = 0;
		_pending_world	  = NULL_SID;
		_initialized	  = false;
	}

	void game_world_controller_t::tick()
	{
		ZoneScoped;

		SFG_ASSERT(_initialized);

		_main_world.get_debug_draw().begin_frame();

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

				_main_world.tick_logic(dt_seconds);
				_main_world.update_world_transforms();
				_main_world.tick_physics(dt_seconds);
				_main_world.tick_logic_post_physics(dt_seconds);
				_main_world.tick_animation_prep(dt_seconds);
				_main_world.tick_animation_logic(dt_seconds);
				_main_world.tick_logic_post_animation(dt_seconds);
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
			_main_world.update_world_transforms(false);

		_main_world.draw_world_script_debug();
		_main_world.get_debug_draw().debug_draw_missing_resources(_main_world);

		produce_snapshot();
	}

	void game_world_controller_t::resize(vec2u16_t render_resolution)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		if (_render_context.get_size() == render_resolution)
			return;

		_render_context.resize(render_resolution);
	}

	bool game_world_controller_t::load_world(sid_t world)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		bool world_exists = false;

		for (const world_meta_t& world_meta : _package_meta->worlds)
		{
			if (world_meta.sid != world)
				continue;

			world_exists = true;
			break;
		}

		if (!world_exists)
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
		_pending_world = NULL_SID;

		world_cooker_t::world_from_json(_main_world, world_json);
		_main_world.update_world_transforms(false);
		_main_world.begin_play();

		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_render_alpha	  = 0.0f;
		_producer_slot	  = 0;
		_consumer_slot	  = 1;
		_render_snapshot  = nullptr;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);

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

	bool game_world_controller_t::queue_world_load_by_name_hash(sid_t name_hash)
	{
		SFG_ASSERT(_initialized);

		if (_pending_world != NULL_SID)
			return false;

		for (const world_meta_t& world : _package_meta->worlds)
		{
			if (world.name_hash != name_hash)
				continue;

			_pending_world = world.sid;
			return true;
		}

		return false;
	}

	bool game_world_controller_t::apply_pending_world_load()
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(_pending_world != NULL_SID);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		const sid_t pending_world = _pending_world;
		_pending_world			  = NULL_SID;

		return load_world(pending_world);
	}

	bool game_world_controller_t::acquire_render_world()
	{
		ZoneScoped;

		SFG_ASSERT(_initialized);
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		_render_alpha = calculate_render_alpha();

		u8 mailbox = _snapshot_mailbox.load(std::memory_order_acquire);

		while (mailbox & GAME_WORLD_SNAPSHOT_FRESH_FLAG)
		{
			if (_snapshot_mailbox.compare_exchange_weak(mailbox, _consumer_slot, std::memory_order_acquire, std::memory_order_acquire))
			{
				_consumer_slot = static_cast<u8>((mailbox & GAME_WORLD_SNAPSHOT_SLOT_MASK) % GAME_WORLD_SNAPSHOT_SLOT_COUNT);
				break;
			}
		}

		_render_snapshot = &_snapshot_slots[_consumer_slot];
		return true;
	}

	bool game_world_controller_t::render_world(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		SFG_ASSERT(_initialized);
		SFG_ASSERT(_render_snapshot != nullptr);
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		_render_prep_data.reset();
		world_rendering_t::render_world(_render_context, *_render_snapshot, _render_prep_data, _render_alpha, frame_index, global_cbv_index, global_layout);
		_render_snapshot = nullptr;

		gfx_backend::get().queue_signal(queue, &signal, &signal_value, 1);
		return true;
	}

	void game_world_controller_t::produce_snapshot()
	{
		world_render_snapshot_t& snapshot = _snapshot_slots[_producer_slot];
		world_snapshot_producer_t::produce(_main_world, snapshot, engine_runtime_t::get().get_project_settings());
		publish_snapshot();
	}

	void game_world_controller_t::publish_snapshot()
	{
		const u8 previous = _snapshot_mailbox.exchange(_producer_slot | GAME_WORLD_SNAPSHOT_FRESH_FLAG, std::memory_order_release);

		_producer_slot = static_cast<u8>((previous & GAME_WORLD_SNAPSHOT_SLOT_MASK) % GAME_WORLD_SNAPSHOT_SLOT_COUNT);
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
