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

#include "assets/thumbnail/editor_thumbnail_render_service.hpp"
#include "assets/editor_asset.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"

#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>

namespace sfg
{
	namespace
	{
		struct global_buffer_data_t
		{
			f32 delta_time	 = 0.0f;
			f32 elapsed_time = 0.0f;
		};
	}

	editor_thumbnail_render_service_config_t editor_thumbnail_render_service_config_t::make_default()
	{
		return {
			.world =
				{
					.particle_simulation =
						{
							.emitter_initial_capacity			   = 0,
							.particle_per_emitter_initial_capacity = 0,
							.particle_max_count					   = 0,
						},
					.render_resolution				   = vec2u16_t(256, 256),
					.render_entity_max_count		   = 1024,
					.render_sprite_max_count		   = 16,
					.render_particle_max_count		   = 0,
					.render_bone_max_count			   = 128,
					.render_bone_initial_capacity	   = 128,
					.animation_graph_budget_bytes	   = 64 * 1024,
					.component_table_initial_capacity  = 32,
					.entity_free_list_initial_capacity = 16,
					.used_resource_initial_capacity	   = 32,
					.text_allocation_initial_capacity  = 32,
					.text_budget_bytes				   = 4096,
					.physics_enabled				   = false,
				},
			.render_context =
				{
					.size				 = vec2u16_t(256, 256),
					.entity_max			 = 1024,
					.sprite_max			 = 16,
					.particle_max		 = 0,
					.bone_max			 = 128,
					.light_max			 = 4,
					.triangle_vertex_max = editor_thumbnail_render_util_t::DEBUG_TRIANGLE_VERTEX_MAX,
					.triangle_index_max	 = editor_thumbnail_render_util_t::DEBUG_TRIANGLE_INDEX_MAX,
					.enable_ssao		 = 0,
					.enable_bloom		 = 1,
				},
			.snapshot =
				{
					.material_initial_capacity		  = 32,
					.entity_initial_capacity		  = 10,
					.renderable_initial_capacity	  = 80,
					.draw_initial_capacity			  = 64,
					.sprite_initial_capacity		  = 16,
					.particle_draw_initial_capacity	  = 0,
					.particle_initial_capacity		  = 0,
					.bone_initial_capacity			  = 128,
					.triangle_vertex_initial_capacity = editor_thumbnail_render_util_t::DEBUG_TRIANGLE_VERTEX_MAX,
					.triangle_index_initial_capacity  = editor_thumbnail_render_util_t::DEBUG_TRIANGLE_INDEX_MAX,
				},
			.render_prep =
				{
					.view_initial_capacity				= 1,
					.depth_queue_initial_capacity		= 80,
					.opaque_queue_initial_capacity		= 80,
					.transparent_queue_initial_capacity = 80,
					.shadow_queue_initial_capacity		= 0,
					.visible_queue_initial_capacity		= 10,
					.shadow_view_initial_capacity		= 0,
				},
			.render_resolution				   = vec2u16_t(256, 256),
			.world_pool_initial_capacity	   = 16,
			.world_pool_max_count			   = 64,
			.request_initial_capacity		   = 256,
			.texture_resource_initial_capacity = 32,
			.pixel_bytes					   = 4,
		};
	}

	void editor_thumbnail_render_service_t::init()
	{
		init(editor_thumbnail_render_service_config_t::make_default());
	}

