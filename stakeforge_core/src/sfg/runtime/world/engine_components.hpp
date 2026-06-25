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

#include <sfg/common/hashing.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

#include <cstddef>

namespace sfg
{
	struct component_hierarchy_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_hierarchy"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_hierarchy";

		entity_guid_t first_child  = NULL_ENTITY_GUID;
		entity_guid_t parent	   = NULL_ENTITY_GUID;
		entity_guid_t next_sibling = NULL_ENTITY_GUID;
		entity_guid_t prev_sibling = NULL_ENTITY_GUID;
	};

	struct component_guid_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_guid"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_guid";

		entity_guid_t guid = NULL_ENTITY_GUID;
	};

	struct component_transform_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_transform"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_transform";

		vec3f_t pos	  = vec3f_t::zero;
		quat_t	rot	  = {};
		vec3f_t scale = vec3f_t::one;
	};

	struct component_name_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_name"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_name";

		u32 text_index = ECS_INVALID_INDEX;
	};

	struct component_mesh_renderer_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_mesh_renderer"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_mesh_renderer";

		resource_handle_t mesh	   = NULL_RESOURCE_HANDLE;
		resource_handle_t material = NULL_RESOURCE_HANDLE;
	};

	struct component_render_object_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_render_object"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_render_object";

		u32 render_id = 0;
	};

	struct component_camera_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_camera"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_camera";

		f32 fov_degrees = 60.0f;
		f32 near_plane	= 0.1f;
		f32 far_plane	= 1000.0f;
		i8	priority	= 0;
	};

	struct component_skybox_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_skybox"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_skybox";

		resource_handle_t skybox_asset = NULL_RESOURCE_HANDLE;
		f32				  intensity	   = 1.0f;
		f32				  exposure	   = 1.0f;
	};

	struct component_prefab_reference_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_prefab_reference"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_prefab_reference";

		resource_handle_t prefab = NULL_RESOURCE_HANDLE;
	};

	struct debug_widgets_inplace_vector_t
	{
		u32	   data[4] = {1, 2, 3, 0};
		size_t size	   = 3;
	};

	struct component_debug_widgets_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "debug_widgets_component"_hs;
		static inline constexpr const char* DEBUG_NAME = "debug_widgets_component";

		debug_widgets_inplace_vector_t inplace_vector_value					= {};
		resource_handle_t			   resource_value						= NULL_RESOURCE_HANDLE;
		resource_handle_t			   audio_handle_value					= NULL_RESOURCE_HANDLE;
		resource_handle_t			   font_handle_value					= NULL_RESOURCE_HANDLE;
		resource_handle_t			   mesh_handle_value					= NULL_RESOURCE_HANDLE;
		resource_handle_t			   skeleton_handle_value				= NULL_RESOURCE_HANDLE;
		resource_handle_t			   animation_handle_value				= NULL_RESOURCE_HANDLE;
		resource_handle_t			   material_handle_value				= NULL_RESOURCE_HANDLE;
		resource_handle_t			   shader_handle_value					= NULL_RESOURCE_HANDLE;
		resource_handle_t			   texture_handle_value					= NULL_RESOURCE_HANDLE;
		resource_handle_t			   texture_sampler_handle_value			= NULL_RESOURCE_HANDLE;
		resource_handle_t			   physical_material_handle_value		= NULL_RESOURCE_HANDLE;
		resource_handle_t			   prefab_handle_value					= NULL_RESOURCE_HANDLE;
		resource_handle_t			   animation_state_machine_handle_value = NULL_RESOURCE_HANDLE;
		resource_handle_t			   hdr_skybox_handle_value				= NULL_RESOURCE_HANDLE;
		entity_guid_t				   entity_guid_value					= NULL_ENTITY_GUID;
		quat_t						   quat_value							= {};
		f32							   f32_value							= 1.0f;
		i32							   i32_value							= -32;
		u32							   u32_value							= 32;
		u32							   text_id_value						= ECS_INVALID_INDEX;
		u32							   enum32_value							= 1;
		i8							   i8_value								= -8;
		u8							   u8_value								= 8;
		u8							   bool8_value							= 1;
		u8							   enum8_value							= 1;
	};

	struct component_alive_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_alive"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_alive";
	};

	struct component_disabled_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_disabled"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_disabled";
	};

	struct component_no_serialize_t
	{
		static inline constexpr sid_t		TYPE_ID	   = "component_no_serialize"_hs;
		static inline constexpr const char* DEBUG_NAME = "component_no_serialize";
	};

	struct engine_component_reflection_t
	{
		engine_component_reflection_t();
	};

	inline engine_component_reflection_t g_reflect_engine_component;
}
