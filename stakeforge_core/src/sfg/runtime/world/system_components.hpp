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

#include <sfg/audio/audio_engine.hpp>
#include <sfg/common/type_id.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/resources/common_resources.hpp>

namespace JPH
{
	class CharacterVirtual;
	class Constraint;
	class Ragdoll;
}

namespace sfg
{
	enum class system_constraint_type_e : u8
	{
		fixed,
		distance,
		point,
		hinge,
		cone,
		slider,
		swing_twist,
		six_dof,
		pulley,
		vehicle,
		count,
	};

	struct system_constraint_slot_t
	{
		JPH::Constraint* constraint	   = nullptr;
		entity_id_t		 target_entity = NULL_ENTITY_ID;
	};

	struct component_system_transform_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_transform";

		mat4x3_t prev_abs_mat		= mat4x3_t::identity;
		mat4x3_t abs_mat			= mat4x3_t::identity;
		quat_t	 prev_abs_rot		= quat_t::identity;
		quat_t	 abs_rot			= quat_t::identity;
		vec3f_t	 prev_abs_pos		= vec3f_t::zero;
		vec3f_t	 prev_abs_scale		= vec3f_t::one;
		vec3f_t	 abs_pos			= vec3f_t::zero;
		vec3f_t	 abs_scale			= vec3f_t::one;
		bool	 snap_interpolation = false;
	};

	SFG_DEFINE_TYPE_ID(component_system_transform_t);

	struct component_system_skinned_mesh_renderer_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_skinned_mesh_renderer";

		resource_handle_t skeleton				 = NULL_RESOURCE_HANDLE;
		chunk_handle32_t  bones_handle			 = {};
		chunk_handle32_t  inverse_binds_handle	 = {};
		bool			  final_bones_calculated = false;
	};

	SFG_DEFINE_TYPE_ID(component_system_skinned_mesh_renderer_t);

	struct component_system_ragdoll_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_ragdoll";

		JPH::Ragdoll*	  ragdoll			= nullptr;
		mat4x3_t		  entity_from_root	= mat4x3_t::identity;
		aabb_t			  world_bounds		= {};
		chunk_handle32_t  frozen_local_pose = {};
		chunk_handle32_t  joint_global_pose = {};
		resource_handle_t ragdoll_resource	= NULL_RESOURCE_HANDLE;
		resource_handle_t skeleton			= NULL_RESOURCE_HANDLE;
		u32				  joint_count		= 0;
		u8				  collision_layer	= 0;
	};

	SFG_DEFINE_TYPE_ID(component_system_ragdoll_t);

	struct component_system_sprite_renderer_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_sprite_renderer";

		render_resource_handle_t texture	  = {};
		vec2f_t					 uv_start	  = vec2f_t::zero;
		vec2f_t					 uv_size	  = vec2f_t::zero;
		vec2u16_t				 texture_size = vec2u16_t::zero;
	};

	SFG_DEFINE_TYPE_ID(component_system_sprite_renderer_t);

	struct component_system_animation_player_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_animation_player";

		resource_handle_t animation	  = NULL_RESOURCE_HANDLE;
		f32				  sample_time = 0.0f;
	};

	SFG_DEFINE_TYPE_ID(component_system_animation_player_t);

	struct component_system_animation_graph_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_animation_graph";

		resource_handle_t animation_graph		 = NULL_RESOURCE_HANDLE;
		resource_handle_t skeleton				 = NULL_RESOURCE_HANDLE;
		chunk_handle32_t  initial_pose			 = {};
		chunk_handle32_t  parameters			 = {};
		chunk_handle32_t  nodes					 = {};
		f32				  accumulated_delta_time = 0.0f;
		u32				  parameter_count		 = 0;
		u32				  node_count			 = 0;
		u32				  tick_frame_count		 = 0;
		bool			  force_evaluate		 = false;
	};

	SFG_DEFINE_TYPE_ID(component_system_animation_graph_t);

	struct component_system_audio_source_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_audio_source";

		audio_voice_handle_t voice				= {};
		resource_handle_t	 audio				= NULL_RESOURCE_HANDLE;
		audio_bus_e			 bus				= audio_bus_e::sfx;
		bool				 play_requested		= false;
		bool				 resume_after_pause = false;
	};

	SFG_DEFINE_TYPE_ID(component_system_audio_source_t);

	struct component_system_canvas_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_canvas";

		u32 runtime_index = UINT32_MAX;
	};

	SFG_DEFINE_TYPE_ID(component_system_canvas_t);

	struct component_system_physics_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_physics";

		JPH::CharacterVirtual* character			= nullptr;
		quat_t				   local_rotation		= quat_t::identity;
		vec3f_t				   local_position		= vec3f_t::zero;
		vec3f_t				   last_ground_velocity = vec3f_t::zero;
		resource_handle_t	   physical_material	= NULL_RESOURCE_HANDLE;
		u32					   body_id				= UINT32_MAX;
		entity_id_t			   single_sub_entity	= NULL_ENTITY_ID;
		u8					   motion_type			= 0;
	};

	SFG_DEFINE_TYPE_ID(component_system_physics_t);

	struct component_system_constraints_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_constraints";

		system_constraint_slot_t slots[static_cast<u32>(system_constraint_type_e::count)] = {};
		u16						 active_mask											  = 0;
	};

	SFG_DEFINE_TYPE_ID(component_system_constraints_t);

	struct component_system_destroyer_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_system_destroyer";

		f32 timer	 = 0.0f;
		f32 end_time = 0.0f;
	};

	SFG_DEFINE_TYPE_ID(component_system_destroyer_t);

	struct system_component_reflection_t
	{
		system_component_reflection_t();
	};

	inline system_component_reflection_t g_reflect_system_component;

}