	void editor_thumbnail_render_service_t::init(const editor_thumbnail_render_service_config_t& config)
	{
		SFG_ASSERT(config.world_pool_initial_capacity != 0);
		SFG_ASSERT(config.world_pool_initial_capacity <= config.world_pool_max_count);
		SFG_ASSERT(config.render_resolution == config.world.render_resolution);
		SFG_ASSERT(config.render_resolution == config.render_context.size);
		SFG_ASSERT(config.world.render_entity_max_count == config.render_context.entity_max);
		SFG_ASSERT(config.world.render_sprite_max_count == config.render_context.sprite_max);
		SFG_ASSERT(config.world.render_particle_max_count == config.render_context.particle_max);
		SFG_ASSERT(config.world.render_bone_max_count == config.render_context.bone_max);
		SFG_ASSERT(config.snapshot.entity_initial_capacity <= config.render_context.entity_max);
		SFG_ASSERT(config.snapshot.sprite_initial_capacity <= config.render_context.sprite_max);
		SFG_ASSERT(config.snapshot.particle_initial_capacity <= config.render_context.particle_max);
		SFG_ASSERT(config.snapshot.bone_initial_capacity <= config.render_context.bone_max);
		SFG_ASSERT(config.snapshot.light_initial_capacity <= config.render_context.light_max);
		SFG_ASSERT(config.snapshot.reflection_probe_initial_capacity <= config.render_context.reflection_probe_max);
		SFG_ASSERT(config.pixel_bytes != 0);

		_config		  = config;
		_world_config = config.world;

		gfx_backend& backend	= gfx_backend::get();
		_semaphore_frame.sem	= backend.create_semaphore();
		_semaphore_transfer.sem = backend.create_semaphore();
		_semaphore_readback.sem = backend.create_semaphore();

		_cmd_prepare = backend.create_command_buffer({
			.type		= command_type::graphics,
			.debug_name = "thumb_prep",
		});

		_cmd_transit = backend.create_command_buffer({
			.type		= command_type::graphics,
			.debug_name = "thumb_transit",
		});

		_cmd_transfer = backend.create_command_buffer({
			.type		= command_type::transfer,
			.debug_name = "thumb_xfer",
		});

		_cmd_resolve = backend.create_command_buffer({
			.type		= command_type::graphics,
			.debug_name = "thumb_resolve",
		});

		resource_desc_t global_desc = {};
		global_desc.size			= sizeof(global_buffer_data_t);
		global_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		global_desc.set_name("thumbnail_global");

		_global_buffer = backend.create_resource(global_desc);
		_global_index  = backend.get_resource_gpu_index(_global_buffer);
		backend.map_resource(_global_buffer, _mapped_global);

		texture_desc_t thumbnail_texture_desc = {};
		thumbnail_texture_desc.texture_format = format_e::r8g8b8a8_srgb;
		thumbnail_texture_desc.initial_states = resource_state_copy_source;
		thumbnail_texture_desc.size			  = config.render_resolution;
		thumbnail_texture_desc.flags		  = texture_flags::tf_render_target | texture_flags::tf_transfer_source | texture_flags::tf_is_2d;
		thumbnail_texture_desc.view_count	  = 1;
		thumbnail_texture_desc.views[0]		  = {.type = view_type::render_target};
		thumbnail_texture_desc.set_name("thumbnail_output");
		_thumbnail_texture = backend.create_texture(thumbnail_texture_desc);

		resource_desc_t readback_desc = {};
		readback_desc.size			  = static_cast<u32>(config.render_resolution.x) * config.render_resolution.y * config.pixel_bytes;
		readback_desc.flags			  = resource_flags::rf_readback;
		readback_desc.set_name("thumbnail_readback");
		_thumbnail_readback = backend.create_resource(readback_desc);
		_readback_pixels.reserve(readback_desc.size);
		backend.map_resource(_thumbnail_readback, _mapped_readback);

		_render_context.init(config.render_context);

		const shader_internals_t* shader				= resource_manager_t::get().find_internals<shader_internals_t>("editor/resource_pack/shaders/thumbnail_capture_copy.hlsl"_hs);
		const shader_internals_t* debug_triangle_shader = resource_manager_t::get().find_internals<shader_internals_t>("editor/resource_pack/shaders/editor_world_physics_debug.hlsl"_hs);
		_thumbnail_shader								= render_resources_t::get().get_shader_hw(shader->psos[0]);
		_debug_triangle_shader							= render_resources_t::get().get_shader_hw(debug_triangle_shader->psos[0]);
		_snapshot.reserve(config.snapshot);
		_prep_data.reserve(config.render_prep);
		_worlds.reserve(config.world_pool_max_count);
		_available_worlds.reserve(config.world_pool_max_count);
		_pending_renders.reserve(config.world_pool_max_count);
		_requests.reserve(config.request_initial_capacity);

		grow_world_pool(config.world_pool_initial_capacity);
	}

