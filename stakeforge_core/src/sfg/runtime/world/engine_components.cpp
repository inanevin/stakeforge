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
#define reflected_field_flags_e reflected_field_flags_e_v2_compat
#define reflected_type_flags_e	reflected_type_flags_e_v2_compat
#include <sfg/reflection/reflection_container_ops.hpp>
#undef reflected_type_flags_e
#undef reflected_field_flags_e

#include <cstddef>
#include <iterator>

namespace sfg
{
	SFG_DEFINE_TYPE_ID(debug_widgets_enum);
	SFG_DEFINE_TYPE_ID(debug_widgets_enum2);

	namespace
	{
		void register_type_if_missing(const reflected_type_desc_t& desc)
		{
			reflection_registry_t& registry = reflection_registry_t::get();
			if (registry.find_type(desc.type_id) == nullptr)
				registry.register_type(desc);
		}

		void register_type_v2_if_missing(const reflected_type_descriptor_t& desc)
		{
			reflection_registry_v2& registry = reflection_registry_v2::get();
			if (registry.find_type(desc.type_id) == nullptr)
				registry.register_type(desc);
		}

		void register_component_mesh_renderer_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{
					.name		  = "mesh",
					.display_name = "Mesh",
					.tooltip	  = "Mesh resource rendered by this entity.",
					.type		  = reflected_value_type_e::mesh_handle,
					.offset		  = offsetof(component_mesh_renderer_t, mesh),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "material",
					.display_name = "Material",
					.tooltip	  = "Material resource used when drawing the mesh.",
					.type		  = reflected_value_type_e::material_handle,
					.offset		  = offsetof(component_mesh_renderer_t, material),
					.size		  = sizeof(resource_handle_t),
				},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_mesh_renderer",
				.display_name = "Mesh Renderer",
				.category	  = "Rendering",
				.type_id	  = component_mesh_renderer_t::TYPE_ID,
				.size		  = sizeof(component_mesh_renderer_t),
				.alignment	  = alignof(component_mesh_renderer_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_render_object_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{
					.name		  = "render_id",
					.display_name = "Render ID",
					.tooltip	  = "Renderer-owned object index for this entity.",
					.type		  = reflected_value_type_e::u32,
					.offset		  = offsetof(component_render_object_t, render_id),
					.size		  = sizeof(u32),
				},
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
				{
					.name		  = "fov_degrees",
					.display_name = "Field of View",
					.tooltip	  = "Vertical camera field of view in degrees.",
					.type		  = reflected_value_type_e::f32,
					.offset		  = offsetof(component_camera_t, fov_degrees),
					.size		  = sizeof(f32),
				},
				{
					.name		  = "near_plane",
					.display_name = "Near Plane",
					.tooltip	  = "Nearest visible camera depth in world units.",
					.type		  = reflected_value_type_e::f32,
					.offset		  = offsetof(component_camera_t, near_plane),
					.size		  = sizeof(f32),
				},
				{
					.name		  = "far_plane",
					.display_name = "Far Plane",
					.tooltip	  = "Farthest visible camera depth in world units.",
					.type		  = reflected_value_type_e::f32,
					.offset		  = offsetof(component_camera_t, far_plane),
					.size		  = sizeof(f32),
				},
				{
					.name		  = "priority",
					.display_name = "Priority",
					.tooltip	  = "Camera selection priority when more than one camera is active.",
					.type		  = reflected_value_type_e::i8,
					.offset		  = offsetof(component_camera_t, priority),
					.size		  = sizeof(i8),
				},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_camera",
				.display_name = "Camera",
				.category	  = "Rendering",
				.type_id	  = component_camera_t::TYPE_ID,
				.size		  = sizeof(component_camera_t),
				.alignment	  = alignof(component_camera_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_skybox_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{
					.name		  = "skybox_asset",
					.display_name = "Skybox",
					.tooltip	  = "HDR skybox resource used for the scene background and lighting.",
					.type		  = reflected_value_type_e::hdr_skybox_handle,
					.offset		  = offsetof(component_skybox_t, skybox_asset),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "intensity",
					.display_name = "Intensity",
					.tooltip	  = "Multiplier applied to skybox lighting contribution.",
					.type		  = reflected_value_type_e::f32,
					.offset		  = offsetof(component_skybox_t, intensity),
					.size		  = sizeof(f32),
				},
				{
					.name		  = "exposure",
					.display_name = "Exposure",
					.tooltip	  = "Exposure multiplier applied when sampling the skybox.",
					.type		  = reflected_value_type_e::f32,
					.offset		  = offsetof(component_skybox_t, exposure),
					.size		  = sizeof(f32),
				},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_skybox",
				.display_name = "Skybox",
				.category	  = "Rendering",
				.type_id	  = component_skybox_t::TYPE_ID,
				.size		  = sizeof(component_skybox_t),
				.alignment	  = alignof(component_skybox_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_prefab_reference_reflection()
		{
			static const reflected_field_desc_t fields[] = {
				{
					.name		  = "prefab",
					.display_name = "Prefab",
					.type		  = reflected_value_type_e::prefab_handle,
					.offset		  = offsetof(component_prefab_reference_t, prefab),
					.size		  = sizeof(resource_handle_t),
				},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "component_prefab_reference",
				.display_name = "Prefab Reference",
				.category	  = "component",
				.type_id	  = component_prefab_reference_t::TYPE_ID,
				.size		  = sizeof(component_prefab_reference_t),
				.alignment	  = alignof(component_prefab_reference_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_debug_widgets_reflection()
		{
			static const reflected_enum_value_desc_t enum8_values[] = {
				{
					.name		  = "none",
					.display_name = "None",
					.value		  = 0,
				},
				{
					.name		  = "small",
					.display_name = "Small",
					.value		  = 1,
				},
				{
					.name		  = "large",
					.display_name = "Large",
					.value		  = 2,
				},
			};
			static const reflected_enum_value_desc_t enum32_values[] = {
				{
					.name		  = "none",
					.display_name = "None",
					.value		  = 0,
				},
				{
					.name		  = "near",
					.display_name = "Near",
					.value		  = 1,
				},
				{
					.name		  = "far",
					.display_name = "Far",
					.value		  = 2,
				},
			};
			static const reflected_field_desc_t fields[] = {
				{
					.name		  = "f32_value",
					.display_name = "F32",
					.tooltip	  = "Debug reflected f32 value.",
					.type		  = reflected_value_type_e::f32,
					.offset		  = offsetof(component_debug_widgets_t, f32_value),
					.size		  = sizeof(f32),
				},
				{
					.name		  = "i32_value",
					.display_name = "I32",
					.tooltip	  = "Debug reflected i32 value.",
					.type		  = reflected_value_type_e::i32,
					.offset		  = offsetof(component_debug_widgets_t, i32_value),
					.size		  = sizeof(i32),
				},
				{
					.name		  = "u32_value",
					.display_name = "U32",
					.tooltip	  = "Debug reflected u32 value.",
					.type		  = reflected_value_type_e::u32,
					.offset		  = offsetof(component_debug_widgets_t, u32_value),
					.size		  = sizeof(u32),
				},
				{
					.name		  = "u8_value",
					.display_name = "U8",
					.tooltip	  = "Debug reflected u8 value.",
					.type		  = reflected_value_type_e::u8,
					.offset		  = offsetof(component_debug_widgets_t, u8_value),
					.size		  = sizeof(u8),
				},
				{
					.name		  = "bool8_value",
					.display_name = "Bool8",
					.tooltip	  = "Debug reflected bool8 value.",
					.type		  = reflected_value_type_e::bool8,
					.offset		  = offsetof(component_debug_widgets_t, bool8_value),
					.size		  = sizeof(u8),
				},
				{
					.name		  = "audio_handle_value",
					.display_name = "Audio Handle",
					.tooltip	  = "Debug reflected audio handle.",
					.type		  = reflected_value_type_e::audio_handle,
					.offset		  = offsetof(component_debug_widgets_t, audio_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "font_handle_value",
					.display_name = "Font Handle",
					.tooltip	  = "Debug reflected font handle.",
					.type		  = reflected_value_type_e::font_handle,
					.offset		  = offsetof(component_debug_widgets_t, font_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "mesh_handle_value",
					.display_name = "Mesh Handle",
					.tooltip	  = "Debug reflected mesh handle.",
					.type		  = reflected_value_type_e::mesh_handle,
					.offset		  = offsetof(component_debug_widgets_t, mesh_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "skeleton_handle_value",
					.display_name = "Skeleton Handle",
					.tooltip	  = "Debug reflected skeleton handle.",
					.type		  = reflected_value_type_e::skeleton_handle,
					.offset		  = offsetof(component_debug_widgets_t, skeleton_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "animation_handle_value",
					.display_name = "Animation Handle",
					.tooltip	  = "Debug reflected animation handle.",
					.type		  = reflected_value_type_e::animation_handle,
					.offset		  = offsetof(component_debug_widgets_t, animation_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "material_handle_value",
					.display_name = "Material Handle",
					.tooltip	  = "Debug reflected material handle.",
					.type		  = reflected_value_type_e::material_handle,
					.offset		  = offsetof(component_debug_widgets_t, material_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "shader_handle_value",
					.display_name = "Shader Handle",
					.tooltip	  = "Debug reflected shader handle.",
					.type		  = reflected_value_type_e::shader_handle,
					.offset		  = offsetof(component_debug_widgets_t, shader_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "texture_handle_value",
					.display_name = "Texture Handle",
					.tooltip	  = "Debug reflected texture handle.",
					.type		  = reflected_value_type_e::texture_handle,
					.offset		  = offsetof(component_debug_widgets_t, texture_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "texture_sampler_handle_value",
					.display_name = "Texture Sampler Handle",
					.tooltip	  = "Debug reflected texture sampler handle.",
					.type		  = reflected_value_type_e::texture_sampler_handle,
					.offset		  = offsetof(component_debug_widgets_t, texture_sampler_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "physical_material_handle_value",
					.display_name = "Physical Material Handle",
					.tooltip	  = "Debug reflected physical material handle.",
					.type		  = reflected_value_type_e::physical_material_handle,
					.offset		  = offsetof(component_debug_widgets_t, physical_material_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "prefab_handle_value",
					.display_name = "Prefab Handle",
					.tooltip	  = "Debug reflected prefab handle.",
					.type		  = reflected_value_type_e::prefab_handle,
					.offset		  = offsetof(component_debug_widgets_t, prefab_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "animation_state_machine_handle_value",
					.display_name = "Animation State Machine Handle",
					.tooltip	  = "Debug reflected animation state machine handle.",
					.type		  = reflected_value_type_e::animation_state_machine_handle,
					.offset		  = offsetof(component_debug_widgets_t, animation_state_machine_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "hdr_skybox_handle_value",
					.display_name = "HDR Skybox Handle",
					.tooltip	  = "Debug reflected HDR skybox handle.",
					.type		  = reflected_value_type_e::hdr_skybox_handle,
					.offset		  = offsetof(component_debug_widgets_t, hdr_skybox_handle_value),
					.size		  = sizeof(resource_handle_t),
				},
				{
					.name		  = "entity_guid_value",
					.display_name = "Entity GUID",
					.tooltip	  = "Debug reflected entity guid value.",
					.type		  = reflected_value_type_e::entity_guid,
					.offset		  = offsetof(component_debug_widgets_t, entity_guid_value),
					.size		  = sizeof(entity_guid_t),
				},
				{
					.name		  = "text_id_value",
					.display_name = "Text ID",
					.tooltip	  = "Debug reflected text id value.",
					.type		  = reflected_value_type_e::text_id,
					.offset		  = offsetof(component_debug_widgets_t, text_id_value),
					.size		  = sizeof(u32),
				},
				{
					.name		  = "quat_value",
					.display_name = "Quat",
					.tooltip	  = "Debug reflected quaternion value.",
					.type		  = reflected_value_type_e::quat,
					.offset		  = offsetof(component_debug_widgets_t, quat_value),
					.size		  = sizeof(quat_t),
				},
				{
					.enum_values  = {.data = enum8_values, .size = std::size(enum8_values)},
					.name		  = "enum8_value",
					.display_name = "Enum8",
					.tooltip	  = "Debug reflected enum8 value.",
					.type		  = reflected_value_type_e::enum8,
					.offset		  = offsetof(component_debug_widgets_t, enum8_value),
					.size		  = sizeof(u8),
				},
				{
					.enum_values  = {.data = enum32_values, .size = std::size(enum32_values)},
					.name		  = "enum32_value",
					.display_name = "Enum32",
					.tooltip	  = "Debug reflected enum32 value.",
					.type		  = reflected_value_type_e::enum32,
					.offset		  = offsetof(component_debug_widgets_t, enum32_value),
					.size		  = sizeof(u32),
				},
				{
					.name		  = "inplace_vector_value",
					.display_name = "Inplace Vector",
					.tooltip	  = "Debug reflected inplace vector value.",
					.type		  = reflected_value_type_e::inplace_vector,
					.sub_type_id  = "u32"_hs,
					.offset		  = offsetof(component_debug_widgets_t, inplace_vector_value),
					.size		  = sizeof(debug_widgets_inplace_vector_t),
					.stride		  = sizeof(u32),
					.capacity	  = 4,
				},
				{
					.name		  = "i8_value",
					.display_name = "I8",
					.tooltip	  = "Debug reflected i8 value.",
					.type		  = reflected_value_type_e::i8,
					.offset		  = offsetof(component_debug_widgets_t, i8_value),
					.size		  = sizeof(i8),
				},
			};

			register_type_if_missing({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "debug_widgets_component",
				.display_name = "Debug Widgets",
				.category	  = "Debug",
				.type_id	  = component_debug_widgets_t::TYPE_ID,
				.size		  = sizeof(component_debug_widgets_t),
				.alignment	  = alignof(component_debug_widgets_t),
				.flags		  = reflected_type_flags_component,
			});
		}

		void register_component_debug_widgets_reflection_v2()
		{
			register_type_v2_if_missing({
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

			register_type_v2_if_missing({
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

			register_type_v2_if_missing({
				.name = "quat_t",
				.fields =
					{
						{.name = "x", .offset = offsetof(quat_t, x), .type = reflected_value_type_e_v2::f32},
						{.name = "y", .offset = offsetof(quat_t, y), .type = reflected_value_type_e_v2::f32},
						{.name = "z", .offset = offsetof(quat_t, z), .type = reflected_value_type_e_v2::f32},
						{.name = "w", .offset = offsetof(quat_t, w), .type = reflected_value_type_e_v2::f32},
					},
				.type_id   = type_id_t<quat_t>::value,
				.size	   = sizeof(quat_t),
				.alignment = alignof(quat_t),
			});

			register_type_v2_if_missing({
				.name		  = "debug_widgets_component",
				.display_name = "Debug Widgets",
				.fields =
					{
						{
							.name	 = "f32_value",
							.tooltip = "Debug reflected f32 value.",
							.offset	 = offsetof(component_debug_widgets_t, f32_value),
							.type	 = reflected_value_type_e_v2::f32,
						},
						{
							.name	 = "i32_value",
							.tooltip = "Debug reflected i32 value.",
							.offset	 = offsetof(component_debug_widgets_t, i32_value),
							.type	 = reflected_value_type_e_v2::i32,
						},
						{
							.name	 = "u32_value",
							.tooltip = "Debug reflected u32 value.",
							.offset	 = offsetof(component_debug_widgets_t, u32_value),
							.type	 = reflected_value_type_e_v2::u32,
						},
						{
							.name	 = "u8_value",
							.tooltip = "Debug reflected u8 value.",
							.offset	 = offsetof(component_debug_widgets_t, u8_value),
							.type	 = reflected_value_type_e_v2::u8,
						},
						{
							.name	 = "bool8_value",
							.tooltip = "Debug reflected bool8 value.",
							.offset	 = offsetof(component_debug_widgets_t, bool8_value),
							.type	 = reflected_value_type_e_v2::boolean,
						},
						{
							.name		 = "audio_handle_value",
							.tooltip	 = "Debug reflected audio handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, audio_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "font_handle_value",
							.tooltip	 = "Debug reflected font handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, font_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "mesh_handle_value",
							.tooltip	 = "Debug reflected mesh handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, mesh_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "skeleton_handle_value",
							.tooltip	 = "Debug reflected skeleton handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, skeleton_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "animation_handle_value",
							.tooltip	 = "Debug reflected animation handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, animation_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "material_handle_value",
							.tooltip	 = "Debug reflected material handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, material_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "shader_handle_value",
							.tooltip	 = "Debug reflected shader handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, shader_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "texture_handle_value",
							.tooltip	 = "Debug reflected texture handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, texture_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "texture_sampler_handle_value",
							.tooltip	 = "Debug reflected texture sampler handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, texture_sampler_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "physical_material_handle_value",
							.tooltip	 = "Debug reflected physical material handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, physical_material_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "prefab_handle_value",
							.tooltip	 = "Debug reflected prefab handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, prefab_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "animation_state_machine_handle_value",
							.tooltip	 = "Debug reflected animation state machine handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, animation_state_machine_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "hdr_skybox_handle_value",
							.tooltip	 = "Debug reflected HDR skybox handle.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID,
							.offset		 = offsetof(component_debug_widgets_t, hdr_skybox_handle_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "entity_guid_value",
							.tooltip	 = "Debug reflected entity guid value.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID,
							.offset		 = offsetof(component_debug_widgets_t, entity_guid_value),
							.type		 = reflected_value_type_e_v2::u64,
						},
						{
							.name		 = "text_id_value",
							.tooltip	 = "Debug reflected text id value.",
							.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_WORLD_TEXT_ID,
							.offset		 = offsetof(component_debug_widgets_t, text_id_value),
							.type		 = reflected_value_type_e_v2::u32,
						},
						{
							.name		 = "quat_value",
							.tooltip	 = "Debug reflected quaternion value.",
							.sub_type_id = type_id_t<quat_t>::value,
							.offset		 = offsetof(component_debug_widgets_t, quat_value),
							.type		 = reflected_value_type_e_v2::object,
						},
						{
							.name		 = "enum8_value",
							.tooltip	 = "Debug reflected enum8 value.",
							.sub_type_id = type_id_t<debug_widgets_enum>::value,
							.offset		 = offsetof(component_debug_widgets_t, enum8_value),
							.type		 = reflected_value_type_e_v2::u8,
						},
						{
							.name		 = "enum32_value",
							.tooltip	 = "Debug reflected enum32 value.",
							.sub_type_id = type_id_t<debug_widgets_enum2>::value,
							.offset		 = offsetof(component_debug_widgets_t, enum32_value),
							.type		 = reflected_value_type_e_v2::u32,
						},
						{
							.container_ops = reflection_container_ops_t::sized_array_ops<debug_widgets_inplace_vector_t, u32, 4, &debug_widgets_inplace_vector_t::data, &debug_widgets_inplace_vector_t::size>(reflected_value_type_e_v2::u32),
							.name		   = "inplace_vector_value",
							.tooltip	   = "Debug reflected inplace vector value.",
							.offset		   = offsetof(component_debug_widgets_t, inplace_vector_value),
							.type		   = reflected_value_type_e_v2::container,
						},
						{
							.name	 = "i8_value",
							.tooltip = "Debug reflected i8 value.",
							.offset	 = offsetof(component_debug_widgets_t, i8_value),
							.type	 = reflected_value_type_e_v2::i8,
						},
					},
				.type_id   = component_debug_widgets_t::TYPE_ID,
				.size	   = sizeof(component_debug_widgets_t),
				.alignment = alignof(component_debug_widgets_t),
				.flags	   = reflected_type_flag_component,
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
				.flags		  = reflected_type_flags_component | reflected_type_flags_no_ui,
			});
		}

	}

	engine_component_reflection_t::engine_component_reflection_t()
	{
		register_component_mesh_renderer_reflection();
		register_component_render_object_reflection();
		register_component_camera_reflection();
		register_component_skybox_reflection();
		register_component_prefab_reference_reflection();
		register_component_debug_widgets_reflection();
		register_component_debug_widgets_reflection_v2();
		register_component_disabled_reflection();
	}
}
