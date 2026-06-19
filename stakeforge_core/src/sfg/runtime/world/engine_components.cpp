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

#include "engine_components.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	namespace
	{
		void register_type_if_missing(const reflected_type_desc_t& desc)
		{
			reflection_registry_t& registry = reflection_registry_t::get();
			if (registry.find_type(desc.type_id) == nullptr)
				registry.register_type(desc);
		}

		void register_component_hierarchy_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "first_child", .display_name = "First Child", .type = reflected_value_type_e::entity_id, .offset = offsetof(component_hierarchy_t, first_child), .size = sizeof(entity_id_t)},
				{.name = "parent", .display_name = "Parent", .type = reflected_value_type_e::entity_id, .offset = offsetof(component_hierarchy_t, parent), .size = sizeof(entity_id_t)},
				{.name = "next_sibling", .display_name = "Next Sibling", .type = reflected_value_type_e::entity_id, .offset = offsetof(component_hierarchy_t, next_sibling), .size = sizeof(entity_id_t)},
				{.name = "prev_sibling", .display_name = "Previous Sibling", .type = reflected_value_type_e::entity_id, .offset = offsetof(component_hierarchy_t, prev_sibling), .size = sizeof(entity_id_t)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_hierarchy",
				.display_name = "Hierarchy",
				.category	  = "component",
				.type_id	  = component_hierarchy_t::TYPE_ID,
				.size		  = sizeof(component_hierarchy_t),
				.alignment	  = alignof(component_hierarchy_t),
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}

		void register_component_transform_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "pos", .display_name = "Position", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_transform_t, pos), .size = sizeof(vec3f_t)},
				{.name = "rot", .display_name = "Rotation", .type = reflected_value_type_e::quat, .offset = offsetof(component_transform_t, rot), .size = sizeof(quat_t)},
				{.name = "scale", .display_name = "Scale", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_transform_t, scale), .size = sizeof(vec3f_t)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_transform",
				.display_name = "Transform",
				.category	  = "component",
				.type_id	  = component_transform_t::TYPE_ID,
				.size		  = sizeof(component_transform_t),
				.alignment	  = alignof(component_transform_t),
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}

		void register_component_name_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "text_index", .display_name = "Name", .type = reflected_value_type_e::text_id, .offset = offsetof(component_name_t, text_index), .size = sizeof(u32)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_name",
				.display_name = "Name",
				.category	  = "component",
				.type_id	  = component_name_t::TYPE_ID,
				.size		  = sizeof(component_name_t),
				.alignment	  = alignof(component_name_t),
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}

		void register_component_mesh_renderer_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "mesh", .display_name = "Mesh", .type = reflected_value_type_e::mesh_handle, .offset = offsetof(component_mesh_renderer_t, mesh), .size = sizeof(resource_handle_t)},
				{.name = "material", .display_name = "Material", .type = reflected_value_type_e::material_handle, .offset = offsetof(component_mesh_renderer_t, material), .size = sizeof(resource_handle_t)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_mesh_renderer",
				.display_name = "Mesh Renderer",
				.category	  = "component",
				.type_id	  = component_mesh_renderer_t::TYPE_ID,
				.size		  = sizeof(component_mesh_renderer_t),
				.alignment	  = alignof(component_mesh_renderer_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_render_object_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "render_id", .display_name = "Render ID", .type = reflected_value_type_e::u32, .offset = offsetof(component_render_object_t, render_id), .size = sizeof(u32)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_render_object",
				.display_name = "Render Object",
				.category	  = "component",
				.type_id	  = component_render_object_t::TYPE_ID,
				.size		  = sizeof(component_render_object_t),
				.alignment	  = alignof(component_render_object_t),
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}

		void register_component_camera_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "fov_degrees", .display_name = "Field of View", .type = reflected_value_type_e::f32, .offset = offsetof(component_camera_t, fov_degrees), .size = sizeof(f32)},
				{.name = "near_plane", .display_name = "Near Plane", .type = reflected_value_type_e::f32, .offset = offsetof(component_camera_t, near_plane), .size = sizeof(f32)},
				{.name = "far_plane", .display_name = "Far Plane", .type = reflected_value_type_e::f32, .offset = offsetof(component_camera_t, far_plane), .size = sizeof(f32)},
				{.name = "priority", .display_name = "Priority", .type = reflected_value_type_e::i8, .offset = offsetof(component_camera_t, priority), .size = sizeof(i8)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_camera",
				.display_name = "Camera",
				.category	  = "component",
				.type_id	  = component_camera_t::TYPE_ID,
				.size		  = sizeof(component_camera_t),
				.alignment	  = alignof(component_camera_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_skybox_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{.name = "skybox_asset", .display_name = "Skybox", .type = reflected_value_type_e::hdr_skybox_handle, .offset = offsetof(component_skybox_t, skybox_asset), .size = sizeof(resource_handle_t)},
				{.name = "intensity", .display_name = "Intensity", .type = reflected_value_type_e::f32, .offset = offsetof(component_skybox_t, intensity), .size = sizeof(f32)},
				{.name = "exposure", .display_name = "Exposure", .type = reflected_value_type_e::f32, .offset = offsetof(component_skybox_t, exposure), .size = sizeof(f32)},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_skybox",
				.display_name = "Skybox",
				.category	  = "component",
				.type_id	  = component_skybox_t::TYPE_ID,
				.size		  = sizeof(component_skybox_t),
				.alignment	  = alignof(component_skybox_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_disabled_reflection()
		{
			register_type_if_missing({
				.name		  = "component_disabled",
				.display_name = "Disabled",
				.category	  = "component",
				.type_id	  = component_disabled_t::TYPE_ID,
				.size		  = 0,
				.alignment	  = 1,
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_no_serialize_reflection()
		{
			register_type_if_missing({
				.name		  = "component_no_serialize",
				.display_name = "No Serialize",
				.category	  = "component",
				.type_id	  = component_no_serialize_t::TYPE_ID,
				.size		  = 0,
				.alignment	  = 1,
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}

		void register_component_alive_reflection()
		{
			register_type_if_missing({
				.name		  = "component_alive",
				.display_name = "Alive",
				.category	  = "component",
				.type_id	  = component_alive_t::TYPE_ID,
				.size		  = 0,
				.alignment	  = 1,
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}
	}

	engine_component_reflection_t::engine_component_reflection_t()
	{
		register_component_hierarchy_reflection();
		register_component_transform_reflection();
		register_component_name_reflection();
		register_component_mesh_renderer_reflection();
		register_component_render_object_reflection();
		register_component_camera_reflection();
		register_component_skybox_reflection();
		register_component_alive_reflection();
		register_component_disabled_reflection();
		register_component_no_serialize_reflection();
	}
}
