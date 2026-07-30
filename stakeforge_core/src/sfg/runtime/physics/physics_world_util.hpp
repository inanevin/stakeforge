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

#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/physics/physics_types.hpp>
#include <sfg/runtime/world/system_components.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace JPH
{
	enum class EMotorState;
	class PhysicsSystem;
	class Shape;
	class TwoBodyConstraintSettings;
}

namespace sfg
{
	class world_t;
	struct component_cone_constraint_t;
	struct component_distance_constraint_t;
	struct component_fixed_constraint_t;
	struct component_hinge_constraint_t;
	struct component_physical_t;
	struct component_point_constraint_t;
	struct component_pulley_constraint_t;
	struct component_six_dof_constraint_t;
	struct component_slider_constraint_t;
	struct component_swing_twist_constraint_t;
	struct component_vehicle_constraint_t;

	class physics_world_util_t final
	{
	public:
		physics_world_util_t() = delete;

		// -----------------------------------------------------------------------------
		// shapes
		// -----------------------------------------------------------------------------

		static JPH::RefConst<JPH::Shape> create_shape(world_t& world, entity_id_t entity, const component_physical_t& physical, const vec3f_t& scale);

		// -----------------------------------------------------------------------------
		// constraints
		// -----------------------------------------------------------------------------

		static bool create_constraints(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, component_system_constraints_t& system_constraints);
		static void destroy_constraints(JPH::PhysicsSystem& system, component_system_constraints_t& system_constraints);
		static void sync_constraint_properties(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_system_constraints_t& system_constraints);

		// -----------------------------------------------------------------------------
		// conversions
		// -----------------------------------------------------------------------------

		static JPH::Vec3		to_jolt(const vec3f_t& value);
		static JPH::RVec3		to_jolt_position(const vec3f_t& value);
		static JPH::Quat		to_jolt(const quat_t& value);
		static JPH::EMotionType to_jolt(physics_motion_type_e motion_type);
		static vec3f_t			from_jolt(JPH::Vec3Arg value);
		static quat_t			from_jolt(JPH::QuatArg value);
		static JPH::ObjectLayer make_object_layer(u8 project_layer, physics_motion_type_e motion_type);
		static u8				get_project_layer(JPH::ObjectLayer layer);
		static bool				is_moving_layer(JPH::ObjectLayer layer);

	private:
		struct constraint_create_context_t
		{
			const component_system_transform_t* transform		 = nullptr;
			const component_system_transform_t* target_transform = nullptr;
			u32									body_id			 = UINT32_MAX;
			u32									target_body_id	 = UINT32_MAX;
			entity_id_t							entity			 = NULL_ENTITY_ID;
			entity_id_t							target_entity	 = NULL_ENTITY_ID;
		};

		static JPH::RefConst<JPH::Shape> create_shape_from_properties(
			entity_id_t entity, physics_motion_type_e motion_type, physics_shape_type_e shape_type, const vec3f_t& half_extent, f32 radius, f32 half_height, resource_handle_t collision_mesh, const vec3f_t& scale);
		static JPH::RefConst<JPH::Shape> create_compound_shape(world_t& world, entity_id_t entity, const component_physical_t& physical, const vec3f_t& scale);

		static JPH::EMotorState to_jolt(physics_constraint_motor_state_e motor_state);
		static bool				get_constraint_create_context(world_t& world, entity_id_t entity, entity_guid_t target_guid, constraint_create_context_t& out_context);
		static bool				add_two_body_constraint(JPH::PhysicsSystem& system, const constraint_create_context_t& context, system_constraint_type_e type, const JPH::TwoBodyConstraintSettings& settings, component_system_constraints_t& system_constraints);
		static void				wake_constraint_bodies(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const system_constraint_slot_t& slot);
		static bool				create_fixed_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_fixed_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_distance_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_distance_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_point_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_point_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_hinge_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_hinge_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_cone_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_cone_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_slider_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_slider_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_swing_twist_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_swing_twist_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_six_dof_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_six_dof_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_pulley_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_pulley_constraint_t& component, component_system_constraints_t& system_constraints);
		static bool				create_vehicle_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_vehicle_constraint_t& component, component_system_constraints_t& system_constraints);
	};
}