	void editor_thumbnail_render_service_t::uninit()
	{
		gfx_backend& backend = gfx_backend::get();

		backend.wait_semaphore(_semaphore_frame.sem, _semaphore_frame.value);
		backend.wait_semaphore(_semaphore_transfer.sem, _semaphore_transfer.value);
		backend.wait_semaphore(_semaphore_readback.sem, _semaphore_readback.value);

		for (pending_render_t& pending_render : _pending_renders)
			release_world(pending_render.world_index);

		_pending_renders.resize(0);
		_available_worlds.resize(0);

		for (editor_thumbnail_world_t& thumbnail_world : _worlds)
		{
			thumbnail_world.world->unload_all_used_resources();
			thumbnail_world.world->uninit();
			delete thumbnail_world.world;
			thumbnail_world.world = nullptr;
		}

		_worlds.resize(0);
		_render_context.uninit();
		backend.destroy_texture(_thumbnail_texture);
		backend.destroy_resource(_thumbnail_readback);
		backend.destroy_resource(_global_buffer);
		backend.destroy_command_buffer(_cmd_prepare);
		backend.destroy_command_buffer(_cmd_transfer);
		backend.destroy_command_buffer(_cmd_transit);
		backend.destroy_command_buffer(_cmd_resolve);
		backend.destroy_semaphore(_semaphore_frame.sem);
		backend.destroy_semaphore(_semaphore_transfer.sem);
		backend.destroy_semaphore(_semaphore_readback.sem);
		_snapshot			   = {};
		_semaphore_frame	   = {};
		_semaphore_transfer	   = {};
		_semaphore_readback	   = {};
		_cmd_prepare		   = {};
		_cmd_transfer		   = {};
		_cmd_transit		   = {};
		_cmd_resolve		   = {};
		_global_buffer		   = {};
		_thumbnail_texture	   = {};
		_thumbnail_readback	   = {};
		_thumbnail_shader	   = {};
		_debug_triangle_shader = {};
		_readback_pixels.resize(0);
		_requests.resize(0);
		_config			 = {};
		_world_config	 = {};
		_global_index	 = NULL_GPU_INDEX;
		_mapped_global	 = nullptr;
		_mapped_readback = nullptr;
	}

	void editor_thumbnail_render_service_t::request_thumbnail(const editor_asset_t& asset)
	{
		auto pending_it = std::find_if(_pending_renders.begin(), _pending_renders.end(), [&](const pending_render_t& pending_render) { return pending_render.request.thumbnail_guid == asset.thumbnail_guid; });

		if (pending_it != _pending_renders.end())
			return;

		auto it = std::find_if(_requests.begin(), _requests.end(), [&](const thumbnail_request_t& request) { return request.thumbnail_guid == asset.thumbnail_guid; });

		if (it != _requests.end())
		{
			*it = {.asset_guid = asset.guid, .thumbnail_guid = asset.thumbnail_guid, .asset_type = asset.asset_type};
			return;
		}

		it = std::find_if(_requests.begin(), _requests.end(), [&](const thumbnail_request_t& request) { return request.asset_guid == asset.guid; });

		if (it != _requests.end())
		{
			*it = {.asset_guid = asset.guid, .thumbnail_guid = asset.thumbnail_guid, .asset_type = asset.asset_type};
			return;
		}

		_requests.push_back({.asset_guid = asset.guid, .thumbnail_guid = asset.thumbnail_guid, .asset_type = asset.asset_type});
	}

	void editor_thumbnail_render_service_t::cancel_asset(sid_t asset_guid)
	{
		const auto request_end = std::remove_if(_requests.begin(), _requests.end(), [&](const thumbnail_request_t& request) { return request.asset_guid == asset_guid; });
		_requests.erase(request_end, _requests.end());

		for (pending_render_t& pending_render : _pending_renders)
		{
			const bool is_cancelled = pending_render.request.asset_guid == asset_guid;

			if (!is_cancelled)
			{
				const auto duplicate =
					std::find_if(_requests.begin(), _requests.end(), [&](const thumbnail_request_t& request) { return request.asset_guid == pending_render.request.asset_guid || request.thumbnail_guid == pending_render.request.thumbnail_guid; });

				if (duplicate == _requests.end())
					_requests.push_back(pending_render.request);
			}

			release_world(pending_render.world_index);
		}

		_pending_renders.resize(0);
	}

	void editor_thumbnail_render_service_t::tick()
	{
		const size_t available_world_count = _config.world_pool_max_count - _pending_renders.size();
		const size_t prepare_count		   = math::min(_requests.size(), available_world_count);

		for (size_t request_index = 0; request_index < prepare_count; ++request_index)
			prepare_request(_requests[request_index]);

		_requests.erase(_requests.begin(), _requests.begin() + prepare_count);

		for (size_t i = 0; i < _pending_renders.size();)
		{
			pending_render_t& pending_render = _pending_renders[i];

			if (!editor_thumbnail_render_util_t::is_ready_to_render(_worlds[pending_render.world_index]))
			{
				i++;
				continue;
			}

			editor_thumbnail_world_t& thumbnail_world = _worlds[pending_render.world_index];
			produce_snapshot(thumbnail_world);
			render_world();
			resolve_world_to_thumbnail_texture();
			readback_thumbnail_texture();

			save_rendered_thumbnail(pending_render.request);

			release_world(pending_render.world_index);
			_pending_renders[i] = _pending_renders.back();
			_pending_renders.pop_back();
		}
	}

