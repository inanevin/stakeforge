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

#pragma once

#include <sfg/common/type_id.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

#include <cstddef>

namespace sfg
{
	struct component_hierarchy_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_hierarchy";

		entity_id_t first_child	 = NULL_ENTITY_ID;
		entity_id_t parent		 = NULL_ENTITY_ID;
		entity_id_t next_sibling = NULL_ENTITY_ID;
		entity_id_t prev_sibling = NULL_ENTITY_ID;
	};

	SFG_DEFINE_TYPE_ID(component_hierarchy_t);

	struct component_guid_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_guid";

		entity_guid_t guid = NULL_ENTITY_GUID;
	};

	SFG_DEFINE_TYPE_ID(component_guid_t);

	struct component_transform_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_transform";

		vec3f_t pos	  = vec3f_t::zero;
		quat_t	rot	  = {};
		vec3f_t scale = vec3f_t::one;
	};

	SFG_DEFINE_TYPE_ID(component_transform_t);

	struct component_name_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_name";

		char text[64];
	};

	SFG_DEFINE_TYPE_ID(component_name_t);

	struct component_mesh_renderer_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_mesh_renderer";

		inplace_vector_t<resource_handle_t, 16> materials = {};
		resource_handle_t						mesh	  = NULL_RESOURCE_HANDLE;
	};

	SFG_DEFINE_TYPE_ID(component_mesh_renderer_t);

	struct component_camera_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_camera";

		f32 fov_degrees = 60.0f;
		f32 near_plane	= 0.1f;
		f32 far_plane	= 1000.0f;
		i8	priority	= 0;
	};

	SFG_DEFINE_TYPE_ID(component_camera_t);

	struct component_skybox_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_skybox";

		resource_handle_t skybox_asset = NULL_RESOURCE_HANDLE;
		f32				  intensity	   = 1.0f;
		f32				  exposure	   = 1.0f;
	};

	SFG_DEFINE_TYPE_ID(component_skybox_t);

	struct component_prefab_reference_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_prefab_reference";

		resource_handle_t prefab  = NULL_RESOURCE_HANDLE;
		bool			  is_root = false;
	};

	SFG_DEFINE_TYPE_ID(component_prefab_reference_t);

	enum class debug_widgets_enum : u8
	{
		debug_widgets_enum_a,
		debug_widgets_enum_b,
	};

	enum class debug_widgets_enum2 : u32
	{
		debug_widgets_enum2_a,
		debug_widgets_enum2_b,
	};

	struct debug_struct2_t
	{
		f32 f32_value = 2.0f;
		u32 u32_value = 20;
	};

	SFG_DEFINE_TYPE_ID(debug_struct2_t);

	struct debug_struct_t
	{
		vec3f_t			vec3_value = {1.0f, 2.0f, 3.0f};
		f32				f32_value  = 1.0f;
		debug_struct2_t test	   = {};
	};

	SFG_DEFINE_TYPE_ID(debug_struct_t);

	struct component_debug_widgets_t
	{
		static inline constexpr const char* DEBUG_NAME = "debug_widgets_component";

		inplace_vector_t<u32, 4> inplace_vector_value				  = {1, 2, 3};
		debug_struct_t			 debug_struct_value					  = {};
		debug_struct2_t			 debug_struct2_value				  = {};
		resource_handle_t		 audio_handle_value					  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 font_handle_value					  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 mesh_handle_value					  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 skeleton_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 animation_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 material_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 shader_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 texture_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 texture_sampler_handle_value		  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 physical_material_handle_value		  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 prefab_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 animation_state_machine_handle_value = NULL_RESOURCE_HANDLE;
		resource_handle_t		 hdr_skybox_handle_value			  = NULL_RESOURCE_HANDLE;
		entity_guid_t			 entity_guid_value					  = NULL_ENTITY_GUID;
		quat_t					 quat_value							  = {};
		color_t					 color_value						  = color_t::red;
		f32						 f32_value							  = 1.0f;
		i32						 i32_value							  = -32;
		u32						 u32_value							  = 32;
		u32						 text_id_value						  = ECS_INVALID_INDEX;
		u32						 enum32_value						  = 1;
		i8						 i8_value							  = -8;
		u8						 u8_value							  = 8;
		u8						 bool8_value						  = 1;
		u8						 enum8_value						  = 1;
	};

	SFG_DEFINE_TYPE_ID(component_debug_widgets_t);

	struct component_alive_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_alive";
	};

	SFG_DEFINE_TYPE_ID(component_alive_t);

	struct component_disabled_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_disabled";
	};

	SFG_DEFINE_TYPE_ID(component_disabled_t);

	struct component_no_serialize_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_no_serialize";
	};

	SFG_DEFINE_TYPE_ID(component_no_serialize_t);

	struct engine_component_reflection_t
	{
		engine_component_reflection_t();
	};

	inline engine_component_reflection_t g_reflect_engine_component;
}
