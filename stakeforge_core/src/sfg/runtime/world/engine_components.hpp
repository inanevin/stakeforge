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
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
	struct component_hierarchy_t
	{
		static inline constexpr sid_t TYPE_ID = "component_hierarchy"_hs;

		entity_id_t first_child	 = NULL_ENTITY_ID;
		entity_id_t parent		 = NULL_ENTITY_ID;
		entity_id_t next_sibling = NULL_ENTITY_ID;
		entity_id_t prev_sibling = NULL_ENTITY_ID;
	};

	struct component_transform_t
	{
		static inline constexpr sid_t TYPE_ID = "component_transform"_hs;

		vec3f_t pos	  = vec3f_t::zero;
		quat_t	rot	  = {};
		vec3f_t scale = vec3f_t::one;
	};

	struct component_mesh_renderer_t
	{
		static inline constexpr sid_t TYPE_ID = "component_mesh_renderer"_hs;

		resource_handle_t mesh	   = NULL_RESOURCE_HANDLE;
		resource_handle_t material = NULL_RESOURCE_HANDLE;
	};

	struct component_render_object_t
	{
		static inline constexpr sid_t TYPE_ID = "component_render_object"_hs;

		u32 render_id = 0;
	};

	struct component_camera_t
	{
		static inline constexpr sid_t TYPE_ID = "component_camera"_hs;

		f32 fov_degrees = 60.0f;
		f32 near_plane	= 0.1f;
		f32 far_plane	= 1000.0f;
		i8	priority	= 0;
	};

	struct component_alive_t
	{
		static inline constexpr sid_t TYPE_ID = "component_alive"_hs;
	};

	struct component_disabled_t
	{
		static inline constexpr sid_t TYPE_ID = "component_disabled"_hs;
	};

	struct component_no_serialize_t
	{
		static inline constexpr sid_t TYPE_ID = "component_no_serialize"_hs;
	};

	struct engine_component_reflection_t
	{
		engine_component_reflection_t();
	};

	inline engine_component_reflection_t g_reflect_engine_component;
}