	bool editor_thumbnail_render_service_t::has_pending_work() const
	{
		return !_requests.empty() || !_pending_renders.empty();
	}

	u32 editor_thumbnail_render_service_t::acquire_world()
	{
		if (_available_worlds.empty())
		{
			SFG_ASSERT(_worlds.size() < _config.world_pool_max_count);

			const u32 remaining = _config.world_pool_max_count - static_cast<u32>(_worlds.size());
			const u32 count		= math::min(static_cast<u32>(_worlds.size()), remaining);
			grow_world_pool(count);
		}

		const u32 world_index = _available_worlds.back();
		_available_worlds.pop_back();
		return world_index;
	}

	void editor_thumbnail_render_service_t::release_world(u32 world_index)
	{
		editor_thumbnail_world_t& thumbnail_world = _worlds[world_index];
		thumbnail_world.world->unload_all_used_resources();
		thumbnail_world.world->uninit();
		thumbnail_world.world->init(_world_config);
		thumbnail_world.texture_resources.resize(0);
		thumbnail_world.collision_mesh		  = NULL_RESOURCE_HANDLE;
		thumbnail_world.collision_mesh_center = vec3f_t::zero;
		thumbnail_world.environment_entity	  = NULL_ENTITY_ID;
		thumbnail_world.camera_entity		  = NULL_ENTITY_ID;
		thumbnail_world.display_entity		  = NULL_ENTITY_ID;
		_available_worlds.push_back(world_index);
	}

	void editor_thumbnail_render_service_t::grow_world_pool(u32 count)
	{
		const u32 start = static_cast<u32>(_worlds.size());
		_worlds.resize(_worlds.size() + count);
		_available_worlds.reserve(_available_worlds.size() + count);

		for (u32 i = 0; i < count; ++i)
		{
			const u32				  world_index	  = start + i;
			editor_thumbnail_world_t& thumbnail_world = _worlds[world_index];
			thumbnail_world.world					  = new world_t();
			thumbnail_world.world->init(_world_config);
			thumbnail_world.texture_resources.reserve(_config.texture_resource_initial_capacity);
			_available_worlds.push_back(world_index);
		}
	}

	void editor_thumbnail_render_service_t::prepare_request(const thumbnail_request_t& request)
	{
		const u32				  world_index	  = acquire_world();
		editor_thumbnail_world_t& thumbnail_world = _worlds[world_index];
		editor_thumbnail_render_util_t::setup_world_for_asset(thumbnail_world, request.asset_type, request.asset_guid);
		thumbnail_world.world->load_all_used_resources();

		pending_render_t& pending_render = _pending_renders.emplace_back();
		pending_render.request			 = request;
		pending_render.world_index		 = world_index;
		editor_thumbnail_render_util_t::collect_texture_resources(thumbnail_world);
	}

	void editor_thumbnail_render_service_t::produce_snapshot(editor_thumbnail_world_t& thumbnail_world)
	{
		thumbnail_world.world->update_world_transforms(false);
		thumbnail_world.world->tick_animation_prep(0.0f);
		thumbnail_world.world->tick_animation_logic(0.0f);

		world_snapshot_producer_t::produce(*thumbnail_world.world, _snapshot, engine_runtime_t::get().get_project_settings());
		editor_thumbnail_render_util_t::write_collision_mesh_debug_draw(thumbnail_world, _snapshot.debug_draw);
	}

	void editor_thumbnail_render_service_t::render_world()
	{
		gfx_backend& backend = gfx_backend::get();
		backend.wait_semaphore(_semaphore_frame.sem, _semaphore_frame.value);

		render_resources_t& render_resources = render_resources_t::get();
		render_resources.drain_requests();
		render_resources.flush_texture_region_data_uploads(0);
		render_resources.flush_material_parameter_updates(0);

		texture_queue_t& texture_queue = render_resources.get_texture_upload_queue();
		texture_queue.submit({
			.queue_gfx		= backend.get_queue_gfx(),
			.queue_transfer = backend.get_queue_transfer(),
			.cmd_prepare	= _cmd_prepare,
			.cmd_transfer	= _cmd_transfer,
			.cmd_transit	= _cmd_transit,
			.semaphore		= &_semaphore_transfer,
		});

		const global_buffer_data_t global_data = {};
		SFG_MEMCPY(_mapped_global, &global_data, sizeof(global_buffer_data_t));

		_prep_data.reset();
		world_rendering_t::render_world(_render_context, _snapshot, _prep_data, 1.0f, 0, _global_index, render_globals_t::get_global_bind_layout());

		_semaphore_frame.value++;
		const gfx_handle_t queue_gfx = backend.get_queue_gfx();
		backend.queue_signal(queue_gfx, &_semaphore_frame.sem, &_semaphore_frame.value, 1);

		render_resources.drain_destroy_requests();
	}

