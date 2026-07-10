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
#include <sfg/memory/memory.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/resource_type.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>

namespace sfg
{
#define EDITOR_THUMBNAIL_RENDER_SIZE 256
#define EDITOR_THUMBNAIL_PIXEL_BYTES 4
#define EDITOR_THUMBNAIL_CAMERA_FOV	 50

	namespace
	{
		struct global_buffer_data_t
		{
			f32 delta_time	 = 0.0f;
			f32 elapsed_time = 0.0f;
		};

		void place_camera_for_aabb(world_t& world, entity_id_t camera_entity, const aabb_t& aabb)
		{
			const vec3f_t& half = aabb.bounds_half_extent;
			const f32	   rad	= math::max(half.x, math::max(half.y, half.z));
			const f32	   dist = rad / (math::sin(DEG_2_RAD * EDITOR_THUMBNAIL_CAMERA_FOV / 2.0f));
			world.set_entity_pos_local(camera_entity, vec3f_t(0, 0, dist));
		}
	}

	void editor_thumbnail_render_service_t::init()
	{
		SFG_ASSERT(!_initialized);

		const world_init_config_t world_config{
			.render_resolution		 = vec2u16_t(EDITOR_THUMBNAIL_RENDER_SIZE, EDITOR_THUMBNAIL_RENDER_SIZE),
			.component_table_reserve = 32,
			.free_list_reserve		 = 16,
			.used_resource_reserve	 = 32,
			.text_allocation_reserve = 32,
		};

		gfx_backend& backend	= gfx_backend::get();
		_semaphore_frame.sem	= backend.create_semaphore();
		_semaphore_transfer.sem = backend.create_semaphore();
		_semaphore_readback.sem = backend.create_semaphore();
		_cmd_prepare			= backend.create_command_buffer({
			.type		= command_type::graphics,
			.debug_name = "thumb_prep",
		});
		_cmd_transit			= backend.create_command_buffer({
			.type		= command_type::graphics,
			.debug_name = "thumb_transit",
		});
		_cmd_transfer			= backend.create_command_buffer({
			.type		= command_type::transfer,
			.debug_name = "thumb_xfer",
		});
		_cmd_resolve			= backend.create_command_buffer({
			.type		= command_type::graphics,
			.debug_name = "thumb_resolve",
		});

		resource_desc_t global_desc = {};
		global_desc.size			= sizeof(global_buffer_data_t);
		global_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		global_desc.set_name("thumbnail_global");
		_global_buffer = backend.create_resource(global_desc);
		backend.map_resource(_global_buffer, _mapped_global);
		_global_index = backend.get_resource_gpu_index(_global_buffer);

		texture_desc_t thumbnail_texture_desc = {};
		thumbnail_texture_desc.texture_format = format_e::r8g8b8a8_srgb;
		thumbnail_texture_desc.size			  = vec2u16_t(EDITOR_THUMBNAIL_RENDER_SIZE, EDITOR_THUMBNAIL_RENDER_SIZE);
		thumbnail_texture_desc.flags		  = texture_flags::tf_render_target | texture_flags::tf_transfer_source | texture_flags::tf_is_2d;
		thumbnail_texture_desc.view_count	  = 1;
		thumbnail_texture_desc.views[0]		  = {.type = view_type::render_target};
		thumbnail_texture_desc.set_name("thumbnail_output");
		_thumbnail_texture = backend.create_texture(thumbnail_texture_desc);

		resource_desc_t readback_desc = {};
		readback_desc.size			  = EDITOR_THUMBNAIL_RENDER_SIZE * EDITOR_THUMBNAIL_RENDER_SIZE * EDITOR_THUMBNAIL_PIXEL_BYTES;
		readback_desc.flags			  = resource_flags::rf_readback;
		readback_desc.set_name("thumbnail_readback");
		_thumbnail_readback = backend.create_resource(readback_desc);
		backend.map_resource(_thumbnail_readback, _mapped_readback);
		_readback_pixels.reserve(readback_desc.size);

		_world.init(world_config);
		_render_context.init(world_config.render_resolution);
		const shader_internals_t* shader = resource_manager_t::get().find_internals<shader_internals_t>("editor/resource_pack/shaders/thumbnail_capture_copy.hlsl"_hs);
		SFG_ASSERT(shader != nullptr);
		_thumbnail_shader = render_resources_t::get().get_shader_hw(shader->psos[0]);
		_snapshot.reserve(64);
		setup_base_world();
		_initialized = true;
	}

