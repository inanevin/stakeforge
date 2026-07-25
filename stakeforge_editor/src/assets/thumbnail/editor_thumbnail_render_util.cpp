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

#include "assets/thumbnail/editor_thumbnail_render_util.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_builtin_types.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/render/world_debug_draw_snapshot.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/physics_collision_mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/resource_type.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

namespace sfg
{
#define EDITOR_THUMBNAIL_CAMERA_FOV 50

	void editor_thumbnail_render_util_t::place_camera_for_aabb(world_t& world, entity_id_t camera_entity, const aabb_t& aabb)
	{
		const vec3f_t& half = aabb.bounds_half_extent;
		const f32	   rad	= math::max(half.x, math::max(half.y, half.z));
		const f32	   dist = rad / math::sin(DEG_2_RAD * EDITOR_THUMBNAIL_CAMERA_FOV / 2.0f);
		world.set_entity_pos_local(camera_entity, vec3f_t(0, 0, dist));
	}

	void editor_thumbnail_render_util_t::setup_base_world(editor_thumbnail_world_t& thumbnail_world)
	{
		world_t& world = *thumbnail_world.world;

		thumbnail_world.environment_entity	 = world.create_entity("thumbnail_environment");
		component_environment_t& environment = ecs_helpers_t::table_add_or_get_as<component_environment_t>(world.get_component_table(type_id_t<component_environment_t>::value), thumbnail_world.environment_entity);
		environment.skybox_material			 = DEFAULT_CUBE_SKYBOX_MATERIAL_ASSET_GUID;
		environment.intensity				 = 0.25f;

		world.add_resource(resource_type_e::material, DEFAULT_CUBE_SKYBOX_MATERIAL_ASSET_GUID);
		world.scan_for_resources(thumbnail_world.environment_entity, true);

		thumbnail_world.camera_entity = world.create_entity("thumbnail_camera");
		component_camera_t& camera	  = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value), thumbnail_world.camera_entity);
		camera.priority				  = -1;
		camera.fov_degrees			  = EDITOR_THUMBNAIL_CAMERA_FOV;
		camera.near_plane			  = 0.01f;
		camera.far_plane			  = 250.0f;

		ecs_helpers_t::table_add_or_get_as<component_post_process_t>(world.get_component_table(type_id_t<component_post_process_t>::value), thumbnail_world.camera_entity);
		setup_camera_for_asset(thumbnail_world);
	}

	void editor_thumbnail_render_util_t::setup_camera_for_asset(editor_thumbnail_world_t& thumbnail_world)
	{
		world_t& world = *thumbnail_world.world;
		world.set_entity_pos_local(thumbnail_world.camera_entity, vec3f_t(0.0f, 0.0f, 2.0f));
		world.set_entity_rot_local(thumbnail_world.camera_entity, quat_t::identity);
	}

	void editor_thumbnail_render_util_t::setup_world_for_asset(editor_thumbnail_world_t& thumbnail_world, editor_asset_type_e asset_type, resource_handle_t asset_guid)
	{
		setup_base_world(thumbnail_world);

		switch (asset_type)
		{
		case editor_asset_type_e::prefab:
			setup_world_for_prefab(thumbnail_world, asset_guid);
			break;
		case editor_asset_type_e::material:
			setup_world_for_material(thumbnail_world, asset_guid);
			break;
		case editor_asset_type_e::mesh:
			setup_world_for_mesh(thumbnail_world, asset_guid);
			break;
		case editor_asset_type_e::animation:
			setup_world_for_animation(thumbnail_world, asset_guid);
			break;
		case editor_asset_type_e::physics_collision_mesh:
			setup_world_for_collision_mesh(thumbnail_world, asset_guid);
			break;
		default:
			SFG_ASSERT(false);
			break;
		}
	}

	void editor_thumbnail_render_util_t::setup_world_for_prefab(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid)
	{
		world_t& world				   = *thumbnail_world.world;
		thumbnail_world.display_entity = world_cooker_t::spawn_prefab(world, asset_guid, {});

		world.update_world_transforms(false);

		const ecs_component_table_t& hierarchy_table	 = world.get_component_table(type_id_t<component_hierarchy_t>::value);
		const ecs_component_table_t& mesh_renderer_table = world.get_component_table(type_id_t<component_mesh_renderer_t>::value);
		const ecs_component_table_t& transform_table	 = world.get_component_table(type_id_t<component_system_transform_t>::value);

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

			const mesh_internals_t*				runtime	   = resource_manager_t::get().find_internals<mesh_internals_t>(mesh_renderer->mesh);
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

		scan(scan, thumbnail_world.display_entity);

		if (has_aabb)
		{
			prefab_aabb.update_half_extents();
			place_camera_for_aabb(world, thumbnail_world.camera_entity, prefab_aabb);
		}
	}

	void editor_thumbnail_render_util_t::setup_world_for_material(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid)
	{
		world_t&			  world = *thumbnail_world.world;
		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(asset_guid);
		SFG_ASSERT(asset != nullptr);

		if (asset->sub_type == static_cast<u8>(editor_material_type_e::skybox))
		{
			component_environment_t& environment = ecs_helpers_t::table_get_as<component_environment_t>(world.get_component_table(type_id_t<component_environment_t>::value), thumbnail_world.environment_entity);
			environment.skybox_material			 = asset_guid;
			environment.intensity				 = 1.0f;

			world.scan_for_resources(thumbnail_world.environment_entity, true);
			return;
		}

		thumbnail_world.display_entity = world.create_entity("thumbnail_material");

		component_mesh_renderer_t& mesh_renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(world.get_component_table(type_id_t<component_mesh_renderer_t>::value), thumbnail_world.display_entity);
		mesh_renderer.mesh						 = DEFAULT_MESH_SPHERE_GUID;
		mesh_renderer.materials.push_back(asset_guid);

		world.scan_for_resources(thumbnail_world.display_entity, true);
		world.set_entity_pos_local(thumbnail_world.camera_entity, vec3f_t(0, 0, 1.5f));
	}

	void editor_thumbnail_render_util_t::setup_world_for_mesh(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid)
	{

		world_t& world				   = *thumbnail_world.world;
		thumbnail_world.display_entity = world.create_entity("thumbnail_mesh");

		component_mesh_renderer_t& mesh_renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(world.get_component_table(type_id_t<component_mesh_renderer_t>::value), thumbnail_world.display_entity);
		mesh_renderer.mesh						 = asset_guid;

		for (size_t i = 0; i < decltype(mesh_renderer.materials)::capacity; ++i)
			mesh_renderer.materials.push_back(DEFAULT_OPAQUE_MATERIAL_ASSET_GUID);

		world.scan_for_resources(thumbnail_world.display_entity, true);

		const mesh_internals_t* runtime = resource_manager_t::get().find_internals<mesh_internals_t>(asset_guid);
		place_camera_for_aabb(world, thumbnail_world.camera_entity, runtime->local_bounds);
	}

	void editor_thumbnail_render_util_t::setup_world_for_animation(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid)
	{
	}

	void editor_thumbnail_render_util_t::setup_world_for_collision_mesh(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid)
	{
		world_t& world				   = *thumbnail_world.world;
		thumbnail_world.collision_mesh = asset_guid;

		world.add_resource(resource_type_e::physics_collision_mesh, asset_guid);
		world.load_all_used_resources();

		const physics_collision_mesh_runtime_t* collision_mesh = resource_manager_t::get().find_runtime<physics_collision_mesh_runtime_t>(asset_guid);
		const vec3f_t*							vertices	   = resource_manager_t::get().get_memory().get<vec3f_t>(collision_mesh->vertices);
		vec3f_t									bounds_min	   = vertices[0];
		vec3f_t									bounds_max	   = vertices[0];

		for (u32 i = 1; i < collision_mesh->vertex_count; ++i)
		{
			bounds_min = vec3f_t::min(bounds_min, vertices[i]);
			bounds_max = vec3f_t::max(bounds_max, vertices[i]);
		}

		thumbnail_world.collision_mesh_center = (bounds_min + bounds_max) * 0.5f;
		place_camera_for_aabb(world, thumbnail_world.camera_entity, aabb_t(bounds_min, bounds_max));
	}

	void editor_thumbnail_render_util_t::collect_texture_resources(editor_thumbnail_world_t& thumbnail_world)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();
		thumbnail_world.texture_resources.resize(0);

		const auto append_texture = [&](resource_handle_t handle) {
			if (handle == NULL_RESOURCE_HANDLE)
				return;

			auto it = std::find(thumbnail_world.texture_resources.begin(), thumbnail_world.texture_resources.end(), handle);
			if (it == thumbnail_world.texture_resources.end())
				thumbnail_world.texture_resources.push_back(handle);
		};

		const auto scan_dependencies = [&](const auto& self, resource_handle_t handle) -> void {
			const resource_entry_t* entry = resource_manager.find_entry(handle);
			if (entry == nullptr)
				return;

			if (entry->type == resource_type_e::texture)
				append_texture(handle);

			if (entry->dependency_count == 0)
				return;

			const resource_dependency_t* deps = resource_manager.get_memory().get<resource_dependency_t>(entry->dependencies);
			for (u32 i = 0; i < entry->dependency_count; ++i)
			{
				if (deps[i].type == resource_type_e::texture)
					append_texture(deps[i].handle);
				else
					self(self, deps[i].handle);
			}
		};

		for (const world_t::world_resource_t& world_resource : thumbnail_world.world->get_used_resources())
		{
			if (world_resource.type == resource_type_e::texture)
				append_texture(world_resource.handle);
			else
				scan_dependencies(scan_dependencies, world_resource.handle);
		}
	}

	void editor_thumbnail_render_util_t::write_collision_mesh_debug_draw(const editor_thumbnail_world_t& thumbnail_world, world_debug_draw_snapshot_t& debug_draw)
	{
		if (thumbnail_world.collision_mesh == NULL_RESOURCE_HANDLE)
			return;

		const physics_collision_mesh_runtime_t* collision_mesh = resource_manager_t::get().find_runtime<physics_collision_mesh_runtime_t>(thumbnail_world.collision_mesh);
		const chunk_allocator_t&				aux			   = resource_manager_t::get().get_memory();

		const vec3f_t*		   vertices	   = aux.get<vec3f_t>(collision_mesh->vertices);
		const primitive_index* indices	   = aux.get<primitive_index>(collision_mesh->indices);
		const u32			   index_count = math::min(collision_mesh->index_count, static_cast<u32>(DEBUG_TRIANGLE_INDEX_MAX / 3 * 3));

		debug_draw.triangle_vertices.resize(index_count);
		debug_draw.triangle_indices.resize(index_count);

		for (u32 i = 0; i < index_count; ++i)
		{
			debug_draw.triangle_vertices[i] = {
				.position = vertices[indices[i]] - thumbnail_world.collision_mesh_center,
				.color	  = editor_theme_t::get().color_accent1,
			};
			debug_draw.triangle_indices[i] = i;
		}
	}

	bool editor_thumbnail_render_util_t::is_ready_to_render(const editor_thumbnail_world_t& thumbnail_world)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		for (resource_handle_t texture : thumbnail_world.texture_resources)
		{
			const texture_runtime_t* runtime = resource_manager.find_runtime<texture_runtime_t>(texture);
			if (runtime == nullptr || runtime->residency != texture_residency_e::resident)
				return false;
		}

		return true;
	}

#undef EDITOR_THUMBNAIL_CAMERA_FOV
}