	void editor_thumbnail_render_service_t::resolve_world_to_thumbnail_texture()
	{
		gfx_backend&	   backend	 = gfx_backend::get();
		const gfx_handle_t cmd		 = _cmd_resolve;
		const gfx_handle_t queue_gfx = backend.get_queue_gfx();

		backend.queue_wait(queue_gfx, &_semaphore_frame.sem, &_semaphore_frame.value, 1);
		backend.reset_command_buffer(cmd);

		const barrier_t begin_barrier = {
			.from_states = resource_state_copy_source,
			.to_states	 = resource_state_render_target,
			.texture_t	 = _thumbnail_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = _thumbnail_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = _config.render_resolution.x, .height = _config.render_resolution.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = _config.render_resolution.x, .height = _config.render_resolution.y});
		backend.cmd_bind_layout(cmd, {.layout = render_globals_t::get_global_bind_layout()});

		const gpu_index_t source_texture = _render_context.get_world_texture_index(0);

		backend.cmd_bind_constants(cmd, {.data = &source_texture, .offset = constant_rp0, .count = 1, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = _thumbnail_shader});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		if (!_snapshot.debug_draw.triangle_indices.empty())
		{
			const gpu_index_t rp_constant = _render_context.get_view_render_pass_data_index(0);

			backend.cmd_bind_constants(cmd, {.data = &rp_constant, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = _debug_triangle_shader});
			backend.cmd_bind_vertex_buffers(cmd, {.buffer = _render_context.get_debug_triangle_vertex_buffer(0), .slot = 0, .vertex_size = static_cast<u16>(sizeof(vertex_debug_triangle_t)), .offset = 0});
			backend.cmd_bind_index_buffers(cmd, {.buffer = _render_context.get_debug_triangle_index_buffer(0), .offset = 0, .index_size = static_cast<u8>(sizeof(primitive_index))});
			backend.cmd_draw_indexed_instanced(cmd,
											   {
												   .index_count_per_instance = static_cast<u32>(_snapshot.debug_draw.triangle_indices.size()),
												   .instance_count			 = 1,
												   .start_index_location	 = 0,
												   .base_vertex_location	 = 0,
												   .start_instance_location	 = 0,
											   });
		}
		backend.cmd_end_render_pass(cmd, {});

		const barrier_t readback_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_copy_source,
			.texture_t	 = _thumbnail_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};
		backend.cmd_barrier(cmd, {.barriers = &readback_barrier, .barrier_count = 1});
		backend.cmd_copy_texture_to_buffer(cmd,
										   {
											   .dest_buffer = _thumbnail_readback,
											   .src_texture = _thumbnail_texture,
											   .size		= vec2u_t(_config.render_resolution.x, _config.render_resolution.y),
											   .bpp			= _config.pixel_bytes,
										   });
		backend.close_command_buffer(cmd);
		backend.submit_commands(queue_gfx, &cmd, 1);
		_semaphore_readback.value++;
		backend.queue_signal(queue_gfx, &_semaphore_readback.sem, &_semaphore_readback.value, 1);
	}

	void editor_thumbnail_render_service_t::readback_thumbnail_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		backend.wait_semaphore(_semaphore_readback.sem, _semaphore_readback.value);
		_readback_pixels.resize(static_cast<size_t>(_config.render_resolution.x) * _config.render_resolution.y * _config.pixel_bytes);
		SFG_MEMCPY(_readback_pixels.data(), _mapped_readback, _readback_pixels.size());
	}

	bool editor_thumbnail_render_service_t::save_rendered_thumbnail(const thumbnail_request_t& request)
	{
		editor_asset_t asset = {};
		asset.guid			 = request.asset_guid;
		asset.thumbnail_guid = request.thumbnail_guid;
		asset.asset_type	 = request.asset_type;
		return editor_asset_thumbnailer_t::save_thumbnail(asset, {.data = _readback_pixels.data(), .size = _readback_pixels.size()}, nullptr);
	}

}
