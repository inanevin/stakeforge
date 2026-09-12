/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/entity_tags.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	static inline constexpr u32 PHYSICS_COLLISION_LAYER_MAX = 64;

	struct physics_collision_layer_t
	{
		u8 value = 0;

		bool operator==(const physics_collision_layer_t&) const = default;
	};

	struct physics_collision_layer_mask_t
	{
		u64 bits = UINT64_MAX;

		bool operator==(const physics_collision_layer_mask_t&) const = default;
	};

	enum class physics_motion_type_e : u8
	{
		static_body,
		kinematic_body,
		dynamic_body,
	};

	enum class physics_shape_type_e : u8
	{
		box,
		sphere,
		capsule,
		cylinder,
		mesh,
		compound,
	};

	enum class physics_constraint_motor_state_e : u8
	{
		off,
		velocity,
		position,
	};

	enum physics_query_flags_e : u8
	{
		physics_query_flag_static	 = 1 << 0,
		physics_query_flag_kinematic = 1 << 1,
		physics_query_flag_dynamic	 = 1 << 2,
		physics_query_flag_sensor	 = 1 << 3,
		physics_query_flag_character = 1 << 4,
		physics_query_flag_all		 = physics_query_flag_static | physics_query_flag_kinematic | physics_query_flag_dynamic | physics_query_flag_sensor | physics_query_flag_character,
	};

	struct physics_query_filter_t
	{
		physics_collision_layer_mask_t collision_layers	 = {};
		entity_tag_mask_t			   required_any_tags = {};
		entity_tag_mask_t			   required_all_tags = {};
		entity_tag_mask_t			   excluded_tags	 = {};
		entity_id_t					   ignored_entity	 = NULL_ENTITY_ID;
		u8							   flags			 = physics_query_flag_all;
	};

	struct physics_raycast_t
	{
		vec3f_t origin	  = vec3f_t::zero;
		vec3f_t direction = vec3f_t::forward;
		f32		distance  = 0.0f;
	};

	struct physics_linecast_t
	{
		vec3f_t start = vec3f_t::zero;
		vec3f_t end	  = vec3f_t::zero;
	};

	struct physics_spherecast_t
	{
		vec3f_t origin	  = vec3f_t::zero;
		vec3f_t direction = vec3f_t::forward;
		f32		radius	  = 0.5f;
		f32		distance  = 0.0f;
	};

	struct physics_hit_t
	{
		vec3f_t			  position			= vec3f_t::zero;
		vec3f_t			  normal			= vec3f_t::zero;
		resource_handle_t physical_material = NULL_RESOURCE_HANDLE;
		entity_id_t		  entity			= NULL_ENTITY_ID;
		f32				  distance			= 0.0f;
		f32				  fraction			= 0.0f;
		u32				  sub_shape_id		= 0;
		bool			  is_sensor			= false;
		bool			  is_character		= false;
	};

	struct physics_query_result_t
	{
		u32	 hit_count = 0;
		bool overflow  = false;
	};

	enum class physics_contact_type_e : u8
	{
		begin,
		persist,
		end,
	};

	struct physics_contact_event_t
	{
		vec3f_t				   position		  = vec3f_t::zero;
		vec3f_t				   normal		  = vec3f_t::zero;
		entity_id_t			   entity_a		  = NULL_ENTITY_ID;
		entity_id_t			   entity_b		  = NULL_ENTITY_ID;
		entity_id_t			   sub_shape_a	  = NULL_ENTITY_ID;
		entity_id_t			   sub_shape_b	  = NULL_ENTITY_ID;
		f32					   penetration	  = 0.0f;
		u32					   sub_shape_id_a = 0;
		u32					   sub_shape_id_b = 0;
		physics_contact_type_e type			  = physics_contact_type_e::begin;
		bool				   is_sensor	  = false;
	};

	SFG_DEFINE_TYPE_ID(physics_motion_type_e);
	SFG_DEFINE_TYPE_ID(physics_shape_type_e);
	SFG_DEFINE_TYPE_ID(physics_constraint_motor_state_e);
}
