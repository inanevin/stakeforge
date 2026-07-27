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

#include "editor_world_util.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/char_util.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/animation/animation_bone.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/resources/physics_collision_mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>

namespace sfg
{
#define EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS		 0.06f
#define EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH		 0.5f
#define EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH	 0.12f
#define EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS	 0.05f
#define EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS		 0.4f
#define EDITOR_CONSTRAINT_GIZMO_CONE_LENGTH			 0.65f
#define EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT		 999999.0f
#define EDITOR_PHYSICS_DEFAULT_CONVEX_RADIUS		 0.05f
#define EDITOR_WORLD_SKELETON_JOINT_RADIUS_RATIO	 0.02f
#define EDITOR_WORLD_SKELETON_AXIS_LENGTH_RATIO		 0.12f
#define EDITOR_WORLD_SKELETON_SLOT_HALF_EXTENT_SCALE 1.5f

	namespace
	{
		void draw_physics_shape(world_debug_draw_t&	 debug_draw,
								physics_shape_type_e shape,
								const vec3f_t&		 half_extent,
								f32					 radius,
								f32					 half_height,
								resource_handle_t	 collision_mesh_handle,
								const vec3f_t&		 position,
								const quat_t&		 rotation,
								const vec3f_t&		 scale,
								const color_t&		 color)
		{
			const vec3f_t abs_scale = vec3f_t::abs(scale);

			switch (shape)
			{
			case physics_shape_type_e::box: {
				const mat4x3_t shape_transform = mat4x3_t::transform(position, rotation, vec3f_t::one);

				debug_draw.draw_box(shape_transform, vec3f_t::max(half_extent * abs_scale, {EDITOR_PHYSICS_DEFAULT_CONVEX_RADIUS, EDITOR_PHYSICS_DEFAULT_CONVEX_RADIUS, EDITOR_PHYSICS_DEFAULT_CONVEX_RADIUS}), color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			}
			case physics_shape_type_e::sphere: {
				const f32 scaled_radius = math::max(radius * math::max(abs_scale.x, math::max(abs_scale.y, abs_scale.z)), 0.001f);

				debug_draw.draw_sphere(position, scaled_radius, color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			}
			case physics_shape_type_e::capsule: {
				const f32 scaled_radius		 = math::max(radius * math::max(abs_scale.x, abs_scale.z), 0.001f);
				const f32 scaled_half_height = math::max(half_height * abs_scale.y, 0.001f);

				debug_draw.draw_capsule(position, scaled_radius, scaled_half_height, rotation.get_up(), color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			}
			case physics_shape_type_e::cylinder: {
				const f32 scaled_radius		 = math::max(radius * math::max(abs_scale.x, abs_scale.z), EDITOR_PHYSICS_DEFAULT_CONVEX_RADIUS);
				const f32 scaled_half_height = math::max(half_height * abs_scale.y, EDITOR_PHYSICS_DEFAULT_CONVEX_RADIUS);

				debug_draw.draw_cylinder(position, scaled_radius, scaled_half_height, rotation.get_up(), color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			}
			case physics_shape_type_e::mesh: {
				const physics_collision_mesh_runtime_t* collision_mesh = resource_manager_t::get().find_runtime<physics_collision_mesh_runtime_t>(collision_mesh_handle);

				if (collision_mesh == nullptr)
					break;

				const chunk_allocator_t& memory	  = resource_manager_t::get().get_memory();
				const vec3f_t*			 vertices = memory.get<vec3f_t>(collision_mesh->vertices);
				const primitive_index*	 indices  = memory.get<primitive_index>(collision_mesh->indices);

				for (u32 index = 0; index < collision_mesh->index_count; index += 3)
				{
					const vec3f_t p0 = position + rotation * (vertices[indices[index]] * scale);
					const vec3f_t p1 = position + rotation * (vertices[indices[index + 1]] * scale);
					const vec3f_t p2 = position + rotation * (vertices[indices[index + 2]] * scale);

					debug_draw.draw_triangle(p0, p1, p2, color);
				}

				break;
			}
			case physics_shape_type_e::compound:
				break;
			}
		}
	}

	void editor_world_util_t::draw_skeletons(world_t& world)
	{
		const ecs_component_table_t&	transform_table					   = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t&	alive_table						   = world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	disabled_table					   = world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t&	skinned_mesh_renderer_table		   = world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
		const ecs_component_table_t&	system_skinned_mesh_renderer_table = world.get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		const ecs_component_table_ref_t table_refs[]					   = {
			transform_table.ref(),
			alive_table.ref(),
			skinned_mesh_renderer_table.ref(),
			system_skinned_mesh_renderer_table.ref(),
			!disabled_table.ref(),
		};

		resource_manager_t&		 resource_manager = resource_manager_t::get();
		chunk_allocator_t&		 resource_memory  = resource_manager.get_memory();
		world_debug_draw_t&		 debug_draw		  = world.get_debug_draw();
		const editor_theme_t&	 theme			  = editor_theme_t::get();
		frame_vector_t<mat4x3_t> joint_transforms = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_system_transform_t&				transform					 = ecs_helpers_t::row_get<component_system_transform_t>(row, 0);
			const component_skinned_mesh_renderer_t&		skinned_mesh_renderer		 = ecs_helpers_t::row_get<component_skinned_mesh_renderer_t>(row, 2);
			const component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer = ecs_helpers_t::row_get<component_system_skinned_mesh_renderer_t>(row, 3);
			const skeleton_runtime_t*						skeleton					 = resource_manager.find_runtime<skeleton_runtime_t>(skinned_mesh_renderer.skeleton);

			if (skeleton == nullptr)
				continue;

			const skeleton_joint_runtime_t*		 joints = resource_memory.get<skeleton_joint_runtime_t>(skeleton->joints);
			const span_t<const animation_bone_t> bones	= world.get_animation_controller().get_bones(system_skinned_mesh_renderer.bones_handle);

			joint_transforms.resize(skeleton->joint_count);

			for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
				joint_transforms[joint_index] = transform.abs_mat * bones.data[joint_index].bone_transform * joints[joint_index].bind_global;

			vec3f_t bounds_min = joint_transforms[0].get_translation();
			vec3f_t bounds_max = bounds_min;

			for (u32 joint_index = 1; joint_index < skeleton->joint_count; ++joint_index)
			{
				const vec3f_t position = joint_transforms[joint_index].get_translation();

				bounds_min = vec3f_t::min(bounds_min, position);
				bounds_max = vec3f_t::max(bounds_max, position);
			}

			const vec3f_t dimensions   = bounds_max - bounds_min;
			const f32	  visual_scale = math::max(math::max(dimensions.x, dimensions.y), math::max(dimensions.z, 1.0f));
			const f32	  joint_radius = visual_scale * EDITOR_WORLD_SKELETON_JOINT_RADIUS_RATIO;
			const f32	  axis_length  = visual_scale * EDITOR_WORLD_SKELETON_AXIS_LENGTH_RATIO;

			for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
			{
				const skeleton_joint_runtime_t& joint			= joints[joint_index];
				const mat4x3_t&					joint_transform = joint_transforms[joint_index];
				const vec3f_t					position		= joint_transform.get_translation();
				const char*						joint_name		= resource_memory.get_text(joint.name);

				if (joint.parent_index != SKELETON_JOINT_NO_PARENT)
				{
					const vec3f_t parent_position = joint_transforms[joint.parent_index].get_translation();

					debug_draw.draw_line(parent_position, position, color_t::white, 2.0f, debug_draw_depth_e::depth_tested);
				}

				debug_draw.draw_sphere(position, joint_radius, color_t::purple, 1.5f, debug_draw_depth_e::depth_tested, 10);
				draw_transform_axes(debug_draw, joint_transform, axis_length, 1.5f, debug_draw_depth_e::depth_tested);
				debug_draw.draw_text_3d(position, joint_name, color_t::white, theme.text_small_px_size, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_center, {0.0f, -4.0f});
			}

			if (skeleton->slot_count != 0)
			{
				const skeleton_slot_runtime_t* slots = resource_memory.get<skeleton_slot_runtime_t>(skeleton->slots);

				for (u32 slot_index = 0; slot_index < skeleton->slot_count; ++slot_index)
				{
					const skeleton_slot_runtime_t& slot = slots[slot_index];

					if (slot.slot_joint_index == SKELETON_JOINT_NO_PARENT)
						continue;

					SFG_ASSERT(slot.slot_joint_index < skeleton->joint_count);

					const mat4x3_t slot_local_transform = mat4x3_t::transform(slot.local_position, slot.local_rotation, vec3f_t::one);
					const mat4x3_t slot_transform		= joint_transforms[slot.slot_joint_index] * slot_local_transform;
					const char*	   slot_name			= resource_memory.get_text(slot.debug_name);

					draw_skeleton_slot(debug_draw, slot_transform, slot_name, joint_radius * EDITOR_WORLD_SKELETON_SLOT_HALF_EXTENT_SCALE, axis_length, theme.text_small_px_size);
				}
			}
		}
	}

	void editor_world_util_t::draw_bounding_boxes(world_t& world, const world_render_snapshot_t& snapshot)
	{
		world_debug_draw_t&	  debug_draw   = world.get_debug_draw();
		const editor_theme_t& theme		   = editor_theme_t::get();
		const color_t		  bounds_color = {theme.color_highlight.x, theme.color_highlight.y, theme.color_highlight.z, theme.color_highlight.w};

		for (const world_renderable_t& renderable : snapshot.renderables)
		{
			const world_render_entity_t& entity		   = snapshot.entities[renderable.entity_index];
			const vec3f_t				 center		   = (renderable.aabb.bounds_min + renderable.aabb.bounds_max) * 0.5f;
			const vec3f_t				 dimensions	   = renderable.aabb.bounds_half_extent * 2.0f;
			const mat4x3_t				 box_transform = entity.transform * mat4x3_t::translation(center);
			const vec3f_t				 text_position = entity.transform * vec3f_t(center.x, renderable.aabb.bounds_max.y, center.z);

			debug_draw.draw_box(box_transform, renderable.aabb.bounds_half_extent, bounds_color, 2.0f, debug_draw_depth_e::always_visible);

			char  dimensions_text[96] = {};
			char* text_cur			  = dimensions_text;
			char* text_end			  = dimensions_text + sizeof(dimensions_text);

			char_util::append_double(text_cur, text_end, dimensions.x, 2);
			char_util::append(text_cur, text_end, " x ");
			char_util::append_double(text_cur, text_end, dimensions.y, 2);
			char_util::append(text_cur, text_end, " x ");
			char_util::append_double(text_cur, text_end, dimensions.z, 2);
			debug_draw.draw_text_3d(text_position, dimensions_text, bounds_color, theme.text_small_px_size, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_center, {0.0f, -4.0f});
		}
	}

	void editor_world_util_t::draw_selection_gizmos(world_t& world, span_t<const entity_id_t> selected_entities, const vec2u16_t& render_resolution)
	{
		world_debug_draw_t&			 debug_draw				= world.get_debug_draw();
		const editor_theme_t&		 theme					= editor_theme_t::get();
		const vec4f_t&				 accent_warn			= theme.color_accent_warn;
		const vec4f_t&				 accent1				= theme.color_accent1;
		const color_t				 debug_color			= {accent_warn.x, accent_warn.y, accent_warn.z, accent_warn.w};
		const color_t				 collider_color			= {accent1.x, accent1.y, accent1.z, accent1.w};
		const ecs_component_table_t& system_transform_table = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& transform_table		= world.get_component_table(type_id_t<component_transform_t>::value);
		const ecs_component_table_t& hierarchy_table		= world.get_component_table(type_id_t<component_hierarchy_t>::value);
		const ecs_component_table_t& camera_table			= world.get_component_table(type_id_t<component_camera_t>::value);
		const ecs_component_table_t& light_table			= world.get_component_table(type_id_t<component_light_t>::value);
		const ecs_component_table_t& reflection_probe_table = world.get_component_table(type_id_t<component_reflection_probe_t>::value);
		const ecs_component_table_t& physical_table			= world.get_component_table(type_id_t<component_physical_t>::value);
		const ecs_component_table_t& compound_shape_table	= world.get_component_table(type_id_t<component_compound_shape_t>::value);
		const ecs_component_table_t& particle_emitter_table = world.get_component_table(type_id_t<component_particle_emitter_t>::value);
		frame_vector_t<entity_id_t>	 compound_parents		= {};
		compound_parents.reserve(selected_entities.size);

		for (size_t i = 0; i < selected_entities.size; ++i)
		{
			const entity_id_t					entity		   = selected_entities.data[i];
			const component_system_transform_t& transform	   = ecs_helpers_t::table_get_as_const<component_system_transform_t>(system_transform_table, entity);
			const component_camera_t*			camera		   = ecs_helpers_t::table_find_as_const<component_camera_t>(camera_table, entity);
			const component_compound_shape_t*	compound_shape = ecs_helpers_t::table_find_as_const<component_compound_shape_t>(compound_shape_table, entity);

			if (compound_shape != nullptr)
			{
				const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, entity);

				if (hierarchy.parent != NULL_ENTITY_ID && std::find(compound_parents.begin(), compound_parents.end(), hierarchy.parent) == compound_parents.end())
					compound_parents.push_back(hierarchy.parent);
			}

			if (camera != nullptr)
			{
				SFG_ASSERT(render_resolution.x != 0 && render_resolution.y != 0);

				debug_draw.draw_frustum(transform.abs_pos,
										transform.abs_rot.get_forward(),
										transform.abs_rot.get_up(),
										camera->fov_degrees,
										static_cast<f32>(render_resolution.x) / static_cast<f32>(render_resolution.y),
										camera->near_plane,
										camera->far_plane,
										debug_color,
										2.0f,
										debug_draw_depth_e::always_visible);
			}

			const component_physical_t* physical = ecs_helpers_t::table_find_as_const<component_physical_t>(physical_table, entity);

			if (physical != nullptr)
			{
				const quat_t  body_rotation = transform.abs_rot * physical->local_rotation;
				const vec3f_t body_position = transform.abs_pos + transform.abs_rot * (physical->local_position * transform.abs_scale);
				draw_physics_shape(debug_draw, physical->shape, physical->half_extent, physical->radius, physical->half_height, physical->collision_mesh, body_position, body_rotation, transform.abs_scale, collider_color);

				if (physical->shape == physics_shape_type_e::compound && std::find(compound_parents.begin(), compound_parents.end(), entity) == compound_parents.end())
					compound_parents.push_back(entity);
			}

			draw_constraint_gizmos(world, entity, transform, debug_draw);

			const component_particle_emitter_t* particle_emitter = ecs_helpers_t::table_find_as_const<component_particle_emitter_t>(particle_emitter_table, entity);

			if (particle_emitter != nullptr)
			{
				const aabb_t* bounds = world.get_particle_simulation().find_bounds(entity);

				if (bounds != nullptr)
					debug_draw.draw_aabb(*bounds, debug_color, 2.0f, debug_draw_depth_e::always_visible);

				switch (particle_emitter->shape)
				{
				case particle_spawn_shape_e::point:
					debug_draw.draw_sphere(transform.abs_pos, 0.05f, debug_color, 2.0f, debug_draw_depth_e::always_visible, 12);
					break;
				case particle_spawn_shape_e::box: {
					const mat4x3_t shape_transform = mat4x3_t::transform(transform.abs_pos, transform.abs_rot, vec3f_t::one);
					const vec3f_t  half_extents	   = vec3f_t::abs(particle_emitter->box_half_extents * transform.abs_scale);

					debug_draw.draw_box(shape_transform, half_extents, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					break;
				}
				case particle_spawn_shape_e::sphere: {
					const f32 radius = particle_emitter->shape_radius * math::max(math::abs(transform.abs_scale.x), math::max(math::abs(transform.abs_scale.y), math::abs(transform.abs_scale.z)));

					debug_draw.draw_sphere(transform.abs_pos, radius, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					break;
				}
				case particle_spawn_shape_e::cone:
					debug_draw.draw_cone(transform.abs_pos,
										 transform.abs_rot.get_forward(),
										 particle_emitter->cone_length * math::abs(transform.abs_scale.z),
										 math::degrees_to_radians(particle_emitter->cone_angle_degrees),
										 debug_color,
										 2.0f,
										 debug_draw_depth_e::always_visible,
										 24);
					break;
				}
			}

			const component_reflection_probe_t* reflection_probe = ecs_helpers_t::table_find_as_const<component_reflection_probe_t>(reflection_probe_table, entity);

			if (reflection_probe != nullptr && !reflection_probe->is_global)
			{
				const mat4x3_t box_transform = mat4x3_t::transform(transform.abs_pos, transform.abs_rot, vec3f_t::one);
				const vec3f_t  half_extents	 = vec3f_t::abs(reflection_probe->extents * transform.abs_scale);

				debug_draw.draw_box(box_transform, half_extents, debug_color, 2.0f, debug_draw_depth_e::always_visible);
			}

			const component_light_t* light = ecs_helpers_t::table_find_as_const<component_light_t>(light_table, entity);

			if (light == nullptr)
				continue;

			const vec3f_t forward = transform.abs_rot.get_forward();

			switch (light->type)
			{
			case light_type_e::directional: {
				const vec3f_t offset = transform.abs_rot.get_right() * 0.25f;

				debug_draw.draw_line(transform.abs_pos - offset, transform.abs_pos - offset + forward * 2.0f, debug_color, 2.0f, debug_draw_depth_e::always_visible);
				debug_draw.draw_line(transform.abs_pos, transform.abs_pos + forward * 2.0f, debug_color, 2.0f, debug_draw_depth_e::always_visible);
				debug_draw.draw_line(transform.abs_pos + offset, transform.abs_pos + offset + forward * 2.0f, debug_color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			}
			case light_type_e::point:
				if (light->range > 0.0f)
					debug_draw.draw_sphere(transform.abs_pos, light->range, debug_color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			case light_type_e::spot:
				if (light->range > 0.0f && light->outer_cone_degrees > 0.0f)
					debug_draw.draw_frustum(transform.abs_pos, forward, transform.abs_rot.get_up(), light->outer_cone_degrees * 2.0f, 1.0f, 0.0f, light->range, debug_color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			case light_type_e::area:
				if (light->area_size.x > 0.0f && light->area_size.y > 0.0f)
					debug_draw.draw_rectangle(transform.abs_pos, transform.abs_rot.get_right(), transform.abs_rot.get_up(), light->area_size, debug_color, 2.0f, debug_draw_depth_e::always_visible);
				break;
			}
		}

		for (entity_id_t parent : compound_parents)
		{
			const component_physical_t* physical = ecs_helpers_t::table_find_as_const<component_physical_t>(physical_table, parent);

			if (physical == nullptr || physical->shape != physics_shape_type_e::compound)
				continue;

			const component_system_transform_t& parent_transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(system_transform_table, parent);
			const component_hierarchy_t&		parent_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, parent);
			const quat_t						body_rotation	 = parent_transform.abs_rot * physical->local_rotation;
			const vec3f_t						body_position	 = parent_transform.abs_pos + parent_transform.abs_rot * (physical->local_position * parent_transform.abs_scale);

			for (entity_id_t child = parent_hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t&	  child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
				const component_compound_shape_t* compound_shape  = ecs_helpers_t::table_find_as_const<component_compound_shape_t>(compound_shape_table, child);

				if (compound_shape != nullptr)
				{
					const component_transform_t& child_transform = ecs_helpers_t::table_get_as_const<component_transform_t>(transform_table, child);
					const vec3f_t				 child_position	 = (child_transform.pos + child_transform.rot * (compound_shape->local_position * child_transform.scale)) * parent_transform.abs_scale;
					const quat_t				 shape_rotation	 = body_rotation * child_transform.rot * compound_shape->local_rotation;
					const vec3f_t				 shape_position	 = body_position + body_rotation * child_position;
					const vec3f_t				 shape_scale	 = parent_transform.abs_scale * child_transform.scale;
					draw_physics_shape(debug_draw, compound_shape->shape, compound_shape->half_extent, compound_shape->radius, compound_shape->half_height, compound_shape->collision_mesh, shape_position, shape_rotation, shape_scale, collider_color);
				}

				child = child_hierarchy.next_sibling;
			}
		}
	}

	void editor_world_util_t::draw_transform_axes(world_debug_draw_t& debug_draw, const mat4x3_t& transform, f32 axis_length, f32 thickness_px, debug_draw_depth_e depth)
	{
		const editor_theme_t& theme			 = editor_theme_t::get();
		const color_t		  axis_colors[3] = {
			{theme.color_accent0.x, theme.color_accent0.y, theme.color_accent0.z, theme.color_accent0.w},
			{theme.color_accent_green.x, theme.color_accent_green.y, theme.color_accent_green.z, theme.color_accent_green.w},
			{theme.color_accent1.x, theme.color_accent1.y, theme.color_accent1.z, theme.color_accent1.w},
		};
		const vec3f_t position = transform.get_translation();

		for (u32 axis_index = 0; axis_index < 3; ++axis_index)
		{
			const vec3f_t axis = transform.get_column(axis_index).normalized();
			debug_draw.draw_line(position, position + axis * axis_length, axis_colors[axis_index], thickness_px, depth);
		}
	}

	void editor_world_util_t::draw_skeleton_slot(world_debug_draw_t& debug_draw, const mat4x3_t& transform, const char* name, f32 half_extent, f32 axis_length, f32 text_size)
	{
		const editor_theme_t& theme		 = editor_theme_t::get();
		const color_t		  slot_color = {theme.color_accent2.x, theme.color_accent2.y, theme.color_accent2.z, theme.color_accent2.w};
		const vec3f_t		  extents(half_extent, half_extent, half_extent);
		const vec3f_t		  position	   = transform.get_translation();
		const char*			  display_name = name[0] == '\0' ? "Unnamed Slot" : name;

		debug_draw.draw_box(transform, extents, slot_color, 2.0f, debug_draw_depth_e::depth_tested);
		draw_transform_axes(debug_draw, transform, axis_length, 1.5f, debug_draw_depth_e::depth_tested);
		debug_draw.draw_text_3d(position, display_name, slot_color, text_size, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_center, {0.0f, -4.0f});
	}

	void editor_world_util_t::draw_constraint_gizmos(world_t& world, entity_id_t entity, const component_system_transform_t& transform, world_debug_draw_t& debug_draw)
	{
		const editor_theme_t&		 theme			   = editor_theme_t::get();
		const vec4f_t&				 accent2		   = theme.color_accent2;
		const vec4f_t&				 accent2_dim	   = theme.color_accent2_dim;
		const vec4f_t&				 accent1		   = theme.color_accent1;
		const vec4f_t&				 highlight		   = theme.color_highlight;
		const color_t				 enabled_color	   = {accent2.x, accent2.y, accent2.z, accent2.w};
		const color_t				 disabled_color	   = {accent2_dim.x, accent2_dim.y, accent2_dim.z, accent2_dim.w};
		const color_t				 secondary_color   = {accent1.x, accent1.y, accent1.z, accent1.w};
		const color_t				 limit_color	   = {highlight.x, highlight.y, highlight.z, highlight.w};
		const ecs_component_table_t& transform_table   = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& fixed_table	   = world.get_component_table(type_id_t<component_fixed_constraint_t>::value);
		const ecs_component_table_t& distance_table	   = world.get_component_table(type_id_t<component_distance_constraint_t>::value);
		const ecs_component_table_t& point_table	   = world.get_component_table(type_id_t<component_point_constraint_t>::value);
		const ecs_component_table_t& hinge_table	   = world.get_component_table(type_id_t<component_hinge_constraint_t>::value);
		const ecs_component_table_t& cone_table		   = world.get_component_table(type_id_t<component_cone_constraint_t>::value);
		const ecs_component_table_t& slider_table	   = world.get_component_table(type_id_t<component_slider_constraint_t>::value);
		const ecs_component_table_t& swing_twist_table = world.get_component_table(type_id_t<component_swing_twist_constraint_t>::value);
		const ecs_component_table_t& six_dof_table	   = world.get_component_table(type_id_t<component_six_dof_constraint_t>::value);
		const ecs_component_table_t& pulley_table	   = world.get_component_table(type_id_t<component_pulley_constraint_t>::value);
		const ecs_component_table_t& vehicle_table	   = world.get_component_table(type_id_t<component_vehicle_constraint_t>::value);

		auto find_target_transform = [&world, &transform_table, entity](entity_guid_t target_guid) -> const component_system_transform_t* {
			if (target_guid == NULL_ENTITY_GUID)
				return nullptr;

			const entity_id_t target_entity = world.find_by_guid(target_guid);

			if (target_entity == NULL_ENTITY_ID || target_entity == entity)
				return nullptr;

			return ecs_helpers_t::table_find_as_const<component_system_transform_t>(transform_table, target_entity);
		};

		auto draw_anchor_pair = [&debug_draw](const vec3f_t& local_point, const vec3f_t& target_point, const color_t& color) {
			debug_draw.draw_sphere(local_point, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible, 12);
			debug_draw.draw_sphere(target_point, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible, 12);
			debug_draw.draw_line(local_point, target_point, color, 2.0f, debug_draw_depth_e::always_visible);
		};

		auto draw_frame = [&debug_draw, &secondary_color, &limit_color](const vec3f_t& point, const quat_t& rotation, const color_t& color) {
			debug_draw.draw_arrow(point, point + rotation.get_right() * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(point, point + rotation.get_up() * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, limit_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(point, point + rotation.get_forward() * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, secondary_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
		};

		auto draw_angular_limit = [&debug_draw](const vec3f_t& point, const vec3f_t& axis, const vec3f_t& reference, f32 min_degrees, f32 max_degrees, const color_t& color) {
			const vec3f_t start_direction = reference.rotate(axis, min_degrees).normalized();
			const vec3f_t end_direction	  = reference.rotate(axis, max_degrees).normalized();
			const f32	  angle_radians	  = math::degrees_to_radians(max_degrees - min_degrees);

			if (math::abs(angle_radians) > MATH_EPS)
				debug_draw.draw_arc(point, axis, start_direction, EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, angle_radians, color, 2.0f, debug_draw_depth_e::always_visible, 24);

			debug_draw.draw_line(point, point + start_direction * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_line(point, point + end_direction * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible);
		};

		if (const component_fixed_constraint_t* component = ecs_helpers_t::table_find_as_const<component_fixed_constraint_t>(fixed_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;
			const quat_t						local_frame		 = transform.abs_rot * component->local_rotation;
			const quat_t						target_frame	 = target_transform != nullptr ? target_transform->abs_rot * component->target_rotation : component->target_rotation;

			draw_anchor_pair(local_point, target_point, color);
			draw_frame(local_point, local_frame, color);
			draw_frame(target_point, target_frame, color);
		}

		if (const component_distance_constraint_t* component = ecs_helpers_t::table_find_as_const<component_distance_constraint_t>(distance_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;

			draw_anchor_pair(local_point, target_point, color);

			if (component->min_distance > 0.0f && component->min_distance < EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT)
				debug_draw.draw_sphere(local_point, component->min_distance, limit_color, 2.0f, debug_draw_depth_e::always_visible, 24);

			if (component->max_distance > 0.0f && component->max_distance < EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT)
				debug_draw.draw_sphere(local_point, component->max_distance, limit_color, 2.0f, debug_draw_depth_e::always_visible, 24);
		}

		if (const component_point_constraint_t* component = ecs_helpers_t::table_find_as_const<component_point_constraint_t>(point_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;

			draw_anchor_pair(local_point, target_point, color);
		}

		if (const component_hinge_constraint_t* component = ecs_helpers_t::table_find_as_const<component_hinge_constraint_t>(hinge_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;
			const vec3f_t						local_axis		 = (transform.abs_rot * component->local_hinge_axis).normalized();
			const vec3f_t						target_axis		 = (target_transform != nullptr ? target_transform->abs_rot * component->target_hinge_axis : component->target_hinge_axis).normalized();
			const vec3f_t						local_normal	 = (transform.abs_rot * component->local_normal_axis).normalized();
			const vec3f_t						target_normal	 = (target_transform != nullptr ? target_transform->abs_rot * component->target_normal_axis : component->target_normal_axis).normalized();

			draw_anchor_pair(local_point, target_point, color);
			debug_draw.draw_arrow(local_point, local_point + local_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(target_point, target_point + target_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, secondary_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(local_point, local_point + local_normal * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, limit_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(target_point, target_point + target_normal * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, limit_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			draw_angular_limit(local_point, local_axis, local_normal, component->limit_min_degrees, component->limit_max_degrees, limit_color);
		}

		if (const component_cone_constraint_t* component = ecs_helpers_t::table_find_as_const<component_cone_constraint_t>(cone_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;
			const vec3f_t						local_axis		 = (transform.abs_rot * component->local_twist_axis).normalized();
			const vec3f_t						target_axis		 = (target_transform != nullptr ? target_transform->abs_rot * component->target_twist_axis : component->target_twist_axis).normalized();
			const f32							half_angle		 = math::degrees_to_radians(component->half_cone_angle_degrees);

			draw_anchor_pair(local_point, target_point, color);
			debug_draw.draw_cone(local_point, local_axis, EDITOR_CONSTRAINT_GIZMO_CONE_LENGTH, half_angle, limit_color, 2.0f, debug_draw_depth_e::always_visible, 24);
			debug_draw.draw_arrow(target_point, target_point + target_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, secondary_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
		}

		if (const component_slider_constraint_t* component = ecs_helpers_t::table_find_as_const<component_slider_constraint_t>(slider_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;
			const vec3f_t						local_axis		 = (transform.abs_rot * component->local_slider_axis).normalized();
			const vec3f_t						target_axis		 = (target_transform != nullptr ? target_transform->abs_rot * component->target_slider_axis : component->target_slider_axis).normalized();
			const vec3f_t						local_normal	 = (transform.abs_rot * component->local_normal_axis).normalized();
			const vec3f_t						target_normal	 = (target_transform != nullptr ? target_transform->abs_rot * component->target_normal_axis : component->target_normal_axis).normalized();

			draw_anchor_pair(local_point, target_point, color);
			debug_draw.draw_arrow(local_point - local_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH,
								  local_point + local_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH,
								  color,
								  EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH,
								  EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS,
								  2.0f,
								  debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(target_point, target_point + target_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, secondary_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_line(local_point, local_point + local_normal * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_line(target_point, target_point + target_normal * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible);

			if (math::abs(component->limit_min) < EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT && math::abs(component->limit_max) < EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT)
			{
				const vec3f_t limit_min = local_point + local_axis * component->limit_min;
				const vec3f_t limit_max = local_point + local_axis * component->limit_max;

				debug_draw.draw_line(limit_min, limit_max, limit_color, 3.0f, debug_draw_depth_e::always_visible);
				debug_draw.draw_sphere(limit_min, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible, 12);
				debug_draw.draw_sphere(limit_max, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible, 12);
			}
		}

		if (const component_swing_twist_constraint_t* component = ecs_helpers_t::table_find_as_const<component_swing_twist_constraint_t>(swing_twist_table, entity))
		{
			const component_system_transform_t* target_transform = find_target_transform(component->target_entity);
			const color_t&						color			 = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		 = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	 = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;
			const vec3f_t						local_axis		 = (transform.abs_rot * component->local_twist_axis).normalized();
			const vec3f_t						target_axis		 = (target_transform != nullptr ? target_transform->abs_rot * component->target_twist_axis : component->target_twist_axis).normalized();
			const vec3f_t						local_plane		 = (transform.abs_rot * component->local_plane_axis).normalized();
			const vec3f_t						target_plane	 = (target_transform != nullptr ? target_transform->abs_rot * component->target_plane_axis : component->target_plane_axis).normalized();
			const vec2f_t						half_angles		 = {math::degrees_to_radians(component->normal_half_cone_angle_degrees), math::degrees_to_radians(component->plane_half_cone_angle_degrees)};

			draw_anchor_pair(local_point, target_point, color);
			debug_draw.draw_cone(local_point, local_axis, local_plane, EDITOR_CONSTRAINT_GIZMO_CONE_LENGTH, half_angles, limit_color, 2.0f, debug_draw_depth_e::always_visible, 24);
			debug_draw.draw_arrow(target_point, target_point + target_axis * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, secondary_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_line(target_point, target_point + target_plane * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible);
			draw_angular_limit(local_point, local_axis, local_plane, component->twist_min_angle_degrees, component->twist_max_angle_degrees, limit_color);
		}

		if (const component_six_dof_constraint_t* component = ecs_helpers_t::table_find_as_const<component_six_dof_constraint_t>(six_dof_table, entity))
		{
			const component_system_transform_t* target_transform   = find_target_transform(component->target_entity);
			const color_t&						color			   = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		   = transform.abs_mat * component->local_point;
			const vec3f_t						target_point	   = target_transform != nullptr ? target_transform->abs_mat * component->target_point : component->target_point;
			const quat_t						local_frame		   = transform.abs_rot * component->local_rotation;
			const quat_t						target_frame	   = target_transform != nullptr ? target_transform->abs_rot * component->target_rotation : component->target_rotation;
			const vec3f_t						axes[3]			   = {local_frame.get_right(), local_frame.get_up(), local_frame.get_forward()};
			const vec3f_t						references[3]	   = {local_frame.get_up(), local_frame.get_forward(), local_frame.get_right()};
			const f32							translation_min[3] = {component->translation_limit_min.x, component->translation_limit_min.y, component->translation_limit_min.z};
			const f32							translation_max[3] = {component->translation_limit_max.x, component->translation_limit_max.y, component->translation_limit_max.z};
			const f32							rotation_min[3]	   = {component->rotation_limit_min_degrees.x, component->rotation_limit_min_degrees.y, component->rotation_limit_min_degrees.z};
			const f32							rotation_max[3]	   = {component->rotation_limit_max_degrees.x, component->rotation_limit_max_degrees.y, component->rotation_limit_max_degrees.z};

			draw_anchor_pair(local_point, target_point, color);
			draw_frame(local_point, local_frame, color);
			draw_frame(target_point, target_frame, color);

			for (u32 axis_index = 0; axis_index < 3; ++axis_index)
			{
				if (math::abs(translation_min[axis_index]) < EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT && math::abs(translation_max[axis_index]) < EDITOR_CONSTRAINT_GIZMO_UNBOUNDED_LIMIT)
				{
					const vec3f_t limit_min = local_point + axes[axis_index] * translation_min[axis_index];
					const vec3f_t limit_max = local_point + axes[axis_index] * translation_max[axis_index];

					debug_draw.draw_line(limit_min, limit_max, limit_color, 3.0f, debug_draw_depth_e::always_visible);
				}

				draw_angular_limit(local_point, axes[axis_index], references[axis_index], rotation_min[axis_index], rotation_max[axis_index], limit_color);
			}
		}

		if (const component_pulley_constraint_t* component = ecs_helpers_t::table_find_as_const<component_pulley_constraint_t>(pulley_table, entity))
		{
			const component_system_transform_t* target_transform   = find_target_transform(component->target_entity);
			const color_t&						color			   = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t						local_point		   = transform.abs_mat * component->local_body_point;
			const vec3f_t						target_point	   = target_transform != nullptr ? target_transform->abs_mat * component->target_body_point : component->target_body_point;
			const vec3f_t						fixed_point		   = component->fixed_point;
			const vec3f_t						target_fixed_point = component->target_fixed_point;

			debug_draw.draw_sphere(local_point, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible, 12);
			debug_draw.draw_sphere(target_point, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible, 12);
			debug_draw.draw_sphere(fixed_point, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible, 12);
			debug_draw.draw_sphere(target_fixed_point, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, limit_color, 2.0f, debug_draw_depth_e::always_visible, 12);
			debug_draw.draw_line(local_point, fixed_point, color, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_line(fixed_point, target_fixed_point, secondary_color, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_line(target_fixed_point, target_point, color, 2.0f, debug_draw_depth_e::always_visible);
		}

		if (const component_vehicle_constraint_t* component = ecs_helpers_t::table_find_as_const<component_vehicle_constraint_t>(vehicle_table, entity))
		{
			const color_t& color   = component->enabled != 0 ? enabled_color : disabled_color;
			const vec3f_t  up	   = (transform.abs_rot * component->up).normalized();
			const vec3f_t  forward = (transform.abs_rot * component->forward).normalized();

			debug_draw.draw_arrow(transform.abs_pos, transform.abs_pos + up * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, limit_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
			debug_draw.draw_arrow(transform.abs_pos, transform.abs_pos + forward * EDITOR_CONSTRAINT_GIZMO_ARROW_LENGTH, color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);

			if (component->max_pitch_roll_angle_degrees > 0.0f && component->max_pitch_roll_angle_degrees < 180.0f)
				debug_draw.draw_cone(transform.abs_pos, up, EDITOR_CONSTRAINT_GIZMO_CONE_LENGTH, math::degrees_to_radians(component->max_pitch_roll_angle_degrees), limit_color, 2.0f, debug_draw_depth_e::always_visible, 24);

			for (const physics_vehicle_wheel_t& wheel : component->wheels)
			{
				const vec3f_t wheel_position	   = transform.abs_mat * wheel.position;
				const vec3f_t suspension_direction = (transform.abs_rot * wheel.suspension_direction).normalized();
				const vec3f_t wheel_up			   = (transform.abs_rot * wheel.wheel_up).normalized();
				const vec3f_t wheel_forward		   = (transform.abs_rot * wheel.wheel_forward).normalized();
				const vec3f_t steering_axis		   = (transform.abs_rot * wheel.steering_axis).normalized();
				const vec3f_t wheel_right		   = vec3f_t::cross(wheel_up, wheel_forward).normalized();
				const vec3f_t suspension_min	   = wheel_position + suspension_direction * wheel.suspension_min_length;
				const vec3f_t suspension_max	   = wheel_position + suspension_direction * wheel.suspension_max_length;
				const vec3f_t wheel_center		   = (suspension_min + suspension_max) * 0.5f;

				debug_draw.draw_line(suspension_min, suspension_max, color, 2.0f, debug_draw_depth_e::always_visible);
				debug_draw.draw_sphere(wheel_position, EDITOR_CONSTRAINT_GIZMO_ANCHOR_RADIUS, color, 2.0f, debug_draw_depth_e::always_visible, 12);
				debug_draw.draw_cylinder(wheel_center, wheel.radius, wheel.width * 0.5f, wheel_right, secondary_color, 2.0f, debug_draw_depth_e::always_visible, 20);
				debug_draw.draw_arrow(
					wheel_position, wheel_position + steering_axis * EDITOR_CONSTRAINT_GIZMO_LIMIT_RADIUS, limit_color, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_LENGTH, EDITOR_CONSTRAINT_GIZMO_ARROW_HEAD_RADIUS, 2.0f, debug_draw_depth_e::always_visible);
				draw_angular_limit(wheel_position, steering_axis, wheel_forward, -wheel.max_steer_angle_degrees, wheel.max_steer_angle_degrees, limit_color);
			}
		}
	}
}
