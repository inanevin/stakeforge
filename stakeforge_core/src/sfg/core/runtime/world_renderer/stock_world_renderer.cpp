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

#include "stock_world_renderer.hpp"
#include "stock_world_renderer_util.hpp"
#include "stock_world_renderer_passes.hpp"
#include "stock_world_renderer_draw_common.hpp"
#include "stock_world_renderer_snapshot_producer.hpp"
#include "core/gfx/backend/backend.hpp"
#include "core/gfx/common/commands.hpp"
#include "core/gfx/util/gfx_util.hpp"
#include "core/io/assert.hpp"
#include "core/runtime/engine/engine_threads.hpp"
#include "core/runtime/engine/perf_metrics.hpp"
#include "core/runtime/resources/shader_types.hpp"
#include <execution>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define STOCK_WORLD_RENDERER_SNAPSHOT_SLOT_MASK	 0x3
#define STOCK_WORLD_RENDERER_SNAPSHOT_FRESH_FLAG 0x80

	void stock_world_renderer_t::init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config)
	{
		_render_context.init(config, debug_draw_config);

		for (stock_world_renderer_snapshot_t& snapshot : _snapshot_slots)
		{
			snapshot.init(config, debug_draw_config);
			snapshot.main_view = {
				.near_plane	 = 0.1f,
				.far_plane	 = 1000.0f,
				.fov_degrees = 60.0f,
			};
		}

		_producer_slot = 0;
		_consumer_slot = 1;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
	}

	void stock_world_renderer_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		_visible_draws = {};

		_render_context.uninit();

		for (stock_world_renderer_snapshot_t& snapshot : _snapshot_slots)
			snapshot.uninit();

		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_producer_slot = 0;
		_consumer_slot = 0;
	}

	void stock_world_renderer_t::resize(vec2u16_t size)
	{
		_render_context.resize(size);
	}

	u8 stock_world_renderer_t::produce_snapshot(world_t& world, const project_settings_t& project_settings)
	{
		_snapshot_slots[_producer_slot].release_resources();
		stock_world_renderer_snapshot_producer_t::produce(world, _snapshot_slots[_producer_slot], project_settings);
		return _producer_slot;
	}

	void stock_world_renderer_t::publish_snapshot()
	{
		_snapshot_slots[_producer_slot].retain_resources();
		const u8 previous = _snapshot_mailbox.exchange(_producer_slot | STOCK_WORLD_RENDERER_SNAPSHOT_FRESH_FLAG, std::memory_order_acq_rel);
		_producer_slot	  = previous & STOCK_WORLD_RENDERER_SNAPSHOT_SLOT_MASK;
	}

	void stock_world_renderer_t::acquire_snapshot()
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		u8 current = _snapshot_mailbox.load(std::memory_order_acquire);

		while (current & STOCK_WORLD_RENDERER_SNAPSHOT_FRESH_FLAG)
		{
			if (_snapshot_mailbox.compare_exchange_weak(current, _consumer_slot, std::memory_order_acq_rel, std::memory_order_acquire))
			{
				_consumer_slot = current & STOCK_WORLD_RENDERER_SNAPSHOT_SLOT_MASK;
				break;
			}
		}
	}

	void stock_world_renderer_t::build_render_view(const world_renderer_camera_t& camera, const vec2u16_t& resolution, f32 interpolation_alpha, world_renderer_view_t& out_view) const
	{
		const vec3f_t p	  = vec3f_t::lerp(camera.prev_pos, camera.pos, interpolation_alpha);
		const quat_t  rot = quat_t::slerp(camera.prev_rot, camera.rot, interpolation_alpha);

		out_view.view		   = mat4x4_t::view(rot, p);
		out_view.inv_view	   = out_view.view.inverse();
		out_view.proj		   = mat4x4_t::perspective_reverse_z(camera.fov_degrees, static_cast<f32>(resolution.x) / static_cast<f32>(resolution.y), camera.near_plane, camera.far_plane);
		out_view.view_proj	   = out_view.proj * out_view.view;
		out_view.frustum	   = frustum_t::extract(out_view.view_proj);
		out_view.inv_proj	   = out_view.proj.inverse();
		out_view.inv_view_proj = out_view.view_proj.inverse();
		out_view.pos		   = p;
		out_view.near_plane	   = camera.near_plane;
		out_view.far_plane	   = camera.far_plane;
		out_view.fov_rads	   = camera.fov_degrees * DEG_2_RAD;
	}

	void stock_world_renderer_t::render_scene(f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		perf_metrics_t::update_world_draw_call_count(0);

		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		// main cam view.
		world_renderer_view_t& view = _render_views[frame_index];

		build_render_view(_snapshot_slots[_consumer_slot].main_view, _render_context.get_size(), interpolation_alpha, view);

		stock_world_renderer_prep_data_t prep_data = {};

		prep_data.reserve();

		stock_world_renderer_context_t&		   ctx		= _render_context;
		const stock_world_renderer_snapshot_t& snapshot = _snapshot_slots[_consumer_slot];

		ZoneScoped;

		gfx_backend&							   backend			  = gfx_backend::get();
		stock_world_renderer_shadow_context_t&	   shadow_context	  = ctx.get_shadow_context();
		stock_world_renderer_reflection_context_t& reflection_context = ctx.get_reflection_context();

		stock_world_renderer_util_t::prep_entity_buffer(ctx, snapshot, interpolation_alpha, frame_index);
		stock_world_renderer_util_t::prep_bone_buffer(ctx, snapshot, frame_index);

		const vec2u16_t render_size = ctx.get_size();

		// logarithmic depth scale for clusters
		const f32 cluster_log_scale = STOCK_WORLD_RENDERER_CLUSTER_DEPTH_SLICE_COUNT / std::log2(view.far_plane / view.near_plane);
		const f32 cluster_log_bias	= -std::log2(view.near_plane) * cluster_log_scale;
		u32		  light_counts[4]	= {};

		// first view is main camera view.
		prep_data.add_view({
			.view								 = view.view,
			.view_proj							 = view.view_proj,
			.inv_view							 = view.inv_view,
			.inv_view_proj						 = view.inv_view_proj,
			.frustum							 = view.frustum,
			.camera_pos							 = vec4f_t(view.pos.x, view.pos.y, view.pos.z, 1.0f),
			.cluster_depth						 = vec4f_t(view.near_plane, view.far_plane, cluster_log_scale, cluster_log_bias),
			.cluster_dims						 = {ctx.get_light_cluster_count_x(), ctx.get_light_cluster_count_y(), STOCK_WORLD_RENDERER_CLUSTER_DEPTH_SLICE_COUNT, STOCK_WORLD_RENDERER_CLUSTER_TILE_SIZE},
			.viewport_size						 = vec2f_t(render_size.x, render_size.y),
			.inv_viewport_size					 = vec2f_t(1.0f / render_size.x, 1.0f / render_size.y),
			.near_plane							 = view.near_plane,
			.far_plane							 = view.far_plane,
			.depth_texture_index				 = ctx.get_depth_texture_index(frame_index),
			.cluster_buffer_offset				 = 0,
			.cluster_light_indices_buffer_offset = 0,
			.cluster_light_capacity				 = STOCK_WORLD_RENDERER_CLUSTER_LIGHT_CAPACITY,
			.queue_flags						 = stock_world_renderer_queue_flag_depth | stock_world_renderer_queue_flag_opaque | stock_world_renderer_queue_flag_transparent | stock_world_renderer_queue_flag_visible,
		});

		// cluster light compute will dispatch per view.
		inplace_vector_t<stock_world_renderer_clustered_lighting_view_t, 1 + STOCK_WORLD_RENDERER_REFLECTION_FACE_COUNT> clustered_lighting_views = {};

		clustered_lighting_views.push_back({
			.view_data_index = ctx.get_view_render_pass_data_index(frame_index),
			.cluster_count_x = prep_data.views[0].cluster_dims[0],
			.cluster_count_y = prep_data.views[0].cluster_dims[1],
			.cluster_count_z = prep_data.views[0].cluster_dims[2],
		});

		// other preps
		stock_world_renderer_util_t::prep_shadows(ctx, snapshot, prep_data, view, interpolation_alpha);
		stock_world_renderer_util_t::prep_shadow_buffer(ctx, snapshot, prep_data, frame_index);
		stock_world_renderer_util_t::prep_light_buffer(ctx, snapshot, prep_data, interpolation_alpha, frame_index, light_counts);
		stock_world_renderer_util_t::prep_probes(ctx, snapshot, prep_data, view, interpolation_alpha, frame_index);

		// we render 1 reflection probe per frame max.
		const stock_world_renderer_reflection_probe_t* reflection_probe															= nullptr;
		u16											   reflection_cull_view_indices[STOCK_WORLD_RENDERER_REFLECTION_FACE_COUNT] = {};
		stock_world_renderer_reflection_allocation_t*  reflection_allocation = stock_world_renderer_util_t::prep_reflection_allocation(ctx, snapshot, prep_data, interpolation_alpha, frame_index, reflection_probe, reflection_cull_view_indices);

		if (reflection_allocation != nullptr)
		{
			if (reflection_probe->capture_type == stock_world_renderer_reflection_probe_capture_type_e::scene)
			{
				for (u8 face = 0; face < STOCK_WORLD_RENDERER_REFLECTION_FACE_COUNT; ++face)
				{
					const stock_world_renderer_prep_view_t& prep_view = prep_data.views[reflection_cull_view_indices[face]];

					clustered_lighting_views.push_back({
						.view_data_index = reflection_context.get_view_data_index(*reflection_allocation, frame_index, face),
						.cluster_count_x = prep_view.cluster_dims[0],
						.cluster_count_y = prep_view.cluster_dims[1],
						.cluster_count_z = prep_view.cluster_dims[2],
					});
				}
			}
		}

		// cluster light data and cluster indices are shared, make sure we have enough capacity for all probe rendering
		u32 light_cluster_count = 0;

		for (const stock_world_renderer_clustered_lighting_view_t& view : clustered_lighting_views)
			light_cluster_count += view.cluster_count_x * view.cluster_count_y * view.cluster_count_z;

		ctx.ensure_light_cluster_capacity(frame_index, light_cluster_count);

		stock_world_renderer_util_t::prep_debug_buffer(ctx, snapshot, frame_index);
		stock_world_renderer_util_t::prep_render_queues(ctx, snapshot, prep_data, interpolation_alpha, frame_index);
		stock_world_renderer_util_t::prep_render_pass_buffers(ctx, snapshot, prep_data, view, light_counts, frame_index);

		perf_metrics_t::update_world_visible_renderables_count(prep_data.visible_renderables_count);
		perf_metrics_t::update_world_bone_count(static_cast<u32>(snapshot.bones.size()));
		perf_metrics_t::update_world_light_count(static_cast<u32>(snapshot.lights.size()));
		perf_metrics_t::update_world_probe_count(prep_data.reflection_probe_count);

		// rendering
		const gfx_handle_t cmd_depth			  = ctx.get_command_buffer_depth(frame_index);
		const gfx_handle_t cmd_gbuffer			  = ctx.get_command_buffer_gbuffer(frame_index);
		const gfx_handle_t cmd_lighting			  = ctx.get_command_buffer_lighting(frame_index);
		const gfx_handle_t cmd_forward			  = ctx.get_command_buffer_forward(frame_index);
		const gfx_handle_t cmd_shadows			  = shadow_context.get_command_buffer(frame_index);
		const gfx_handle_t cmd_clustered_lighting = ctx.get_command_buffer_clustered_lighting(frame_index);
		const bool		   ssao_active			  = ctx.is_ssao_enabled() && snapshot.post_process.ssao.enabled != 0;
		const bool		   bloom_active			  = ctx.is_bloom_enabled() && snapshot.post_process.bloom.enabled != 0;

		const gfx_handle_t queue_gfx			  = backend.get_queue_gfx();
		const gfx_handle_t queue_compute		  = backend.get_queue_compute();
		const u32		   render_task_indices[5] = {0, 1, 2, 3, 4};
		const u32		   first_phase_task_count = 3 + static_cast<u32>(ssao_active) + static_cast<u32>(reflection_allocation != nullptr);

		std::for_each(std::execution::par, render_task_indices, render_task_indices + first_phase_task_count, [&](u32 task_index) {
			render_access_scope_t render_scope = {};

			switch (task_index)
			{
			case 0:
				stock_world_renderer_passes_t::render_depth_prepass(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);

				if (!prep_data.shadow_views.empty())
					stock_world_renderer_passes_t::render_shadows(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);

				return;
			case 1:
				stock_world_renderer_passes_t::render_gbuffer(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
				return;
			case 2:
				stock_world_renderer_passes_t::render_clustered_lighting(
					{
						.command_buffer				  = ctx.get_command_buffer_clustered_lighting(frame_index),
						.cluster_buffer				  = ctx.get_light_cluster_buffer(frame_index),
						.cluster_light_indices_buffer = ctx.get_light_cluster_indices_buffer(frame_index),
						.shader						  = ctx.get_clustered_light_culling_shader(),
						.lighting_data_index		  = ctx.get_lighting_render_pass_data_index(frame_index),
						.global_cbv_index			  = global_cbv_index,
					},
					{
						.data = clustered_lighting_views.data(),
						.size = clustered_lighting_views.size(),
					});
				return;
			default:
				break;
			}

			if (ssao_active && task_index == 3)
			{
				stock_world_renderer_passes_t::render_ssao(ctx, snapshot, prep_data, frame_index, global_cbv_index);
				return;
			}

			const u32 reflection_task_index = ssao_active ? 4 : 3;

			SFG_ASSERT(reflection_allocation != nullptr && task_index == reflection_task_index);

			stock_world_renderer_passes_t::render_probe(reflection_context.get_command_buffer_graphics(frame_index),
														ctx,
														snapshot,
														prep_data,
														*reflection_allocation,
														reflection_cull_view_indices,
														reflection_probe->capture_type == stock_world_renderer_reflection_probe_capture_type_e::skybox,
														frame_index,
														global_cbv_index,
														global_layout);
			stock_world_renderer_passes_t::render_prefilter_diffuse_sh(ctx, *reflection_allocation, frame_index, global_cbv_index);
		});

		const gfx_handle_t depth_gbuffer_commands[2] = {cmd_depth, cmd_gbuffer};

		backend.submit_commands(queue_gfx, depth_gbuffer_commands, 2);

		const gfx_handle_t clustered_lighting_semaphore = ctx.get_clustered_lighting_semaphore(frame_index);
		const u64		   clustered_lighting_ready		= ctx.next_clustered_lighting_semaphore_value(frame_index);

		backend.submit_commands(queue_compute, &cmd_clustered_lighting, 1);
		backend.queue_signal(queue_compute, &clustered_lighting_semaphore, &clustered_lighting_ready, 1);

		gfx_handle_t ssao_semaphore = {};
		u64			 gbuffer_ready	= 0;
		u64			 ssao_ready		= 0;

		if (ssao_active)
		{
			ssao_semaphore				= ctx.get_ssao_semaphore(frame_index);
			gbuffer_ready				= ctx.next_ssao_semaphore_value(frame_index);
			ssao_ready					= ctx.next_ssao_semaphore_value(frame_index);
			const gfx_handle_t cmd_ssao = ctx.get_command_buffer_ssao(frame_index);

			backend.queue_signal(queue_gfx, &ssao_semaphore, &gbuffer_ready, 1);
			backend.queue_wait(queue_compute, &ssao_semaphore, &gbuffer_ready, 1);
			backend.submit_commands(queue_compute, &cmd_ssao, 1);
			backend.queue_signal(queue_compute, &ssao_semaphore, &ssao_ready, 1);
		}

		if (!prep_data.shadow_views.empty())
			backend.submit_commands(queue_gfx, &cmd_shadows, 1);

		backend.queue_wait(queue_gfx, &clustered_lighting_semaphore, &clustered_lighting_ready, 1);

		gfx_handle_t reflection_semaphore	 = {};
		u64			 reflection_filter_ready = 0;

		if (reflection_allocation != nullptr)
		{
			const gfx_handle_t cmd_reflection_graphics	= reflection_context.get_command_buffer_graphics(frame_index);
			const gfx_handle_t cmd_reflection_compute	= reflection_context.get_command_buffer_compute(frame_index);
			const u64		   reflection_capture_ready = reflection_context.next_semaphore_value(frame_index);

			reflection_filter_ready = reflection_context.next_semaphore_value(frame_index);
			reflection_semaphore	= reflection_context.get_semaphore(frame_index);

			backend.submit_commands(queue_gfx, &cmd_reflection_graphics, 1);
			backend.queue_signal(queue_gfx, &reflection_semaphore, &reflection_capture_ready, 1);
			backend.queue_wait(queue_compute, &reflection_semaphore, &reflection_capture_ready, 1);
			backend.submit_commands(queue_compute, &cmd_reflection_compute, 1);
			backend.queue_signal(queue_compute, &reflection_semaphore, &reflection_filter_ready, 1);
		}

		{
			render_access_scope_t render_scope = {};

			stock_world_renderer_passes_t::render_lighting(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
			stock_world_renderer_passes_t::render_forward(ctx, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
		}

		if (ssao_active)
			backend.queue_wait(queue_gfx, &ssao_semaphore, &ssao_ready, 1);

		if (reflection_allocation != nullptr)
			backend.queue_wait(queue_gfx, &reflection_semaphore, &reflection_filter_ready, 1);

		const gfx_handle_t lighting_forward_commands[2] = {cmd_lighting, cmd_forward};

		backend.submit_commands(queue_gfx, lighting_forward_commands, 2);

		gfx_handle_t bloom_semaphore = {};
		u64			 lighting_ready	 = 0;
		u64			 bloom_ready	 = 0;

		if (bloom_active)
		{
			bloom_semaphore = ctx.get_bloom_semaphore(frame_index);
			lighting_ready	= ctx.next_bloom_semaphore_value(frame_index);
			bloom_ready		= ctx.next_bloom_semaphore_value(frame_index);

			backend.queue_signal(queue_gfx, &bloom_semaphore, &lighting_ready, 1);
		}

		if (bloom_active)
		{
			stock_world_renderer_passes_t::render_bloom(ctx, snapshot, prep_data, frame_index, global_cbv_index);

			const gfx_handle_t cmd_bloom = ctx.get_command_buffer_bloom(frame_index);

			backend.queue_wait(queue_compute, &bloom_semaphore, &lighting_ready, 1);
			backend.submit_commands(queue_compute, &cmd_bloom, 1);
			backend.queue_signal(queue_compute, &bloom_semaphore, &bloom_ready, 1);
			backend.queue_wait(queue_gfx, &bloom_semaphore, &bloom_ready, 1);
		}

		_visible_draws.assign(prep_data.visible_queue.begin(), prep_data.visible_queue.begin() + prep_data.views[0].visible_range.count);
	}

	void stock_world_renderer_t::render_post_process(u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		stock_world_renderer_passes_t::render_post_process(_render_context, get_render_snapshot(), frame_index, global_cbv_index, global_layout);

		gfx_backend&	   backend = gfx_backend::get();
		const gfx_handle_t command = _render_context.get_command_buffer_post(frame_index);
		backend.submit_commands(backend.get_queue_gfx(), &command, 1);
	}

	void stock_world_renderer_t::render_finalize(u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		stock_world_renderer_passes_t::render_finalize(_render_context, get_render_snapshot(), frame_index, global_cbv_index, global_layout);

		gfx_backend&	   backend = gfx_backend::get();
		const gfx_handle_t command = _render_context.get_command_buffer_finalize(frame_index);
		backend.submit_commands(backend.get_queue_gfx(), &command, 1);
	}

	void stock_world_renderer_t::draw_selection(gfx_handle_t cmd, span_t<const entity_id_t> selected_entities, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		gfx_backend&						   backend		  = gfx_backend::get();
		const stock_world_renderer_snapshot_t& snapshot		  = _snapshot_slots[_consumer_slot];
		const gpu_index_t					   rp_constants[] = {
			_render_context.get_view_render_pass_data_index(frame_index),
			_render_context.get_entity_buffer_index(frame_index),
			_render_context.get_bone_buffer_index(frame_index),
		};

		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = std::size(rp_constants), .param_index = 0});

		const span_t<const stock_world_renderer_queue_item_t> visible_draws	 = {.data = _visible_draws.data(), .size = _visible_draws.size()};
		frame_vector_t<stock_world_renderer_queue_item_t>	  selected_draws = {};
		selected_draws.reserve(visible_draws.size);

		for (const stock_world_renderer_queue_item_t& item : visible_draws)
		{
			const stock_world_renderer_renderable_t& renderable = snapshot.renderables[item.renderable_index];
			const stock_world_renderer_entity_t&	 entity		= snapshot.entities[renderable.entity_index];

			if (std::find(selected_entities.begin(), selected_entities.end(), entity.entity_id) != selected_entities.end())
				selected_draws.push_back(item);
		}

		stock_world_renderer_passes_t::draw_queue(cmd, _render_context, snapshot, {.data = selected_draws.data(), .size = selected_draws.size()}, 0, shader_variant_flags_selection_outline, frame_index);
	}

	void stock_world_renderer_t::draw_entity_ids(gfx_handle_t cmd, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		gfx_backend&						   backend		  = gfx_backend::get();
		const stock_world_renderer_snapshot_t& snapshot		  = _snapshot_slots[_consumer_slot];
		const gpu_index_t					   rp_constants[] = {
			_render_context.get_view_render_pass_data_index(frame_index),
			_render_context.get_entity_buffer_index(frame_index),
			_render_context.get_bone_buffer_index(frame_index),
		};

		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = std::size(rp_constants), .param_index = 0});

		stock_world_renderer_passes_t::draw_queue(cmd, _render_context, snapshot, {.data = _visible_draws.data(), .size = _visible_draws.size()}, stock_world_renderer_pass_flags_id, shader_variant_flags_id_write, frame_index);
	}

}
