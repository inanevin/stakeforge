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

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/resources/resource_type.hpp>

#include <cstddef>
#include <memory>

namespace sfg
{
	SFG_DEFINE_TYPE_ID(debug_widgets_enum);
	SFG_DEFINE_TYPE_ID(debug_widgets_enum2);

	namespace
	{
		u32 get_entity_tag_bitmask_option_count(void* user_data)
		{
			return static_cast<u32>(engine_runtime_t::get().get_project_settings().tags.size());
		}

		bitmask_option_t get_entity_tag_bitmask_option(u32 index, void* user_data)
		{
			const string_t& tag = engine_runtime_t::get().get_project_settings().tags[index];
			return {
				.name  = tag.empty() ? "Unnamed Tag" : tag.c_str(),
				.value = 1ull << index,
			};
		}

		const char* build_entity_tag_bitmask_title(u64 value, void* user_data)
		{
			if (value == 0)
				return "None";

			static thread_local string_t title;
			title.resize(0);
			const vector_t<string_t>& tags = engine_runtime_t::get().get_project_settings().tags;
			for (u32 i = 0; i < tags.size(); ++i)
			{
				if ((value & (1ull << i)) == 0)
					continue;

				if (!title.empty())
					title += " | ";
				if (tags[i].empty())
					title += "Unnamed Tag";
				else
					title += tags[i];
			}
			return title.empty() ? "Unknown" : title.c_str();
		}