	void editor_thumbnail_render_service_t::uninit()
	{
		SFG_ASSERT(_initialized);
		gfx_backend& backend = gfx_backend::get();
		backend.wait_semaphore(_semaphore_frame.sem, _semaphore_frame.value);
		backend.wait_semaphore(_semaphore_transfer.sem, _semaphore_transfer.value);
		backend.wait_semaphore(_semaphore_readback.sem, _semaphore_readback.value);

		_render_context.uninit();
		_world.unload_all_used_resources();
		_world.uninit();
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
		_snapshot			= {};
		_semaphore_frame	= {};
		_semaphore_transfer = {};
		_semaphore_readback = {};
		_cmd_prepare		= {};
		_cmd_transfer		= {};
		_cmd_transit		= {};
		_cmd_resolve		= {};
		_global_buffer		= {};
		_thumbnail_texture	= {};
		_thumbnail_readback = {};
		_thumbnail_shader	= {};
		_readback_pixels.resize(0);
		_global_index		= NULL_GPU_INDEX;
		_mapped_global		= nullptr;
		_mapped_readback	= nullptr;
		_environment_entity = NULL_ENTITY_ID;
		_camera_entity		= NULL_ENTITY_ID;
		_display_entity		= NULL_ENTITY_ID;
		_initialized		= false;
	}

	bool editor_thumbnail_render_service_t::render_thumbnail(const editor_asset_t& asset)
	{
		SFG_ASSERT(_initialized);

		switch (asset.asset_type)
		{
		case editor_asset_type_e::prefab:
			clear_world_for_setup();
			setup_camera_for_asset();
			setup_world_for_prefab(asset);
			break;
		case editor_asset_type_e::material:
			clear_world_for_setup();
			setup_camera_for_asset();
			setup_world_for_material(asset);
			break;
		case editor_asset_type_e::mesh:
			clear_world_for_setup();
			setup_camera_for_asset();
			setup_world_for_mesh(asset);
			break;
		case editor_asset_type_e::hdr_skybox:
			clear_world_for_setup();
			setup_camera_for_asset();
			setup_world_for_skybox(asset);
			break;
		case editor_asset_type_e::animation:
			clear_world_for_setup();
			setup_camera_for_asset();
			setup_world_for_animation(asset);
			break;
		default:
			SFG_ASSERT(false);
			return false;
		}

		produce_snapshot();
		render_world();
		resolve_world_to_thumbnail_texture();
		readback_thumbnail_texture();
		_world.unload_all_used_resources();
		return save_rendered_thumbnail(asset);
	}

	void editor_thumbnail_render_service_t::setup_base_world()
	{
		_environment_entity		   = _world.create_entity("thumbnail_environment");
		component_skybox_t& skybox = ecs_helpers_t::table_add_or_get_as<component_skybox_t>(_world.get_component_table(type_id_t<component_skybox_t>::value), _environment_entity);
		skybox.skybox_asset		   = DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		_world.add_resource(resource_type_e::hdr_skybox, DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID);
		_world.scan_for_resources(_environment_entity, true);

		_camera_entity			   = _world.create_entity("thumbnail_camera");
		component_camera_t& camera = ecs_helpers_t::table_add_or_get_as<component_camera_t>(_world.get_component_table(type_id_t<component_camera_t>::value), _camera_entity);
		camera.priority			   = -1;
		camera.fov_degrees		   = EDITOR_THUMBNAIL_CAMERA_FOV;
		camera.near_plane		   = 0.01f;
		camera.far_plane		   = 250.0f;
		setup_camera_for_asset();
	}

	void editor_thumbnail_render_service_t::clear_world_for_setup()
	{
		if (_display_entity != NULL_ENTITY_ID)
		{
			_world.destroy_entity_tree(_display_entity);
			_display_entity = NULL_ENTITY_ID;
		}

		component_skybox_t& skybox = ecs_helpers_t::table_get_as<component_skybox_t>(_world.get_component_table(type_id_t<component_skybox_t>::value), _environment_entity);
		skybox.skybox_asset		   = DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		_world.scan_for_resources(_environment_entity, true);
	}

