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
#include <sfg/runtime/resources/resource_type.hpp>

#include <cstddef>

namespace sfg
{
	SFG_DEFINE_TYPE_ID(debug_widgets_enum);
	SFG_DEFINE_TYPE_ID(debug_widgets_enum2);

	namespace
	{
		void register_component_hierarchy_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_hierarchy",
				.display_name = "Hierarchy",
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
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_guid_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_guid",
				.display_name = "GUID",
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
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_transform_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_transform",
				.display_name = "Transform",
				.fields =
					{
						{.name = "pos", .display_name = "Position", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_transform_t, pos), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
						{.name = "rot", .display_name = "Rotation", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(component_transform_t, rot), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
						{.name = "scale", .display_name = "Scale", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(component_transform_t, scale), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					},
				.type_id   = type_id_t<component_transform_t>::value,
				.size	   = sizeof(component_transform_t),
				.alignment = alignof(component_transform_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_name_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_name",
				.display_name = "Name",
				.fields =
					{
						{.name = "text", .display_name = "Text", .offset = offsetof(component_name_t, text), .size = sizeof(component_name_t::text), .type = reflected_value_type_e::char_array},
					},
				.type_id   = type_id_t<component_name_t>::value,
				.size	   = sizeof(component_name_t),
				.alignment = alignof(component_name_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_mesh_renderer_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_mesh_renderer",
				.display_name = "Mesh Renderer",
				.fields =
					{
						{.name		   = "mesh",
						 .display_name = "Mesh",
						 .tooltip	   = "Mesh resource rendered by this entity.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH,
						 .offset	   = offsetof(component_mesh_renderer_t, mesh),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
						{.name		   = "material",
						 .display_name = "Material",
						 .tooltip	   = "Material resource used when drawing the mesh.",
						 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MATERIAL,
						 .offset	   = offsetof(component_mesh_renderer_t, material),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
					},
				.type_id   = type_id_t<component_mesh_renderer_t>::value,
				.size	   = sizeof(component_mesh_renderer_t),
				.alignment = alignof(component_mesh_renderer_t),
				.flags	   = reflected_type_flag_component,
			});
		}

		void register_component_render_object_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_render_object",
				.display_name = "Render Object",
				.fields =
					{
						{.name = "render_id", .display_name = "Render ID", .tooltip = "Renderer-owned object index for this entity.", .offset = offsetof(component_render_object_t, render_id), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					},
				.type_id   = type_id_t<component_render_object_t>::value,
				.size	   = sizeof(component_render_object_t),
				.alignment = alignof(component_render_object_t),
				.flags	   = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_camera_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_camera",
				.display_name = "Camera",
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

		void register_component_skybox_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_skybox",
				.display_name = "Skybox",
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

		void register_component_prefab_reference_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_prefab_reference",
				.display_name = "Prefab Reference",
				.fields =
					{
						{.name = "prefab", .display_name = "Prefab", .sub_type_id = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PREFAB, .offset = offsetof(component_prefab_reference_t, prefab), .size = sizeof(resource_handle_t), .type = reflected_value_type_e::u64},
					},
				.type_id   = type_id_t<component_prefab_reference_t>::value,
				.size	   = sizeof(component_prefab_reference_t),
				.alignment = alignof(component_prefab_reference_t),
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
				.name		  = "debug_widgets_component",
				.display_name = "Debug Widgets",
				.fields =
					{
						{.name = "f32_value", .display_name = "F32", .tooltip = "Debug reflected f32 value.", .offset = offsetof(component_debug_widgets_t, f32_value), .size = sizeof(f32), .type = reflected_value_type_e::f32},
						{.name = "i32_value", .display_name = "I32", .tooltip = "Debug reflected i32 value.", .offset = offsetof(component_debug_widgets_t, i32_value), .size = sizeof(i32), .type = reflected_value_type_e::i32},
						{.name = "u32_value", .display_name = "U32", .tooltip = "Debug reflected u32 value.", .offset = offsetof(component_debug_widgets_t, u32_value), .size = sizeof(u32), .type = reflected_value_type_e::u32},
						{.name = "u8_value", .display_name = "U8", .tooltip = "Debug reflected u8 value.", .offset = offsetof(component_debug_widgets_t, u8_value), .size = sizeof(u8), .type = reflected_value_type_e::u8},
						{.name = "bool8_value", .display_name = "Bool8", .tooltip = "Debug reflected bool8 value.", .offset = offsetof(component_debug_widgets_t, bool8_value), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
						{.name		   = "resource_value",
						 .display_name = "Resource Handle",
						 .tooltip	   = "Debug reflected resource handle.",
						 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
						 .offset	   = offsetof(component_debug_widgets_t, resource_value),
						 .size		   = sizeof(resource_handle_t),
						 .type		   = reflected_value_type_e::u64},
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
				.name		  = "component_alive",
				.display_name = "Alive",
				.type_id	  = type_id_t<component_alive_t>::value,
				.size		  = sizeof(component_alive_t),
				.alignment	  = alignof(component_alive_t),
				.flags		  = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_disabled_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_disabled",
				.display_name = "Disabled",
				.type_id	  = type_id_t<component_disabled_t>::value,
				.size		  = sizeof(component_disabled_t),
				.alignment	  = alignof(component_disabled_t),
				.flags		  = reflected_type_flag_component | reflected_type_flag_no_ui,
			});
		}

		void register_component_no_serialize_reflection(reflection_registry_t& registry)
		{
			registry.register_type({
				.name		  = "component_no_serialize",
				.display_name = "No Serialize",
				.type_id	  = type_id_t<component_no_serialize_t>::value,
				.size		  = sizeof(component_no_serialize_t),
				.alignment	  = alignof(component_no_serialize_t),
				.flags		  = reflected_type_flag_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
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
		register_component_render_object_reflection(registry);
		register_component_camera_reflection(registry);
		register_component_skybox_reflection(registry);
		register_component_prefab_reference_reflection(registry);
		register_debug_widgets_enum_reflection(registry);
		register_debug_struct_reflection(registry);
		register_component_debug_widgets_reflection(registry);
		register_component_alive_reflection(registry);
		register_component_disabled_reflection(registry);
		register_component_no_serialize_reflection(registry);
	}
}