		void register_component_hierarchy_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_hierarchy",
				.display_name	 = "Hierarchy",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_hierarchy_t*>(ptr), component_hierarchy_t{}); },
				.fields =
					{
						{.name = "first_child", .display_name = "First Child", .offset = offsetof(component_hierarchy_t, first_child), .size = sizeof(entity_id_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
						{.name = "parent", .display_name = "Parent", .offset = offsetof(component_hierarchy_t, parent), .size = sizeof(entity_id_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
						{.name = "next_sibling", .display_name = "Next Sibling", .offset = offsetof(component_hierarchy_t, next_sibling), .size = sizeof(entity_id_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
						{.name = "prev_sibling", .display_name = "Previous Sibling", .offset = offsetof(component_hierarchy_t, prev_sibling), .size = sizeof(entity_id_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u32},
					},
				.type_id   = type_id_t<component_hierarchy_t>::value,
				.size	   = sizeof(component_hierarchy_t),
				.alignment = alignof(component_hierarchy_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
			});
		}

		void register_component_guid_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_guid",
				.display_name	 = "GUID",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_guid_t*>(ptr), component_guid_t{}); },
				.fields =
					{
						{.name		   = "guid",
						 .display_name = "GUID",
						 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID,
						 .offset	   = offsetof(component_guid_t, guid),
						 .size		   = sizeof(entity_guid_t),
						 .flags		   = reflected_field_flag_no_ui,
						 .type		   = reflected_value_type_e::u64},
					},
				.type_id   = type_id_t<component_guid_t>::value,
				.size	   = sizeof(component_guid_t),
				.alignment = alignof(component_guid_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
			});
		}

		void register_component_transform_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_transform",
				.display_name	 = "Transform",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_transform_t*>(ptr), component_transform_t{}); },
				.fields =
					{
						{.name = "pos", .display_name = "Position", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_transform_t, pos), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
						{.name = "rot", .display_name = "Rotation", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(component_transform_t, rot), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
						{.name = "scale", .display_name = "Scale", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_transform_t, scale), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					},
				.type_id   = type_id_t<component_transform_t>::value,
				.size	   = sizeof(component_transform_t),
				.alignment = alignof(component_transform_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
			});
		}

		void register_component_name_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_name",
				.display_name	 = "Name",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_name_t*>(ptr), component_name_t{}); },
				.fields =
					{
						{.name = "text", .display_name = "Text", .offset = offsetof(component_name_t, text), .size = sizeof(component_name_t::text), .type = reflected_value_type_e::char_array},
					},
				.type_id   = type_id_t<component_name_t>::value,
				.size	   = sizeof(component_name_t),
				.alignment = alignof(component_name_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
			});
		}

		void register_component_mesh_renderer_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_mesh_renderer",
				.display_name	 = "Mesh Renderer",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_mesh_renderer_t*>(ptr), component_mesh_renderer_t{}); },
				.fields =
					{
						{.name		   = "mesh",
						 .display_name = "Mesh",
						 .tooltip	   = "Mesh resource rendered by this entity.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH,
						 .offset	   = offsetof(component_mesh_renderer_t, mesh),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.container_ops = reflection_container_ops_t::inplace_vector_ops_with_default<resource_handle_t, 16, NULL_RESOURCE_HANDLE>(reflected_value_type_e::u64, SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MATERIAL),
						 .name			= "materials",
						 .display_name	= "Materials",
						 .tooltip		= "Material resources used when drawing the mesh.",
						 .offset		= offsetof(component_mesh_renderer_t, materials),
						 .size			= sizeof(inplace_vector_t<resource_handle_t, 16>),
						 .type			= reflected_value_type_e::container},
					},
				.type_id   = type_id_t<component_mesh_renderer_t>::value,
				.size	   = sizeof(component_mesh_renderer_t),
				.alignment = alignof(component_mesh_renderer_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_camera_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_camera",
				.display_name	 = "Camera",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_camera_t*>(ptr), component_camera_t{}); },
				.fields =
					{
						{.name = "fov_degrees", .display_name = "Field of View", .tooltip = "Vertical camera field of view in degrees.", .offset = offsetof(component_camera_t, fov_degrees), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "near_plane", .display_name = "Near Plane", .tooltip = "Nearest visible camera depth in world units.", .offset = offsetof(component_camera_t, near_plane), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "far_plane", .display_name = "Far Plane", .tooltip = "Farthest visible camera depth in world units.", .offset = offsetof(component_camera_t, far_plane), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "priority", .display_name = "Priority", .tooltip = "Camera selection priority when more than one camera is active.", .offset = offsetof(component_camera_t, priority), .size = sizeof(i8), .type = reflected_value_type_e::i8},
					},
				.type_id   = type_id_t<component_camera_t>::value,
				.size	   = sizeof(component_camera_t),
				.alignment = alignof(component_camera_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_light_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "light_type_e",
				.display_name = "Light Type",
				.tooltip	  = "Light shape and attenuation model evaluated by deferred lighting.",
				.fields =
					{
						{.name = "directional", .display_name = "Directional", .tooltip = "Infinite-distance light with a uniform direction and no distance attenuation."},
						{.name = "point", .display_name = "Point", .tooltip = "Omnidirectional inverse-square light emitted from the entity position."},
						{.name = "spot", .display_name = "Spot", .tooltip = "Cone-shaped inverse-square light emitted along the entity forward axis."},
						{.name = "area", .display_name = "Area", .tooltip = "Rectangular emitter oriented by the entity transform."},
					},
				.type_id   = type_id_t<light_type_e>::value,
				.size	   = sizeof(light_type_e),
				.alignment = alignof(light_type_e),
				.flags	   = reflected_type_flag_enum,
			});

			registry.register_type({
				.name			 = "component_light",
				.display_name	 = "Light",
				.tooltip		 = "Deferred scene light supporting directional, point, spot, and rectangular area emission.",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_light_t*>(ptr), component_light_t{}); },
				.fields =
					{
						{.name		   = "type",
						 .display_name = "Type",
						 .tooltip	   = "Selects the light shape and attenuation model.",
						 .sub_type_id  = type_id_t<light_type_e>::value,
						 .offset	   = offsetof(component_light_t, type),
						 .size		   = sizeof(light_type_e),
						 .type		   = reflected_value_type_e::u8},
						{.name		   = "color",
						 .display_name = "Color",
						 .tooltip	   = "Linear light color before the intensity multiplier.",
						 .sub_type_id  = type_id_t<color_t>::value,
						 .offset	   = offsetof(component_light_t, color),
						 .size		   = sizeof(color_t),
						 .type		   = reflected_value_type_e::object},
						{.name = "intensity", .display_name = "Intensity", .tooltip = "Radiometric intensity multiplier applied to the light color.", .offset = offsetof(component_light_t, intensity), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name		   = "range",
						 .display_name = "Range",
						 .tooltip	   = "Smooth cutoff distance in world units; zero disables the finite-range cutoff.",
						 .offset	   = offsetof(component_light_t, range),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.ui_definition		= {.dependency_field = "type"_hs, .dependency_value = static_cast<u32>(light_type_e::spot), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name				= "inner_cone_degrees",
						 .display_name		= "Inner Cone",
						 .tooltip			= "Spotlight half-angle in degrees where emission remains at full strength.",
						 .offset			= offsetof(component_light_t, inner_cone_degrees),
						 .size				= sizeof(f32),
						 .min_clamp			= 0.0f,
						 .max_clamp			= 179.0f,
						 .clamp_granularity = 0.1f,
						 .type				= reflected_value_type_e::f32},
						{.ui_definition		= {.dependency_field = "type"_hs, .dependency_value = static_cast<u32>(light_type_e::spot), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name				= "outer_cone_degrees",
						 .display_name		= "Outer Cone",
						 .tooltip			= "Spotlight half-angle in degrees where emission reaches zero.",
						 .offset			= offsetof(component_light_t, outer_cone_degrees),
						 .size				= sizeof(f32),
						 .min_clamp			= 0.0f,
						 .max_clamp			= 179.0f,
						 .clamp_granularity = 0.1f,
						 .type				= reflected_value_type_e::f32},
						{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = static_cast<u32>(light_type_e::area), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "area_size",
						 .display_name	= "Area Size",
						 .tooltip		= "Full width and height of the rectangular emitter in world units.",
						 .sub_type_id	= type_id_t<vec2f_t>::value,
						 .offset		= offsetof(component_light_t, area_size),
						 .size			= sizeof(vec2f_t),
						 .type			= reflected_value_type_e::object},
						{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = static_cast<u32>(light_type_e::area), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "two_sided",
						 .display_name	= "Two Sided",
						 .tooltip		= "Emits from both faces of the rectangular area light.",
						 .offset		= offsetof(component_light_t, two_sided),
						 .size			= sizeof(u8),
						 .type			= reflected_value_type_e::boolean},
						{.ui_definition = {.dependency_field = "type"_hs, .dependency_value = static_cast<u32>(light_type_e::area), .dependency_type = reflected_field_dependency_type_e::show_if_not_equal},
						 .name			= "cast_shadows",
						 .display_name	= "Cast Shadows",
						 .tooltip		= "Enables realtime shadow rendering for directional, point, and spot lights.",
						 .offset		= offsetof(component_light_t, cast_shadows),
						 .size			= sizeof(u8),
						 .type			= reflected_value_type_e::boolean},
						{.ui_definition = {.dependency_field = "cast_shadows"_hs, .dependency_value = 1, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "shadow_near_plane",
						 .display_name	= "Shadow Near Plane",
						 .tooltip		= "Near clipping distance used by point and spot shadow projections.",
						 .offset		= offsetof(component_light_t, shadow_near_plane),
						 .size			= sizeof(f32),
						 .type			= reflected_value_type_e::f32},
						{.ui_definition		= {.dependency_field = "cast_shadows"_hs, .dependency_value = 1, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name				= "shadow_resolution",
						 .display_name		= "Shadow Resolution",
						 .tooltip			= "Requested square shadow-map width and height when shadows are enabled.",
						 .offset			= offsetof(component_light_t, shadow_resolution),
						 .size				= sizeof(u16),
						 .flags				= reflected_field_flag_clamped,
						 .min_clamp			= 64.0f,
						 .max_clamp			= 8192.0f,
						 .clamp_granularity = 64.0f,
						 .type				= reflected_value_type_e::u16},
						{.ui_definition = {.dependency_field = "cast_shadows"_hs, .dependency_value = 1, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "shadow_bias",
						 .display_name	= "Shadow Bias",
						 .tooltip		= "Receiver depth bias used while sampling the shadow map.",
						 .offset		= offsetof(component_light_t, shadow_bias),
						 .size			= sizeof(f32),
						 .type			= reflected_value_type_e::f32},
						{.ui_definition = {.dependency_field = "cast_shadows"_hs, .dependency_value = 1, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "shadow_normal_bias",
						 .display_name	= "Shadow Normal Bias",
						 .tooltip		= "Slope-scaled receiver bias used to reduce self-shadowing.",
						 .offset		= offsetof(component_light_t, shadow_normal_bias),
						 .size			= sizeof(f32),
						 .type			= reflected_value_type_e::f32},
						{.ui_definition		= {.dependency_field = "type"_hs, .dependency_value = static_cast<u32>(light_type_e::directional), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name				= "shadow_cascade_count",
						 .display_name		= "Shadow Cascades",
						 .tooltip			= "Requested directional shadow cascade count when shadows are enabled.",
						 .offset			= offsetof(component_light_t, shadow_cascade_count),
						 .size				= sizeof(u8),
						 .min_clamp			= 1.0f,
						 .max_clamp			= 8.0f,
						 .clamp_granularity = 1.0f,
						 .type				= reflected_value_type_e::u8},
					},
				.type_id   = type_id_t<component_light_t>::value,
				.size	   = sizeof(component_light_t),
				.alignment = alignof(component_light_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_skybox_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_skybox",
				.display_name	 = "Skybox",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_skybox_t*>(ptr), component_skybox_t{}); },
				.fields =
					{
						{.name		   = "skybox_asset",
						 .display_name = "Skybox",
						 .tooltip	   = "HDR skybox resource used for the scene background and lighting.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_HDR_SKYBOX,
						 .offset	   = offsetof(component_skybox_t, skybox_asset),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name = "intensity", .display_name = "Intensity", .tooltip = "Multiplier applied to skybox lighting contribution.", .offset = offsetof(component_skybox_t, intensity), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "exposure", .display_name = "Exposure", .tooltip = "Exposure multiplier applied when sampling the skybox.", .offset = offsetof(component_skybox_t, exposure), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					},
				.type_id   = type_id_t<component_skybox_t>::value,
				.size	   = sizeof(component_skybox_t),
				.alignment = alignof(component_skybox_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_post_process_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "tonemap_mode_e",
				.display_name = "Tone Map Mode",
				.tooltip	  = "Operator used to compress linear HDR color into the displayable range.",
				.fields =
					{
						{.name = "aces", .display_name = "ACES", .tooltip = "Filmic ACES fitted curve with a smooth highlight rolloff."},
						{.name = "reinhard", .display_name = "Reinhard", .tooltip = "Extended Reinhard curve controlled by the Reinhard white point."},
						{.name = "none", .display_name = "None", .tooltip = "Disables a tone curve and clamps color directly to the displayable range."},
					},
				.type_id   = type_id_t<tonemap_mode_e>::value,
				.size	   = sizeof(tonemap_mode_e),
				.alignment = alignof(tonemap_mode_e),
				.flags	   = reflected_type_flag_enum,
			});

			registry.register_type({
				.name		  = "post_process_ssao_t",
				.display_name = "SSAO",
				.tooltip	  = "Half-resolution horizon-based ambient occlusion using scene depth and G-buffer normals.",
				.fields =
					{
						{.name = "radius_world", .display_name = "Radius", .tooltip = "Occlusion radius in world units.", .offset = offsetof(post_process_ssao_t, radius_world), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name		   = "bias",
						 .display_name = "Bias",
						 .tooltip	   = "Reduces self-occlusion and surface acne; excessive values remove contact occlusion.",
						 .offset	   = offsetof(post_process_ssao_t, bias),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "intensity",
						 .display_name = "Intensity",
						 .tooltip	   = "Multiplier applied to the accumulated occlusion before contrast shaping.",
						 .offset	   = offsetof(post_process_ssao_t, intensity),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "power",
						 .display_name = "Power",
						 .tooltip	   = "Shapes AO contrast; higher values produce darker, more concentrated occlusion.",
						 .offset	   = offsetof(post_process_ssao_t, power),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "random_rotation_strength",
						 .display_name = "Random Rotation Strength",
						 .tooltip	   = "Scales per-pixel sample-pattern rotation used to break up directional banding.",
						 .offset	   = offsetof(post_process_ssao_t, random_rotation_strength),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "direction_count",
						 .display_name = "Direction Count",
						 .tooltip	   = "Number of angular horizon-search directions; higher values improve quality and increase GPU cost.",
						 .offset	   = offsetof(post_process_ssao_t, direction_count),
						 .size		   = sizeof(u32),
						 .type		   = reflected_value_type_e::u32},
						{.name		   = "step_count",
						 .display_name = "Step Count",
						 .tooltip	   = "Number of depth-march steps per direction; higher values improve coverage and increase GPU cost.",
						 .offset	   = offsetof(post_process_ssao_t, step_count),
						 .size		   = sizeof(u32),
						 .type		   = reflected_value_type_e::u32},
						{.name = "enabled", .display_name = "Enabled", .tooltip = "Enables SSAO for cameras using this post-process component.", .offset = offsetof(post_process_ssao_t, enabled), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
					},
				.type_id   = type_id_t<post_process_ssao_t>::value,
				.size	   = sizeof(post_process_ssao_t),
				.alignment = alignof(post_process_ssao_t),
			});

			registry.register_type({
				.name		  = "post_process_bloom_t",
				.display_name = "Bloom",
				.tooltip	  = "Multi-resolution HDR bloom generated before exposure and tone mapping.",
				.fields =
					{
						{.name		   = "strength",
						 .display_name = "Strength",
						 .tooltip	   = "Multiplier applied when bloom is composited into the HDR lighting result.",
						 .offset	   = offsetof(post_process_bloom_t, strength),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "filter_radius",
						 .display_name = "Filter Radius",
						 .tooltip	   = "Normalized sampling radius of the upsample tent filter; higher values produce wider, softer bloom.",
						 .offset	   = offsetof(post_process_bloom_t, filter_radius),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name = "enabled", .display_name = "Enabled", .tooltip = "Enables bloom for cameras using this post-process component.", .offset = offsetof(post_process_bloom_t, enabled), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
					},
				.type_id   = type_id_t<post_process_bloom_t>::value,
				.size	   = sizeof(post_process_bloom_t),
				.alignment = alignof(post_process_bloom_t),
			});

			registry.register_type({
				.name			 = "component_post_process",
				.display_name	 = "Post Process",
				.tooltip		 = "Camera post-processing controls applied to the HDR lighting result before display output.",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_post_process_t*>(ptr), component_post_process_t{}); },
				.fields =
					{
						{.name		   = "ssao",
						 .display_name = "SSAO",
						 .tooltip	   = "Screen-space ambient occlusion settings evaluated before deferred lighting.",
						 .sub_type_id  = type_id_t<post_process_ssao_t>::value,
						 .offset	   = offsetof(component_post_process_t, ssao),
						 .size		   = sizeof(post_process_ssao_t),
						 .type		   = reflected_value_type_e::object},
						{.name		   = "bloom",
						 .display_name = "Bloom",
						 .tooltip	   = "HDR bloom settings evaluated from the deferred lighting result.",
						 .sub_type_id  = type_id_t<post_process_bloom_t>::value,
						 .offset	   = offsetof(component_post_process_t, bloom),
						 .size		   = sizeof(post_process_bloom_t),
						 .type		   = reflected_value_type_e::object},
						{.name		   = "exposure_ev",
						 .display_name = "Exposure",
						 .tooltip	   = "Exposure adjustment in EV stops; each increase of one doubles HDR brightness before tone mapping.",
						 .offset	   = offsetof(component_post_process_t, exposure_ev),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "saturation",
						 .display_name = "Saturation",
						 .tooltip	   = "Color saturation multiplier; zero is grayscale, one is unchanged, and values above one increase saturation.",
						 .offset	   = offsetof(component_post_process_t, saturation),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "temperature",
						 .display_name = "Temperature",
						 .tooltip	   = "White-balance temperature shift; positive values warm the image and negative values cool it.",
						 .offset	   = offsetof(component_post_process_t, temperature),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "tint",
						 .display_name = "Tint",
						 .tooltip	   = "White-balance tint shift; positive values shift toward green and negative values toward magenta.",
						 .offset	   = offsetof(component_post_process_t, tint),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "reinhard_white_point",
						 .display_name = "Reinhard White Point",
						 .tooltip	   = "Highlight white point used by Reinhard tone mapping; higher values preserve a wider HDR highlight range.",
						 .offset	   = offsetof(component_post_process_t, reinhard_white_point),
						 .size		   = sizeof(f32),
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "tonemap_mode",
						 .display_name = "Tone Map Mode",
						 .tooltip	   = "Selects how linear HDR color is compressed into the displayable output range.",
						 .sub_type_id  = type_id_t<tonemap_mode_e>::value,
						 .offset	   = offsetof(component_post_process_t, tonemap_mode),
						 .size		   = sizeof(tonemap_mode_e),
						 .type		   = reflected_value_type_e::u8},
					},
				.type_id   = type_id_t<component_post_process_t>::value,
				.size	   = sizeof(component_post_process_t),
				.alignment = alignof(component_post_process_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_prefab_reference_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_prefab_reference",
				.display_name	 = "Prefab Reference",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_prefab_reference_t*>(ptr), component_prefab_reference_t{}); },
				.fields =
					{
						{.name = "prefab", .display_name = "Prefab", .sub_type_id = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PREFAB, .offset = offsetof(component_prefab_reference_t, prefab), .size = sizeof(resource_handle_t), .type = reflected_value_type_e::u64},
						{.name = "is_root", .display_name = "IsRoot", .offset = offsetof(component_prefab_reference_t, is_root), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					},
				.type_id   = type_id_t<component_prefab_reference_t>::value,
				.size	   = sizeof(component_prefab_reference_t),
				.alignment = alignof(component_prefab_reference_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_entity_tags_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_entity_tags",
				.display_name	 = "Tags",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_entity_tags_t*>(ptr), component_entity_tags_t{}); },
				.fields			 = {{.name = "tags", .display_name = "Tags", .offset = offsetof(component_entity_tags_t, tags), .size = sizeof(u64), .type = reflected_value_type_e::bitmask}},
				.bitmask_opts	 = {.get_option_count_fn = get_entity_tag_bitmask_option_count, .get_option_fn = get_entity_tag_bitmask_option, .build_title_fn = build_entity_tag_bitmask_title},
				.type_id		 = type_id_t<component_entity_tags_t>::value,
				.size			 = sizeof(component_entity_tags_t),
				.alignment		 = alignof(component_entity_tags_t),
				.flags			 = reflected_type_flag_component,
			});
		}

		void register_physics_component_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "physics_shape_type_e",
				.display_name = "Physics Shape",
				.fields		  = {{.name = "box", .display_name = "Box"}, {.name = "sphere", .display_name = "Sphere"}, {.name = "capsule", .display_name = "Capsule"}, {.name = "cylinder", .display_name = "Cylinder"}, {.name = "mesh", .display_name = "Mesh"}},
				.type_id	  = type_id_t<physics_shape_type_e>::value,
				.size		  = sizeof(physics_shape_type_e),
				.alignment	  = alignof(physics_shape_type_e),
				.flags		  = reflected_type_flag_enum,
			});

			registry.register_type({
				.name		  = "physics_motion_type_e",
				.display_name = "Physics Motion",
				.fields		  = {{.name = "static_body", .display_name = "Static"}, {.name = "kinematic_body", .display_name = "Kinematic"}, {.name = "dynamic_body", .display_name = "Dynamic"}},
				.type_id	  = type_id_t<physics_motion_type_e>::value,
				.size		  = sizeof(physics_motion_type_e),
				.alignment	  = alignof(physics_motion_type_e),
				.flags		  = reflected_type_flag_enum,
			});

			registry.register_type({
				.name			 = "component_collider",
				.display_name	 = "Collider",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_collider_t*>(ptr), component_collider_t{}); },
				.fields =
					{
						{.name = "shape", .display_name = "Shape", .sub_type_id = type_id_t<physics_shape_type_e>::value, .offset = offsetof(component_collider_t, shape), .size = sizeof(physics_shape_type_e), .type = reflected_value_type_e::u8},
						{.name = "local_position", .display_name = "Local Position", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_collider_t, local_position), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
						{.name = "local_rotation", .display_name = "Local Rotation", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(component_collider_t, local_rotation), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
						{.ui_definition = {.dependency_field = "shape"_hs, .dependency_value = static_cast<u32>(physics_shape_type_e::box), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "half_extent",
						 .display_name	= "Half Extent",
						 .sub_type_id	= type_id_t<vec3f_t>::value,
						 .offset		= offsetof(component_collider_t, half_extent),
						 .size			= sizeof(vec3f_t),
						 .type			= reflected_value_type_e::object},
						{.ui_definition = {.dependency_field = "shape"_hs, .dependency_value = static_cast<u32>(physics_shape_type_e::box), .dependency_type = reflected_field_dependency_type_e::show_if_not_equal},
						 .name			= "radius",
						 .display_name	= "Radius",
						 .offset		= offsetof(component_collider_t, radius),
						 .size			= sizeof(f32),
						 .flags			= reflected_field_flag_clamped,
						 .min_clamp		= 0.001f,
						 .max_clamp		= 10000.0f,
						 .type			= reflected_value_type_e::f32},
						{.name		   = "half_height",
						 .display_name = "Half Height",
						 .offset	   = offsetof(component_collider_t, half_height),
						 .size		   = sizeof(f32),
						 .flags		   = reflected_field_flag_clamped,
						 .min_clamp	   = 0.001f,
						 .max_clamp	   = 10000.0f,
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "physical_material",
						 .display_name = "Physical Material",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICAL_MATERIAL,
						 .offset	   = offsetof(component_collider_t, physical_material),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.ui_definition = {.dependency_field = "shape"_hs, .dependency_value = static_cast<u32>(physics_shape_type_e::mesh), .dependency_type = reflected_field_dependency_type_e::show_if_equals},
						 .name			= "collision_mesh",
						 .display_name	= "Collision Mesh",
						 .sub_type_id	= SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICS_COLLISION_MESH,
						 .offset		= offsetof(component_collider_t, collision_mesh),
						 .size			= sizeof(resource_handle_t),
						 .type			= reflected_value_type_e::u64},
						{.name		   = "collision_layer",
						 .display_name = "Collision Layer",
						 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_COLLISION_LAYER,
						 .offset	   = offsetof(component_collider_t, collision_layer),
						 .size		   = sizeof(u8),
						 .type		   = reflected_value_type_e::u8},
						{.name = "is_sensor", .display_name = "Sensor", .offset = offsetof(component_collider_t, is_sensor), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
					},
				.type_id   = type_id_t<component_collider_t>::value,
				.size	   = sizeof(component_collider_t),
				.alignment = alignof(component_collider_t),
				.flags	   = reflected_type_flag_component,
			});

			registry.register_type({
				.name			 = "component_rigid_body",
				.display_name	 = "Rigid Body",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_rigid_body_t*>(ptr), component_rigid_body_t{}); },
				.fields =
					{
						{.name		   = "motion_type",
						 .display_name = "Motion Type",
						 .sub_type_id  = type_id_t<physics_motion_type_e>::value,
						 .offset	   = offsetof(component_rigid_body_t, motion_type),
						 .size		   = sizeof(physics_motion_type_e),
						 .type		   = reflected_value_type_e::u8},
						{.name = "mass", .display_name = "Mass", .offset = offsetof(component_rigid_body_t, mass), .size = sizeof(f32), .flags = reflected_field_flag_clamped, .min_clamp = 0.001f, .max_clamp = 1000000.0f, .type = reflected_value_type_e::f32},
						{.name = "gravity_factor", .display_name = "Gravity Factor", .offset = offsetof(component_rigid_body_t, gravity_factor), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name		   = "linear_damping",
						 .display_name = "Linear Damping",
						 .offset	   = offsetof(component_rigid_body_t, linear_damping),
						 .size		   = sizeof(f32),
						 .flags		   = reflected_field_flag_clamped,
						 .min_clamp	   = 0.0f,
						 .max_clamp	   = 1.0f,
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "angular_damping",
						 .display_name = "Angular Damping",
						 .offset	   = offsetof(component_rigid_body_t, angular_damping),
						 .size		   = sizeof(f32),
						 .flags		   = reflected_field_flag_clamped,
						 .min_clamp	   = 0.0f,
						 .max_clamp	   = 1.0f,
						 .type		   = reflected_value_type_e::f32},
						{.name = "motion_quality_continuous", .display_name = "Continuous Collision", .offset = offsetof(component_rigid_body_t, motion_quality_continuous), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
						{.name = "allow_sleep", .display_name = "Allow Sleep", .offset = offsetof(component_rigid_body_t, allow_sleep), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
					},
				.type_id   = type_id_t<component_rigid_body_t>::value,
				.size	   = sizeof(component_rigid_body_t),
				.alignment = alignof(component_rigid_body_t),
				.flags	   = reflected_type_flag_component,
			});

			registry.register_type({
				.name			 = "component_character_mover",
				.display_name	 = "Character Mover",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_character_mover_t*>(ptr), component_character_mover_t{}); },
				.fields =
					{
						{.name = "shape_offset", .display_name = "Shape Offset", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_character_mover_t, shape_offset), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
						{.name		   = "radius",
						 .display_name = "Radius",
						 .offset	   = offsetof(component_character_mover_t, radius),
						 .size		   = sizeof(f32),
						 .flags		   = reflected_field_flag_clamped,
						 .min_clamp	   = 0.001f,
						 .max_clamp	   = 1000.0f,
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "half_height",
						 .display_name = "Half Height",
						 .offset	   = offsetof(component_character_mover_t, half_height),
						 .size		   = sizeof(f32),
						 .flags		   = reflected_field_flag_clamped,
						 .min_clamp	   = 0.001f,
						 .max_clamp	   = 1000.0f,
						 .type		   = reflected_value_type_e::f32},
						{.name		   = "max_slope_degrees",
						 .display_name = "Maximum Slope",
						 .offset	   = offsetof(component_character_mover_t, max_slope_degrees),
						 .size		   = sizeof(f32),
						 .flags		   = reflected_field_flag_clamped,
						 .min_clamp	   = 0.0f,
						 .max_clamp	   = 89.0f,
						 .type		   = reflected_value_type_e::f32},
						{.name = "step_up", .display_name = "Step Up", .offset = offsetof(component_character_mover_t, step_up), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "step_down", .display_name = "Step Down", .offset = offsetof(component_character_mover_t, step_down), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "min_step_forward", .display_name = "Minimum Step Forward", .offset = offsetof(component_character_mover_t, min_step_forward), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "step_forward_test", .display_name = "Step Forward Test", .offset = offsetof(component_character_mover_t, step_forward_test), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "mass", .display_name = "Mass", .offset = offsetof(component_character_mover_t, mass), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "max_strength", .display_name = "Maximum Strength", .offset = offsetof(component_character_mover_t, max_strength), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "padding", .display_name = "Padding", .offset = offsetof(component_character_mover_t, padding), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "predictive_contact_distance", .display_name = "Predictive Contact Distance", .offset = offsetof(component_character_mover_t, predictive_contact_distance), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "penetration_recovery_speed", .display_name = "Penetration Recovery Speed", .offset = offsetof(component_character_mover_t, penetration_recovery_speed), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name		   = "collision_layer",
						 .display_name = "Collision Layer",
						 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_COLLISION_LAYER,
						 .offset	   = offsetof(component_character_mover_t, collision_layer),
						 .size		   = sizeof(u8),
						 .type		   = reflected_value_type_e::u8},
						{.name = "enhanced_internal_edge_removal", .display_name = "Enhanced Internal Edge Removal", .offset = offsetof(component_character_mover_t, enhanced_internal_edge_removal), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
					},
				.type_id   = type_id_t<component_character_mover_t>::value,
				.size	   = sizeof(component_character_mover_t),
				.alignment = alignof(component_character_mover_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_debug_widgets_enum_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "debug_widgets_enum",
				.display_name = "Debug Widgets Enum",
				.fields =
					{
						{.name = "debug_widgets_enum_a", .display_name = "Debug Widgets Enum A"},
						{.name = "debug_widgets_enum_b", .display_name = "Debug Widgets Enum B"},
					},
				.type_id   = type_id_t<debug_widgets_enum>::value,
				.size	   = sizeof(debug_widgets_enum),
				.alignment = alignof(debug_widgets_enum),
				.flags	   = reflected_type_flag_enum,
			});

			registry.register_type({
				.name		  = "debug_widgets_enum2",
				.display_name = "Debug Widgets Enum2",
				.fields =
					{
						{.name = "debug_widgets_enum2_a", .display_name = "Debug Widgets Enum2 A"},
						{.name = "debug_widgets_enum2_b", .display_name = "Debug Widgets Enum2 B"},
					},
				.type_id   = type_id_t<debug_widgets_enum2>::value,
				.size	   = sizeof(debug_widgets_enum2),
				.alignment = alignof(debug_widgets_enum2),
				.flags	   = reflected_type_flag_enum,
			});
		}

		void register_debug_struct_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "debug_struct2_t",
				.display_name = "Debug Struct2",
				.fields =
					{
						{.name = "f32_value", .display_name = "F32", .tooltip = "Debug reflected nested struct f32 value.", .offset = offsetof(debug_struct2_t, f32_value), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "u32_value", .display_name = "U32", .tooltip = "Debug reflected nested struct u32 value.", .offset = offsetof(debug_struct2_t, u32_value), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					},
				.type_id   = type_id_t<debug_struct2_t>::value,
				.size	   = sizeof(debug_struct2_t),
				.alignment = alignof(debug_struct2_t),
			});

			registry.register_type({
				.name		  = "debug_struct_t",
				.display_name = "Debug Struct",
				.fields =
					{
						{.name		   = "vec3_value",
						 .display_name = "Vec3",
						 .tooltip	   = "Debug reflected struct vec3 value.",
						 .sub_type_id  = type_id_t<vec3f_t>::value,
						 .offset	   = offsetof(debug_struct_t, vec3_value),
						 .size		   = sizeof(vec3f_t),
						 .type		   = reflected_value_type_e::object},
						{
							.name		  = "f32_value",
							.display_name = "F32",
							.tooltip	  = "Debug reflected struct f32 value.",
							.offset		  = offsetof(debug_struct_t, f32_value),
							.size		  = sizeof(f32),
							.type		  = reflected_value_type_e::f32,
						},
						{
							.name		  = "test",
							.display_name = "Test",
							.tooltip	  = "yeah",
							.sub_type_id  = type_id_t<debug_struct2_t>::value,
							.offset		  = offsetof(debug_struct_t, test),
							.size		  = sizeof(debug_struct2_t),
							.type		  = reflected_value_type_e::object,
						},
					},
				.type_id   = type_id_t<debug_struct_t>::value,
				.size	   = sizeof(debug_struct_t),
				.alignment = alignof(debug_struct_t),
			});
		}

		void register_component_debug_widgets_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "debug_widgets_component",
				.display_name	 = "Debug Widgets",
				.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_debug_widgets_t*>(ptr), component_debug_widgets_t{}); },
				.fields =
					{
						{.name = "f32_value", .display_name = "F32", .tooltip = "Debug reflected f32 value.", .offset = offsetof(component_debug_widgets_t, f32_value), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "i32_value", .display_name = "I32", .tooltip = "Debug reflected i32 value.", .offset = offsetof(component_debug_widgets_t, i32_value), .size = sizeof(i32), .type = reflected_value_type_e::i32},
						{.name = "u32_value", .display_name = "U32", .tooltip = "Debug reflected u32 value.", .offset = offsetof(component_debug_widgets_t, u32_value), .size = sizeof(u32), .type = reflected_value_type_e::u32},
						{.name = "u8_value", .display_name = "U8", .tooltip = "Debug reflected u8 value.", .offset = offsetof(component_debug_widgets_t, u8_value), .size = sizeof(u8), .type = reflected_value_type_e::u8},
						{.name = "bool8_value", .display_name = "Bool8", .tooltip = "Debug reflected bool8 value.", .offset = offsetof(component_debug_widgets_t, bool8_value), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
						{.name		   = "audio_handle_value",
						 .display_name = "Audio Handle",
						 .tooltip	   = "Debug reflected audio handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_AUDIO,
						 .offset	   = offsetof(component_debug_widgets_t, audio_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "font_handle_value",
						 .display_name = "Font Handle",
						 .tooltip	   = "Debug reflected font handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_FONT,
						 .offset	   = offsetof(component_debug_widgets_t, font_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "mesh_handle_value",
						 .display_name = "Mesh Handle",
						 .tooltip	   = "Debug reflected mesh handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH,
						 .offset	   = offsetof(component_debug_widgets_t, mesh_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "skeleton_handle_value",
						 .display_name = "Skeleton Handle",
						 .tooltip	   = "Debug reflected skeleton handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON,
						 .offset	   = offsetof(component_debug_widgets_t, skeleton_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "animation_handle_value",
						 .display_name = "Animation Handle",
						 .tooltip	   = "Debug reflected animation handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION,
						 .offset	   = offsetof(component_debug_widgets_t, animation_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "material_handle_value",
						 .display_name = "Material Handle",
						 .tooltip	   = "Debug reflected material handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MATERIAL,
						 .offset	   = offsetof(component_debug_widgets_t, material_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "shader_handle_value",
						 .display_name = "Shader Handle",
						 .tooltip	   = "Debug reflected shader handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SHADER,
						 .offset	   = offsetof(component_debug_widgets_t, shader_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "texture_handle_value",
						 .display_name = "Texture Handle",
						 .tooltip	   = "Debug reflected texture handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_TEXTURE,
						 .offset	   = offsetof(component_debug_widgets_t, texture_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "texture_sampler_handle_value",
						 .display_name = "Texture Sampler Handle",
						 .tooltip	   = "Debug reflected texture sampler handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_TEXTURE_SAMPLER,
						 .offset	   = offsetof(component_debug_widgets_t, texture_sampler_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "physical_material_handle_value",
						 .display_name = "Physical Material Handle",
						 .tooltip	   = "Debug reflected physical material handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICAL_MATERIAL,
						 .offset	   = offsetof(component_debug_widgets_t, physical_material_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "prefab_handle_value",
						 .display_name = "Prefab Handle",
						 .tooltip	   = "Debug reflected prefab handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PREFAB,
						 .offset	   = offsetof(component_debug_widgets_t, prefab_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "animation_state_machine_handle_value",
						 .display_name = "Animation State Machine Handle",
						 .tooltip	   = "Debug reflected animation state machine handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION_STATE_MACHINE,
						 .offset	   = offsetof(component_debug_widgets_t, animation_state_machine_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "hdr_skybox_handle_value",
						 .display_name = "HDR Skybox Handle",
						 .tooltip	   = "Debug reflected HDR skybox handle.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_HDR_SKYBOX,
						 .offset	   = offsetof(component_debug_widgets_t, hdr_skybox_handle_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "entity_guid_value",
						 .display_name = "Entity GUID",
						 .tooltip	   = "Debug reflected entity guid value.",
						 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID,
						 .offset	   = offsetof(component_debug_widgets_t, entity_guid_value),
						 .size		   = sizeof(entity_guid_t),
						 .type		   = reflected_value_type_e::u64},
						{.name = "text_id_value", .display_name = "Text ID", .tooltip = "Debug reflected text id value.", .offset = offsetof(component_debug_widgets_t, text_id_value), .size = sizeof(u32), .type = reflected_value_type_e::u32},
						{.name		   = "quat_value",
						 .display_name = "Quat",
						 .tooltip	   = "Debug reflected quaternion value.",
						 .sub_type_id  = type_id_t<quat_t>::value,
						 .offset	   = offsetof(component_debug_widgets_t, quat_value),
						 .size		   = sizeof(quat_t),
						 .type		   = reflected_value_type_e::object},
						{.name		   = "color_value",
						 .display_name = "Color",
						 .tooltip	   = "Debug reflected color value.",
						 .sub_type_id  = type_id_t<color_t>::value,
						 .offset	   = offsetof(component_debug_widgets_t, color_value),
						 .size		   = sizeof(color_t),
						 .type		   = reflected_value_type_e::object},
						{.name		   = "enum8_value",
						 .display_name = "Enum8",
						 .tooltip	   = "Debug reflected enum8 value.",
						 .sub_type_id  = type_id_t<debug_widgets_enum>::value,
						 .offset	   = offsetof(component_debug_widgets_t, enum8_value),
						 .size		   = sizeof(u8),
						 .type		   = reflected_value_type_e::u8},
						{.name		   = "enum32_value",
						 .display_name = "Enum32",
						 .tooltip	   = "Debug reflected enum32 value.",
						 .sub_type_id  = type_id_t<debug_widgets_enum2>::value,
						 .offset	   = offsetof(component_debug_widgets_t, enum32_value),
						 .size		   = sizeof(u32),
						 .type		   = reflected_value_type_e::u32},
						{.container_ops = reflection_container_ops_t::inplace_vector_ops<u32, 4>(reflected_value_type_e::u32),
						 .name			= "inplace_vector_value",
						 .display_name	= "Inplace Vector",
						 .tooltip		= "Debug reflected inplace vector value.",
						 .offset		= offsetof(component_debug_widgets_t, inplace_vector_value),
						 .size			= sizeof(inplace_vector_t<u32, 4>),
						 .type			= reflected_value_type_e::container},
						{.name		   = "debug_struct_value",
						 .display_name = "Debug Struct",
						 .tooltip	   = "Debug reflected object struct value.",
						 .sub_type_id  = type_id_t<debug_struct_t>::value,
						 .offset	   = offsetof(component_debug_widgets_t, debug_struct_value),
						 .size		   = sizeof(debug_struct_t),
						 .type		   = reflected_value_type_e::object},
						{.name		   = "debug_struct2_value",
						 .display_name = "Debug Struct2",
						 .tooltip	   = "Debug reflected object struct2 value.",
						 .sub_type_id  = type_id_t<debug_struct2_t>::value,
						 .offset	   = offsetof(component_debug_widgets_t, debug_struct2_value),
						 .size		   = sizeof(debug_struct2_t),
						 .type		   = reflected_value_type_e::object},
						{.name = "i8_value", .display_name = "I8", .tooltip = "Debug reflected i8 value.", .offset = offsetof(component_debug_widgets_t, i8_value), .size = sizeof(i8), .type = reflected_value_type_e::i8},
					},
				.type_id   = type_id_t<component_debug_widgets_t>::value,
				.size	   = sizeof(component_debug_widgets_t),
				.alignment = alignof(component_debug_widgets_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_alive_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_alive",
				.display_name	 = "Alive",
				.default_init_fn = [](void*) {},
				.type_id		 = type_id_t<component_alive_t>::value,
				.size			 = sizeof(component_alive_t),
				.alignment		 = alignof(component_alive_t),
				.flags			 = reflected_type_flag_tag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
			});
		}

		void register_component_disabled_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_disabled",
				.display_name	 = "Disabled",
				.default_init_fn = [](void*) {},
				.type_id		 = type_id_t<component_disabled_t>::value,
				.size			 = sizeof(component_disabled_t),
				.alignment		 = alignof(component_disabled_t),
				.flags			 = reflected_type_flag_tag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_no_serialize_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name			 = "component_no_serialize",
				.display_name	 = "No Serialize",
				.default_init_fn = [](void*) {},
				.type_id		 = type_id_t<component_no_serialize_t>::value,
				.size			 = sizeof(component_no_serialize_t),
				.alignment		 = alignof(component_no_serialize_t),
				.flags			 = reflected_type_flag_tag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
			});
		}
	}

	engine_component_reflection_t::engine_component_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		register_component_hierarchy_reflection(registry);
		register_component_guid_reflection(registry);
		register_component_transform_reflection(registry);
		register_component_name_reflection(registry);
		register_component_mesh_renderer_reflection(registry);
		register_component_camera_reflection(registry);
		register_component_light_reflection(registry);
		register_component_post_process_reflection(registry);
		register_component_skybox_reflection(registry);
		register_component_prefab_reference_reflection(registry);
		register_component_entity_tags_reflection(registry);
		register_physics_component_reflection(registry);
		register_debug_widgets_enum_reflection(registry);
		register_debug_struct_reflection(registry);
		register_component_debug_widgets_reflection(registry);
		register_component_alive_reflection(registry);
		register_component_disabled_reflection(registry);
		register_component_no_serialize_reflection(registry);
	}
}