	void editor_thumbnail_render_service_t::setup_camera_for_asset()
	{
		_world.set_entity_pos_local(_camera_entity, vec3f_t(0.0f, 0.0f, 2.0f));
		_world.set_entity_rot_local(_camera_entity, quat_t::identity);
	}

	void editor_thumbnail_render_service_t::setup_world_for_prefab(const editor_asset_t& asset)
	{
		_display_entity = _world.spawn_prefab(asset.guid, {});
		SFG_ASSERT(_display_entity != NULL_ENTITY_ID);

		_world.update_world_transforms(false);

		const ecs_component_table_t& hierarchy_table	 = _world.get_component_table(type_id_t<component_hierarchy_t>::value);
		const ecs_component_table_t& mesh_renderer_table = _world.get_component_table(type_id_t<component_mesh_renderer_t>::value);
		const ecs_component_table_t& transform_table	 = _world.get_component_table(type_id_t<component_system_transform_t>::value);

		aabb_t prefab_aabb = {};
		bool   has_aabb	   = false;

		const auto add_point = [&](const vec3f_t& point) {
			if (!has_aabb)
			{
				prefab_aabb = aabb_t(point, point);
				has_aabb	= true;
				return;
			}

			prefab_aabb.bounds_min = vec3f_t::min(prefab_aabb.bounds_min, point);
			prefab_aabb.bounds_max = vec3f_t::max(prefab_aabb.bounds_max, point);
		};

		const auto add_mesh_renderer = [&](entity_id_t entity) {
			const component_mesh_renderer_t* mesh_renderer = ecs_helpers_t::table_find_as_const<component_mesh_renderer_t>(mesh_renderer_table, entity);
			if (mesh_renderer == nullptr || mesh_renderer->mesh == NULL_RESOURCE_HANDLE)
				return;

			const mesh_internals_t* runtime = resource_manager_t::get().find_internals<mesh_internals_t>(mesh_renderer->mesh);
			SFG_ASSERT(runtime != nullptr);

			const component_system_transform_t& transform  = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);
			const vec3f_t&						bounds_min = runtime->local_bounds.bounds_min;
			const vec3f_t&						bounds_max = runtime->local_bounds.bounds_max;

			add_point(transform.abs_mat * vec3f_t(bounds_min.x, bounds_min.y, bounds_min.z));
			add_point(transform.abs_mat * vec3f_t(bounds_min.x, bounds_min.y, bounds_max.z));
			add_point(transform.abs_mat * vec3f_t(bounds_min.x, bounds_max.y, bounds_min.z));
			add_point(transform.abs_mat * vec3f_t(bounds_min.x, bounds_max.y, bounds_max.z));
			add_point(transform.abs_mat * vec3f_t(bounds_max.x, bounds_min.y, bounds_min.z));
			add_point(transform.abs_mat * vec3f_t(bounds_max.x, bounds_min.y, bounds_max.z));
			add_point(transform.abs_mat * vec3f_t(bounds_max.x, bounds_max.y, bounds_min.z));
			add_point(transform.abs_mat * vec3f_t(bounds_max.x, bounds_max.y, bounds_max.z));
		};

		const auto scan = [&](const auto& self, entity_id_t current) -> void {
			add_mesh_renderer(current);

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, current);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
				self(self, child);
				child = next_child;
			}
		};

		scan(scan, _display_entity);

		if (has_aabb)
		{
			prefab_aabb.update_half_extents();
			place_camera_for_aabb(_world, _camera_entity, prefab_aabb);
		}
	}

	void editor_thumbnail_render_service_t::setup_world_for_material(const editor_asset_t& asset)
	{
		_display_entity							 = _world.create_entity("thumbnail_material");
		component_mesh_renderer_t& mesh_renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(_world.get_component_table(type_id_t<component_mesh_renderer_t>::value), _display_entity);
		mesh_renderer.mesh						 = DEFAULT_MESH_SPHERE_GUID;
		mesh_renderer.materials.push_back(asset.guid);
		_world.scan_for_resources(_display_entity, true);

		_world.set_entity_pos_local(_camera_entity, vec3f_t(0, 0, 1.5f));
	}

	void editor_thumbnail_render_service_t::setup_world_for_mesh(const editor_asset_t& asset)
	{
		_display_entity							 = _world.create_entity("thumbnail_mesh");
		component_mesh_renderer_t& mesh_renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(_world.get_component_table(type_id_t<component_mesh_renderer_t>::value), _display_entity);
		mesh_renderer.mesh						 = asset.guid;
		for (size_t i = 0; i < decltype(mesh_renderer.materials)::capacity; ++i)
			mesh_renderer.materials.push_back(DEFAULT_GBUFFER_MATERIAL_ASSET_GUID);
		_world.scan_for_resources(_display_entity, true);

		const mesh_internals_t* runtime = resource_manager_t::get().find_internals<mesh_internals_t>(asset.guid);
		SFG_ASSERT(runtime != nullptr);

		place_camera_for_aabb(_world, _camera_entity, runtime->local_bounds);
	}

	void editor_thumbnail_render_service_t::setup_world_for_skybox(const editor_asset_t& asset)
	{
		component_skybox_t& skybox = ecs_helpers_t::table_get_as<component_skybox_t>(_world.get_component_table(type_id_t<component_skybox_t>::value), _environment_entity);
		skybox.skybox_asset		   = asset.guid;
		_world.scan_for_resources(_environment_entity, true);
	}

	void editor_thumbnail_render_service_t::setup_world_for_animation(const editor_asset_t& asset)
	{
	}

	void editor_thumbnail_render_service_t::produce_snapshot()
	{
		_world.update_world_transforms(false);
		world_snapshot_producer_t::produce(_world, _snapshot);
	}

	void editor_thumbnail_render_service_t::render_world()
	{
		gfx_backend& backend = gfx_backend::get();
		backend.wait_semaphore(_semaphore_frame.sem, _semaphore_frame.value);

		render_resources_t& render_resources = render_resources_t::get();
		render_resources.drain_requests();

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
		world_rendering_t::render_world(_render_context, _snapshot, 1.0f, 0, _global_index, render_globals_t::get_global_bind_layout());

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

		const gfx_handle_t world_texture	 = _render_context.get_world_texture(0);
		barrier_t		   begin_barriers[2] = {};
		u16				   begin_count		 = 0;

		u32 state = backend.get_texture_state(world_texture);
		if (state != resource_state_ps_resource)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_ps_resource,
				.texture_t	 = world_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		state = backend.get_texture_state(_thumbnail_texture);
		if (state != resource_state_render_target)
		{
			begin_barriers[begin_count++] = {
				.from_states = state,
				.to_states	 = resource_state_render_target,
				.texture_t	 = _thumbnail_texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
		}

		if (begin_count > 0)
			backend.cmd_barrier(cmd, {.barriers = begin_barriers, .barrier_count = begin_count});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = _thumbnail_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = EDITOR_THUMBNAIL_RENDER_SIZE, .height = EDITOR_THUMBNAIL_RENDER_SIZE});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = EDITOR_THUMBNAIL_RENDER_SIZE, .height = EDITOR_THUMBNAIL_RENDER_SIZE});
		backend.cmd_bind_layout(cmd, {.layout = render_globals_t::get_global_bind_layout()});
		const gpu_index_t source_texture = _render_context.get_world_texture_index(0);
		backend.cmd_bind_constants(cmd, {.data = &source_texture, .offset = constant_rp0, .count = 1, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = _thumbnail_shader});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});
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
											   .size		= vec2u_t(EDITOR_THUMBNAIL_RENDER_SIZE, EDITOR_THUMBNAIL_RENDER_SIZE),
											   .bpp			= EDITOR_THUMBNAIL_PIXEL_BYTES,
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
		_readback_pixels.resize(EDITOR_THUMBNAIL_RENDER_SIZE * EDITOR_THUMBNAIL_RENDER_SIZE * EDITOR_THUMBNAIL_PIXEL_BYTES);
		SFG_MEMCPY(_readback_pixels.data(), _mapped_readback, _readback_pixels.size());
	}

	bool editor_thumbnail_render_service_t::save_rendered_thumbnail(const editor_asset_t& asset)
	{
		return editor_asset_thumbnailer_t::save_thumbnail(asset, {.data = _readback_pixels.data(), .size = _readback_pixels.size()});
	}

#undef EDITOR_THUMBNAIL_RENDER_SIZE
#undef EDITOR_THUMBNAIL_PIXEL_BYTES
}
