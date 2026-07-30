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

#include "physics_world_util.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/physics_collision_mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/PulleyConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

namespace sfg
{
#define PHYSICS_OBJECT_LAYER_MOVING_BIT (1 << 6)

	JPH::Vec3 physics_world_util_t::to_jolt(const vec3f_t& value)
	{
		return {value.x, value.y, value.z};
	}

	JPH::RVec3 physics_world_util_t::to_jolt_position(const vec3f_t& value)
	{
		return {value.x, value.y, value.z};
	}

	JPH::Quat physics_world_util_t::to_jolt(const quat_t& value)
	{
		return {value.x, value.y, value.z, value.w};
	}

	JPH::EMotionType physics_world_util_t::to_jolt(physics_motion_type_e motion_type)
	{
		switch (motion_type)
		{
		case physics_motion_type_e::static_body:
			return JPH::EMotionType::Static;
		case physics_motion_type_e::kinematic_body:
			return JPH::EMotionType::Kinematic;
		case physics_motion_type_e::dynamic_body:
			return JPH::EMotionType::Dynamic;
		}

		SFG_ASSERT(false);
		return JPH::EMotionType::Static;
	}

	JPH::EMotorState physics_world_util_t::to_jolt(physics_constraint_motor_state_e motor_state)
	{
		switch (motor_state)
		{
		case physics_constraint_motor_state_e::off:
			return JPH::EMotorState::Off;
		case physics_constraint_motor_state_e::velocity:
			return JPH::EMotorState::Velocity;
		case physics_constraint_motor_state_e::position:
			return JPH::EMotorState::Position;
		}

		SFG_ASSERT(false);
		return JPH::EMotorState::Off;
	}

	vec3f_t physics_world_util_t::from_jolt(JPH::Vec3Arg value)
	{
		return {value.GetX(), value.GetY(), value.GetZ()};
	}

	quat_t physics_world_util_t::from_jolt(JPH::QuatArg value)
	{
		return {value.GetX(), value.GetY(), value.GetZ(), value.GetW()};
	}

	JPH::ObjectLayer physics_world_util_t::make_object_layer(u8 project_layer, physics_motion_type_e motion_type)
	{
		SFG_ASSERT(project_layer < PHYSICS_COLLISION_LAYER_MAX);

		return static_cast<JPH::ObjectLayer>(project_layer | (motion_type == physics_motion_type_e::static_body ? 0 : PHYSICS_OBJECT_LAYER_MOVING_BIT));
	}

	u8 physics_world_util_t::get_project_layer(JPH::ObjectLayer layer)
	{
		return static_cast<u8>(layer & (PHYSICS_OBJECT_LAYER_MOVING_BIT - 1));
	}

	bool physics_world_util_t::is_moving_layer(JPH::ObjectLayer layer)
	{
		return (layer & PHYSICS_OBJECT_LAYER_MOVING_BIT) != 0;
	}

	JPH::RefConst<JPH::Shape> physics_world_util_t::create_shape(world_t& world, entity_id_t entity, const component_physical_t& physical, const vec3f_t& scale)
	{
		if (physical.shape == physics_shape_type_e::compound)
			return create_compound_shape(world, entity, physical, scale);

		return create_shape_from_properties(entity, physical.motion_type, physical.shape, physical.half_extent, physical.radius, physical.half_height, physical.collision_mesh, scale);
	}

	JPH::RefConst<JPH::Shape> physics_world_util_t::create_shape_from_properties(
		entity_id_t entity, physics_motion_type_e motion_type, physics_shape_type_e shape_type, const vec3f_t& half_extent, f32 radius, f32 half_height, resource_handle_t collision_mesh_handle, const vec3f_t& scale)
	{
		if (shape_type == physics_shape_type_e::mesh && motion_type != physics_motion_type_e::static_body)
		{
			SFG_ERR("mesh colliders only support static physics bodies: {0}", entity);
			return {};
		}

		const vec3f_t			  abs_scale = vec3f_t::abs(scale);
		JPH::RefConst<JPH::Shape> shape		= {};

		switch (shape_type)
		{
		case physics_shape_type_e::box: {
			JPH::BoxShapeSettings			settings(to_jolt(vec3f_t::max(half_extent * abs_scale, {JPH::cDefaultConvexRadius, JPH::cDefaultConvexRadius, JPH::cDefaultConvexRadius})));
			JPH::ShapeSettings::ShapeResult result = settings.Create();

			if (result.HasError())
				SFG_WARN("failed to create box physics shape for entity {0}: {1}", entity, result.GetError().c_str());
			else
				shape = result.Get();

			break;
		}
		case physics_shape_type_e::sphere: {
			const f32						scaled_radius = radius * math::max(abs_scale.x, math::max(abs_scale.y, abs_scale.z));
			JPH::SphereShapeSettings		settings(math::max(scaled_radius, 0.001f));
			JPH::ShapeSettings::ShapeResult result = settings.Create();

			if (result.HasError())
				SFG_WARN("failed to create sphere physics shape for entity {0}: {1}", entity, result.GetError().c_str());
			else
				shape = result.Get();

			break;
		}
		case physics_shape_type_e::capsule: {
			const f32						scaled_radius = radius * math::max(abs_scale.x, abs_scale.z);
			JPH::CapsuleShapeSettings		settings(math::max(half_height * abs_scale.y, 0.001f), math::max(scaled_radius, 0.001f));
			JPH::ShapeSettings::ShapeResult result = settings.Create();

			if (result.HasError())
				SFG_WARN("failed to create capsule physics shape for entity {0}: {1}", entity, result.GetError().c_str());
			else
				shape = result.Get();

			break;
		}
		case physics_shape_type_e::cylinder: {
			const f32						scaled_radius = radius * math::max(abs_scale.x, abs_scale.z);
			JPH::CylinderShapeSettings		settings(math::max(half_height * abs_scale.y, JPH::cDefaultConvexRadius), math::max(scaled_radius, JPH::cDefaultConvexRadius));
			JPH::ShapeSettings::ShapeResult result = settings.Create();

			if (result.HasError())
				SFG_WARN("failed to create cylinder physics shape for entity {0}: {1}", entity, result.GetError().c_str());
			else
				shape = result.Get();

			break;
		}
		case physics_shape_type_e::mesh: {
			const physics_collision_mesh_runtime_t* collision_mesh = resource_manager_t::get().find_runtime<physics_collision_mesh_runtime_t>(collision_mesh_handle);

			if (collision_mesh == nullptr)
			{
				SFG_ERR("failed to find collision mesh resource for entity {0}", entity);
				break;
			}

			JPH::Shape* mesh_shape = *resource_manager_t::get().get_memory().get<JPH::Shape*>(collision_mesh->mesh_shape);

			if (scale.equals(vec3f_t::one))
			{
				shape = mesh_shape;
			}
			else
			{
				JPH::ScaledShapeSettings		settings(mesh_shape, to_jolt(scale));
				JPH::ShapeSettings::ShapeResult result = settings.Create();

				if (result.HasError())
					SFG_ERR("failed to scale collision mesh shape for entity {0}: {1}", entity, result.GetError().c_str());
				else
					shape = result.Get();
			}

			break;
		}
		case physics_shape_type_e::compound:
			break;
		}

		return shape;
	}

	JPH::RefConst<JPH::Shape> physics_world_util_t::create_compound_shape(world_t& world, entity_id_t entity, const component_physical_t& physical, const vec3f_t& scale)
	{
		const ecs_component_table_t&	 hierarchy_table	   = world.get_component_table(type_id_t<component_hierarchy_t>::value);
		const ecs_component_table_t&	 transform_table	   = world.get_component_table(type_id_t<component_transform_t>::value);
		const ecs_component_table_t&	 compound_shape_table  = world.get_component_table(type_id_t<component_compound_shape_t>::value);
		const component_hierarchy_t&	 hierarchy			   = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, entity);
		JPH::StaticCompoundShapeSettings settings			   = {};
		u32								 shape_count		   = 0;
		bool							 shape_creation_failed = false;

		for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t&	  child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
			const component_compound_shape_t* compound_shape  = ecs_helpers_t::table_find_as_const<component_compound_shape_t>(compound_shape_table, child);

			if (compound_shape != nullptr && compound_shape->shape != physics_shape_type_e::compound)
			{
				const component_transform_t& transform	 = ecs_helpers_t::table_get_as_const<component_transform_t>(transform_table, child);
				const vec3f_t				 child_scale = scale * transform.scale;
				JPH::RefConst<JPH::Shape>	 child_shape =
					create_shape_from_properties(child, physical.motion_type, compound_shape->shape, compound_shape->half_extent, compound_shape->radius, compound_shape->half_height, compound_shape->collision_mesh, child_scale);

				if (child_shape != nullptr)
				{
					const vec3f_t child_position = (transform.pos + transform.rot * (compound_shape->local_position * transform.scale)) * scale;
					const quat_t  child_rotation = transform.rot * compound_shape->local_rotation;
					settings.AddShape(to_jolt(child_position), to_jolt(child_rotation), child_shape, child);
					shape_count++;
				}
				else
				{
					shape_creation_failed = true;
				}
			}

			child = child_hierarchy.next_sibling;
		}

		if (shape_creation_failed)
			return {};

		if (shape_count == 0)
		{
			SFG_WARN("compound physical body has no immediate child compound shapes: {0}", entity);
			return {};
		}

		JPH::ShapeSettings::ShapeResult result = settings.Create();

		if (result.HasError())
		{
			SFG_WARN("failed to create compound physics shape for entity {0}: {1}", entity, result.GetError().c_str());
			return {};
		}

		return result.Get();
	}

	bool physics_world_util_t::get_constraint_create_context(world_t& world, entity_id_t entity, entity_guid_t target_guid, constraint_create_context_t& out_context)
	{
		const ecs_component_table_t&	  system_physics_table = world.get_component_table(type_id_t<component_system_physics_t>::value);
		const ecs_component_table_t&	  transform_table	   = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr && system_physics->body_id != UINT32_MAX);

		world.calculate_transform_direct(entity);
		out_context = {
			.transform = &ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity),
			.body_id   = system_physics->body_id,
			.entity	   = entity,
		};

		if (target_guid == NULL_ENTITY_GUID)
			return true;

		const entity_id_t target_entity = world.find_by_guid(target_guid);

		if (target_entity == NULL_ENTITY_ID || target_entity == entity)
			return false;

		const component_system_physics_t* target_physics = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, target_entity);

		if (target_physics == nullptr || target_physics->body_id == UINT32_MAX)
			return false;

		world.calculate_transform_direct(target_entity);
		out_context.target_transform = &ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, target_entity);
		out_context.target_body_id	 = target_physics->body_id;
		out_context.target_entity	 = target_entity;
		return true;
	}

	bool physics_world_util_t::add_two_body_constraint(JPH::PhysicsSystem& system, const constraint_create_context_t& context, system_constraint_type_e type, const JPH::TwoBodyConstraintSettings& settings, component_system_constraints_t& system_constraints)
	{
		JPH::BodyID				body_ids[2] = {JPH::BodyID(context.body_id), JPH::BodyID(context.target_body_id)};
		const i32				body_count	= context.target_entity == NULL_ENTITY_ID ? 1 : 2;
		JPH::BodyLockMultiWrite body_lock(system.GetBodyLockInterface(), body_ids, body_count);
		JPH::Body*				body = body_lock.GetBody(0);

		if (body == nullptr)
		{
			SFG_WARN("failed to lock physics body while creating constraints for entity {0}", context.entity);
			return false;
		}

		JPH::Body* target_body = body_count == 2 ? body_lock.GetBody(1) : &JPH::Body::sFixedToWorld;

		if (target_body == nullptr)
		{
			SFG_WARN("failed to lock target physics body while creating constraints for entity {0}", context.entity);
			return false;
		}

		JPH::Constraint* constraint = settings.Create(*body, *target_body);
		SFG_ASSERT(constraint != nullptr);

		system.AddConstraint(constraint);

		const u32 slot_index				 = static_cast<u32>(type);
		system_constraints.slots[slot_index] = {
			.constraint	   = constraint,
			.target_entity = context.target_entity,
		};
		system_constraints.active_mask |= static_cast<u16>(1u << slot_index);
		return true;
	}

	void physics_world_util_t::wake_constraint_bodies(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const system_constraint_slot_t& slot)
	{
		const ecs_component_table_t&	  system_physics_table = world.get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t& system_physics	   = ecs_helpers_t::table_get_as_const<component_system_physics_t>(system_physics_table, entity);
		JPH::BodyID						  body_ids[2]		   = {JPH::BodyID(system_physics.body_id), JPH::BodyID()};
		i32								  body_count		   = 1;

		if (slot.target_entity != NULL_ENTITY_ID)
		{
			const component_system_physics_t& target_physics = ecs_helpers_t::table_get_as_const<component_system_physics_t>(system_physics_table, slot.target_entity);
			body_ids[body_count++]							 = JPH::BodyID(target_physics.body_id);
		}

		system.GetBodyInterface().ActivateBodies(body_ids, body_count);
	}

	bool physics_world_util_t::create_fixed_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_fixed_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		const quat_t				 world_rotation		   = context.transform->abs_rot * component.local_rotation;
		const quat_t				 target_world_rotation = context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_rotation : component.target_rotation;
		JPH::FixedConstraintSettings settings			   = {};
		settings.mEnabled								   = component.enabled != 0;
		settings.mUserData								   = entity;
		settings.mSpace									   = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1								   = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mAxisX1								   = to_jolt(world_rotation.get_right());
		settings.mAxisY1								   = to_jolt(world_rotation.get_up());
		settings.mPoint2								   = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		settings.mAxisX2								   = to_jolt(target_world_rotation.get_right());
		settings.mAxisY2								   = to_jolt(target_world_rotation.get_up());
		return add_two_body_constraint(system, context, system_constraint_type_e::fixed, settings, system_constraints);
	}

	bool physics_world_util_t::create_distance_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_distance_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::DistanceConstraintSettings settings = {};
		settings.mEnabled						 = component.enabled != 0;
		settings.mUserData						 = entity;
		settings.mMinDistance					 = component.min_distance;
		settings.mMaxDistance					 = component.max_distance;
		settings.mLimitsSpringSettings			 = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component.spring_frequency, component.spring_damping);
		settings.mSpace							 = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1						 = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mPoint2						 = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		return add_two_body_constraint(system, context, system_constraint_type_e::distance, settings, system_constraints);
	}

	bool physics_world_util_t::create_point_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_point_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::PointConstraintSettings settings = {};
		settings.mEnabled					  = component.enabled != 0;
		settings.mUserData					  = entity;
		settings.mSpace						  = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1					  = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mPoint2					  = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		return add_two_body_constraint(system, context, system_constraint_type_e::point, settings, system_constraints);
	}

	bool physics_world_util_t::create_hinge_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_hinge_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::HingeConstraintSettings settings	= {};
		settings.mEnabled						= component.enabled != 0;
		settings.mUserData						= entity;
		settings.mLimitsMin						= math::clamp(math::degrees_to_radians(component.limit_min_degrees), -JPH::JPH_PI, 0.0f);
		settings.mLimitsMax						= math::clamp(math::degrees_to_radians(component.limit_max_degrees), 0.0f, JPH::JPH_PI);
		settings.mLimitsSpringSettings			= JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component.spring_frequency, component.spring_damping);
		settings.mMaxFrictionTorque				= component.max_friction_torque;
		settings.mMotorSettings.mSpringSettings = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component.motor_frequency, component.motor_damping);
		settings.mMotorSettings.SetTorqueLimit(component.max_motor_torque);
		settings.mSpace		  = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1	  = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mHingeAxis1  = to_jolt(context.transform->abs_rot * component.local_hinge_axis);
		settings.mNormalAxis1 = to_jolt(context.transform->abs_rot * component.local_normal_axis);
		settings.mPoint2	  = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		settings.mHingeAxis2  = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_hinge_axis : component.target_hinge_axis);
		settings.mNormalAxis2 = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_normal_axis : component.target_normal_axis);

		if (!add_two_body_constraint(system, context, system_constraint_type_e::hinge, settings, system_constraints))
			return false;

		JPH::HingeConstraint& constraint = *static_cast<JPH::HingeConstraint*>(system_constraints.slots[static_cast<u32>(system_constraint_type_e::hinge)].constraint);

		constraint.SetTargetAngularVelocity(math::degrees_to_radians(component.motor_target_velocity_degrees_per_second));
		constraint.SetTargetAngle(math::degrees_to_radians(component.motor_target_angle_degrees));
		constraint.SetMotorState(to_jolt(component.motor_state));

		return true;
	}

	bool physics_world_util_t::create_cone_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_cone_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::ConeConstraintSettings settings = {};
		settings.mEnabled					 = component.enabled != 0;
		settings.mUserData					 = entity;
		settings.mHalfConeAngle				 = math::degrees_to_radians(component.half_cone_angle_degrees);
		settings.mSpace						 = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1					 = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mTwistAxis1				 = to_jolt(context.transform->abs_rot * component.local_twist_axis);
		settings.mPoint2					 = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		settings.mTwistAxis2				 = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_twist_axis : component.target_twist_axis);
		return add_two_body_constraint(system, context, system_constraint_type_e::cone, settings, system_constraints);
	}

	bool physics_world_util_t::create_slider_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_slider_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::SliderConstraintSettings settings	= {};
		settings.mEnabled						= component.enabled != 0;
		settings.mUserData						= entity;
		settings.mLimitsMin						= component.limit_min;
		settings.mLimitsMax						= component.limit_max;
		settings.mLimitsSpringSettings			= JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component.spring_frequency, component.spring_damping);
		settings.mMaxFrictionForce				= component.max_friction_force;
		settings.mMotorSettings.mSpringSettings = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component.motor_frequency, component.motor_damping);
		settings.mMotorSettings.SetForceLimit(component.max_motor_force);
		settings.mSpace		  = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1	  = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mSliderAxis1 = to_jolt(context.transform->abs_rot * component.local_slider_axis);
		settings.mNormalAxis1 = to_jolt(context.transform->abs_rot * component.local_normal_axis);
		settings.mPoint2	  = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		settings.mSliderAxis2 = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_slider_axis : component.target_slider_axis);
		settings.mNormalAxis2 = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_normal_axis : component.target_normal_axis);

		if (!add_two_body_constraint(system, context, system_constraint_type_e::slider, settings, system_constraints))
			return false;

		JPH::SliderConstraint& constraint = *static_cast<JPH::SliderConstraint*>(system_constraints.slots[static_cast<u32>(system_constraint_type_e::slider)].constraint);

		constraint.SetTargetVelocity(component.motor_target_velocity);
		constraint.SetTargetPosition(component.motor_target_position);
		constraint.SetMotorState(to_jolt(component.motor_state));

		return true;
	}

	bool physics_world_util_t::create_swing_twist_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_swing_twist_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::SwingTwistConstraintSettings settings = {};
		settings.mEnabled						   = component.enabled != 0;
		settings.mUserData						   = entity;
		settings.mNormalHalfConeAngle			   = math::degrees_to_radians(component.normal_half_cone_angle_degrees);
		settings.mPlaneHalfConeAngle			   = math::degrees_to_radians(component.plane_half_cone_angle_degrees);
		settings.mTwistMinAngle					   = math::degrees_to_radians(component.twist_min_angle_degrees);
		settings.mTwistMaxAngle					   = math::degrees_to_radians(component.twist_max_angle_degrees);
		settings.mMaxFrictionTorque				   = component.max_friction_torque;
		settings.mSpace							   = JPH::EConstraintSpace::WorldSpace;
		settings.mPosition1						   = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mTwistAxis1					   = to_jolt(context.transform->abs_rot * component.local_twist_axis);
		settings.mPlaneAxis1					   = to_jolt(context.transform->abs_rot * component.local_plane_axis);
		settings.mPosition2						   = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		settings.mTwistAxis2					   = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_twist_axis : component.target_twist_axis);
		settings.mPlaneAxis2					   = to_jolt(context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_plane_axis : component.target_plane_axis);
		return add_two_body_constraint(system, context, system_constraint_type_e::swing_twist, settings, system_constraints);
	}

	bool physics_world_util_t::create_six_dof_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_six_dof_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		const quat_t				  world_rotation					   = context.transform->abs_rot * component.local_rotation;
		const quat_t				  target_world_rotation				   = context.target_transform != nullptr ? context.target_transform->abs_rot * component.target_rotation : component.target_rotation;
		JPH::SixDOFConstraintSettings settings							   = {};
		settings.mEnabled												   = component.enabled != 0;
		settings.mUserData												   = entity;
		settings.mSwingType												   = JPH::ESwingType::Pyramid;
		settings.mLimitMin[JPH::SixDOFConstraintSettings::TranslationX]	   = component.translation_limit_min.x;
		settings.mLimitMin[JPH::SixDOFConstraintSettings::TranslationY]	   = component.translation_limit_min.y;
		settings.mLimitMin[JPH::SixDOFConstraintSettings::TranslationZ]	   = component.translation_limit_min.z;
		settings.mLimitMax[JPH::SixDOFConstraintSettings::TranslationX]	   = component.translation_limit_max.x;
		settings.mLimitMax[JPH::SixDOFConstraintSettings::TranslationY]	   = component.translation_limit_max.y;
		settings.mLimitMax[JPH::SixDOFConstraintSettings::TranslationZ]	   = component.translation_limit_max.z;
		settings.mLimitMin[JPH::SixDOFConstraintSettings::RotationX]	   = math::degrees_to_radians(component.rotation_limit_min_degrees.x);
		settings.mLimitMin[JPH::SixDOFConstraintSettings::RotationY]	   = math::degrees_to_radians(component.rotation_limit_min_degrees.y);
		settings.mLimitMin[JPH::SixDOFConstraintSettings::RotationZ]	   = math::degrees_to_radians(component.rotation_limit_min_degrees.z);
		settings.mLimitMax[JPH::SixDOFConstraintSettings::RotationX]	   = math::degrees_to_radians(component.rotation_limit_max_degrees.x);
		settings.mLimitMax[JPH::SixDOFConstraintSettings::RotationY]	   = math::degrees_to_radians(component.rotation_limit_max_degrees.y);
		settings.mLimitMax[JPH::SixDOFConstraintSettings::RotationZ]	   = math::degrees_to_radians(component.rotation_limit_max_degrees.z);
		settings.mMaxFriction[JPH::SixDOFConstraintSettings::TranslationX] = component.max_translation_friction.x;
		settings.mMaxFriction[JPH::SixDOFConstraintSettings::TranslationY] = component.max_translation_friction.y;
		settings.mMaxFriction[JPH::SixDOFConstraintSettings::TranslationZ] = component.max_translation_friction.z;
		settings.mMaxFriction[JPH::SixDOFConstraintSettings::RotationX]	   = component.max_rotation_friction.x;
		settings.mMaxFriction[JPH::SixDOFConstraintSettings::RotationY]	   = component.max_rotation_friction.y;
		settings.mMaxFriction[JPH::SixDOFConstraintSettings::RotationZ]	   = component.max_rotation_friction.z;
		settings.mSpace													   = JPH::EConstraintSpace::WorldSpace;
		settings.mPosition1												   = to_jolt_position(context.transform->abs_mat * component.local_point);
		settings.mAxisX1												   = to_jolt(world_rotation.get_right());
		settings.mAxisY1												   = to_jolt(world_rotation.get_up());
		settings.mPosition2												   = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_point : component.target_point);
		settings.mAxisX2												   = to_jolt(target_world_rotation.get_right());
		settings.mAxisY2												   = to_jolt(target_world_rotation.get_up());
		return add_two_body_constraint(system, context, system_constraint_type_e::six_dof, settings, system_constraints);
	}

	bool physics_world_util_t::create_pulley_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_pulley_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		constraint_create_context_t context = {};

		if (!get_constraint_create_context(world, entity, component.target_entity, context))
			return false;

		JPH::PulleyConstraintSettings settings = {};
		settings.mEnabled					   = component.enabled != 0;
		settings.mUserData					   = entity;
		settings.mFixedPoint1				   = to_jolt_position(component.fixed_point);
		settings.mFixedPoint2				   = to_jolt_position(component.target_fixed_point);
		settings.mRatio						   = component.ratio;
		settings.mMinLength					   = component.min_length;
		settings.mMaxLength					   = component.max_length;
		settings.mSpace						   = JPH::EConstraintSpace::WorldSpace;
		settings.mBodyPoint1				   = to_jolt_position(context.transform->abs_mat * component.local_body_point);
		settings.mBodyPoint2				   = to_jolt_position(context.target_transform != nullptr ? context.target_transform->abs_mat * component.target_body_point : component.target_body_point);
		return add_two_body_constraint(system, context, system_constraint_type_e::pulley, settings, system_constraints);
	}

	bool physics_world_util_t::create_vehicle_constraint(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_vehicle_constraint_t& component, component_system_constraints_t& system_constraints)
	{
		const ecs_component_table_t&	  system_physics_table		  = world.get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics			  = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		const i32						  wheel_count				  = static_cast<i32>(component.wheels.size());
		f32								  engine_torque_ratio_sum	  = 0.0f;
		bool							  differential_settings_valid = !component.differentials.empty() && component.differential_limited_slip_ratio > 1.0f;

		for (const physics_vehicle_differential_t& differential : component.differentials)
		{
			const bool left_wheel_valid	 = differential.left_wheel_index == -1 || (differential.left_wheel_index >= 0 && differential.left_wheel_index < wheel_count);
			const bool right_wheel_valid = differential.right_wheel_index == -1 || (differential.right_wheel_index >= 0 && differential.right_wheel_index < wheel_count);
			const bool wheel_pair_valid	 = (differential.left_wheel_index != -1 || differential.right_wheel_index != -1) && differential.left_wheel_index != differential.right_wheel_index;

			differential_settings_valid &= left_wheel_valid && right_wheel_valid && wheel_pair_valid && differential.differential_ratio > 0.0f && differential.engine_torque_ratio >= 0.0f && differential.limited_slip_ratio > 1.0f;
			engine_torque_ratio_sum += differential.engine_torque_ratio;
		}

		differential_settings_valid &= math::abs(engine_torque_ratio_sum - 1.0f) < 1.0e-6f;

		if (system_physics == nullptr || system_physics->body_id == UINT32_MAX || system_physics->motion_type != static_cast<u8>(physics_motion_type_e::dynamic_body) || component.wheels.empty() || !differential_settings_valid)
			return false;

		JPH::VehicleConstraintSettings		   settings			   = {};
		JPH::WheeledVehicleControllerSettings* controller_settings = new JPH::WheeledVehicleControllerSettings();
		settings.mEnabled										   = component.enabled != 0;
		settings.mUserData										   = entity;
		settings.mUp											   = to_jolt(component.up);
		settings.mForward										   = to_jolt(component.forward);
		settings.mMaxPitchRollAngle								   = math::degrees_to_radians(component.max_pitch_roll_angle_degrees);
		controller_settings->mDifferentialLimitedSlipRatio		   = component.differential_limited_slip_ratio;
		controller_settings->mDifferentials.reserve(static_cast<JPH::uint>(component.differentials.size()));
		settings.mController = controller_settings;
		settings.mWheels.reserve(static_cast<JPH::uint>(component.wheels.size()));

		for (const physics_vehicle_wheel_t& wheel : component.wheels)
		{
			JPH::WheelSettingsWV* wheel_settings	 = new JPH::WheelSettingsWV();
			wheel_settings->mPosition				 = to_jolt(wheel.position);
			wheel_settings->mSuspensionDirection	 = to_jolt(wheel.suspension_direction);
			wheel_settings->mSteeringAxis			 = to_jolt(wheel.steering_axis);
			wheel_settings->mWheelUp				 = to_jolt(wheel.wheel_up);
			wheel_settings->mWheelForward			 = to_jolt(wheel.wheel_forward);
			wheel_settings->mSuspensionMinLength	 = wheel.suspension_min_length;
			wheel_settings->mSuspensionMaxLength	 = wheel.suspension_max_length;
			wheel_settings->mSuspensionPreloadLength = wheel.suspension_preload_length;
			wheel_settings->mSuspensionSpring		 = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, wheel.suspension_frequency, wheel.suspension_damping);
			wheel_settings->mRadius					 = wheel.radius;
			wheel_settings->mWidth					 = wheel.width;
			wheel_settings->mInertia				 = wheel.inertia;
			wheel_settings->mAngularDamping			 = wheel.angular_damping;
			wheel_settings->mMaxSteerAngle			 = math::degrees_to_radians(wheel.max_steer_angle_degrees);
			wheel_settings->mMaxBrakeTorque			 = wheel.max_brake_torque;
			wheel_settings->mMaxHandBrakeTorque		 = wheel.max_hand_brake_torque;
			settings.mWheels.push_back(wheel_settings);
		}

		for (const physics_vehicle_differential_t& differential : component.differentials)
		{
			JPH::VehicleDifferentialSettings differential_settings = {};
			differential_settings.mLeftWheel					   = differential.left_wheel_index;
			differential_settings.mRightWheel					   = differential.right_wheel_index;
			differential_settings.mDifferentialRatio			   = differential.differential_ratio;
			differential_settings.mEngineTorqueRatio			   = differential.engine_torque_ratio;
			differential_settings.mLimitedSlipRatio				   = differential.limited_slip_ratio;
			controller_settings->mDifferentials.push_back(differential_settings);
		}

		const JPH::BodyID  body_id(system_physics->body_id);
		JPH::BodyLockWrite body_lock(system.GetBodyLockInterface(), body_id);

		if (!body_lock.Succeeded())
		{
			SFG_WARN("failed to lock physics body while creating vehicle constraint for entity {0}", entity);
			return false;
		}

		JPH::VehicleConstraint* constraint = new JPH::VehicleConstraint(body_lock.GetBody(), settings);
		constraint->SetVehicleCollisionTester(new JPH::VehicleCollisionTesterRay(make_object_layer(component.collision_layer, physics_motion_type_e::dynamic_body)));
		system.AddConstraint(constraint);
		system.AddStepListener(constraint);

		const u32 slot_index				 = static_cast<u32>(system_constraint_type_e::vehicle);
		system_constraints.slots[slot_index] = {
			.constraint	   = constraint,
			.target_entity = NULL_ENTITY_ID,
		};
		system_constraints.active_mask |= static_cast<u16>(1u << slot_index);
		return true;
	}

	bool physics_world_util_t::create_constraints(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, component_system_constraints_t& system_constraints)
	{
		const ecs_component_table_t& fixed_table		= world.get_component_table(type_id_t<component_fixed_constraint_t>::value);
		const ecs_component_table_t& distance_table		= world.get_component_table(type_id_t<component_distance_constraint_t>::value);
		const ecs_component_table_t& point_table		= world.get_component_table(type_id_t<component_point_constraint_t>::value);
		const ecs_component_table_t& hinge_table		= world.get_component_table(type_id_t<component_hinge_constraint_t>::value);
		const ecs_component_table_t& cone_table			= world.get_component_table(type_id_t<component_cone_constraint_t>::value);
		const ecs_component_table_t& slider_table		= world.get_component_table(type_id_t<component_slider_constraint_t>::value);
		const ecs_component_table_t& swing_twist_table	= world.get_component_table(type_id_t<component_swing_twist_constraint_t>::value);
		const ecs_component_table_t& six_dof_table		= world.get_component_table(type_id_t<component_six_dof_constraint_t>::value);
		const ecs_component_table_t& pulley_table		= world.get_component_table(type_id_t<component_pulley_constraint_t>::value);
		const ecs_component_table_t& vehicle_table		= world.get_component_table(type_id_t<component_vehicle_constraint_t>::value);
		bool						 creation_succeeded = true;
		bool						 constraint_found	= false;

		if (const component_fixed_constraint_t* component = ecs_helpers_t::table_find_as_const<component_fixed_constraint_t>(fixed_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_fixed_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_distance_constraint_t* component = ecs_helpers_t::table_find_as_const<component_distance_constraint_t>(distance_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_distance_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_point_constraint_t* component = ecs_helpers_t::table_find_as_const<component_point_constraint_t>(point_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_point_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_hinge_constraint_t* component = ecs_helpers_t::table_find_as_const<component_hinge_constraint_t>(hinge_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_hinge_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_cone_constraint_t* component = ecs_helpers_t::table_find_as_const<component_cone_constraint_t>(cone_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_cone_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_slider_constraint_t* component = ecs_helpers_t::table_find_as_const<component_slider_constraint_t>(slider_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_slider_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_swing_twist_constraint_t* component = ecs_helpers_t::table_find_as_const<component_swing_twist_constraint_t>(swing_twist_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_swing_twist_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_six_dof_constraint_t* component = ecs_helpers_t::table_find_as_const<component_six_dof_constraint_t>(six_dof_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_six_dof_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_pulley_constraint_t* component = ecs_helpers_t::table_find_as_const<component_pulley_constraint_t>(pulley_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_pulley_constraint(world, system, entity, *component, system_constraints);
		}

		if (const component_vehicle_constraint_t* component = ecs_helpers_t::table_find_as_const<component_vehicle_constraint_t>(vehicle_table, entity))
		{
			constraint_found = true;
			creation_succeeded &= create_vehicle_constraint(world, system, entity, *component, system_constraints);
		}

		if (!creation_succeeded)
			destroy_constraints(system, system_constraints);

		return constraint_found && creation_succeeded;
	}

	void physics_world_util_t::destroy_constraints(JPH::PhysicsSystem& system, component_system_constraints_t& system_constraints)
	{
		for (u32 i = 0; i < static_cast<u32>(system_constraint_type_e::count); ++i)
		{
			const u16 slot_bit = static_cast<u16>(1u << i);

			if ((system_constraints.active_mask & slot_bit) == 0)
				continue;

			system_constraint_slot_t& slot = system_constraints.slots[i];
			SFG_ASSERT(slot.constraint != nullptr);

			if (i == static_cast<u32>(system_constraint_type_e::vehicle))
				system.RemoveStepListener(static_cast<JPH::VehicleConstraint*>(slot.constraint));

			system.RemoveConstraint(slot.constraint);
			slot = {};
		}

		system_constraints.active_mask = 0;
	}

	void physics_world_util_t::sync_constraint_properties(world_t& world, JPH::PhysicsSystem& system, entity_id_t entity, const component_system_constraints_t& system_constraints)
	{
		for (u32 i = 0; i < static_cast<u32>(system_constraint_type_e::count); ++i)
		{
			const u16 slot_bit = static_cast<u16>(1u << i);

			if ((system_constraints.active_mask & slot_bit) == 0)
				continue;

			const system_constraint_slot_t& slot = system_constraints.slots[i];
			SFG_ASSERT(slot.constraint != nullptr);

			switch (static_cast<system_constraint_type_e>(i))
			{
			case system_constraint_type_e::fixed: {
				const ecs_component_table_t&		table	  = world.get_component_table(type_id_t<component_fixed_constraint_t>::value);
				const component_fixed_constraint_t* component = ecs_helpers_t::table_find_as_const<component_fixed_constraint_t>(table, entity);

				if (component != nullptr && slot.constraint->GetEnabled() != (component->enabled != 0))
					slot.constraint->SetEnabled(component->enabled != 0);
				break;
			}
			case system_constraint_type_e::distance: {
				const ecs_component_table_t&		   table	 = world.get_component_table(type_id_t<component_distance_constraint_t>::value);
				const component_distance_constraint_t* component = ecs_helpers_t::table_find_as_const<component_distance_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::DistanceConstraint& constraint	  = *static_cast<JPH::DistanceConstraint*>(slot.constraint);
				const f32				 min_distance = component->min_distance < 0.0f ? constraint.GetMinDistance() : component->min_distance;
				const f32				 max_distance = component->max_distance < 0.0f ? constraint.GetMaxDistance() : component->max_distance;

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (min_distance != constraint.GetMinDistance() || max_distance != constraint.GetMaxDistance())
					constraint.SetDistance(min_distance, max_distance);

				const JPH::SpringSettings& spring = constraint.GetLimitsSpringSettings();

				if (spring.mMode != JPH::ESpringMode::FrequencyAndDamping || spring.mFrequency != component->spring_frequency || spring.mDamping != component->spring_damping)
					constraint.SetLimitsSpringSettings(JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component->spring_frequency, component->spring_damping));
				break;
			}
			case system_constraint_type_e::point: {
				const ecs_component_table_t&		table	  = world.get_component_table(type_id_t<component_point_constraint_t>::value);
				const component_point_constraint_t* component = ecs_helpers_t::table_find_as_const<component_point_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::PointConstraint&		 constraint		 = *static_cast<JPH::PointConstraint*>(slot.constraint);
				const ecs_component_table_t& transform_table = world.get_component_table(type_id_t<component_system_transform_t>::value);

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				world.calculate_transform_direct(entity);

				const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);
				constraint.SetPoint1(JPH::EConstraintSpace::WorldSpace, to_jolt_position(transform.abs_mat * component->local_point));

				if (slot.target_entity == NULL_ENTITY_ID)
				{
					constraint.SetPoint2(JPH::EConstraintSpace::WorldSpace, to_jolt_position(component->target_point));
				}
				else
				{
					world.calculate_transform_direct(slot.target_entity);

					const component_system_transform_t& target_transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, slot.target_entity);
					constraint.SetPoint2(JPH::EConstraintSpace::WorldSpace, to_jolt_position(target_transform.abs_mat * component->target_point));
				}
				break;
			}
			case system_constraint_type_e::hinge: {
				const ecs_component_table_t&		table	  = world.get_component_table(type_id_t<component_hinge_constraint_t>::value);
				const component_hinge_constraint_t* component = ecs_helpers_t::table_find_as_const<component_hinge_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::HingeConstraint& constraint = *static_cast<JPH::HingeConstraint*>(slot.constraint);
				const f32			  min_angle	 = math::clamp(math::degrees_to_radians(component->limit_min_degrees), -JPH::JPH_PI, 0.0f);
				const f32			  max_angle	 = math::clamp(math::degrees_to_radians(component->limit_max_degrees), 0.0f, JPH::JPH_PI);

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (min_angle != constraint.GetLimitsMin() || max_angle != constraint.GetLimitsMax())
					constraint.SetLimits(min_angle, max_angle);

				const JPH::SpringSettings& spring = constraint.GetLimitsSpringSettings();

				if (spring.mMode != JPH::ESpringMode::FrequencyAndDamping || spring.mFrequency != component->spring_frequency || spring.mDamping != component->spring_damping)
					constraint.SetLimitsSpringSettings(JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component->spring_frequency, component->spring_damping));

				if (constraint.GetMaxFrictionTorque() != component->max_friction_torque)
					constraint.SetMaxFrictionTorque(component->max_friction_torque);

				JPH::MotorSettings&		  motor_settings = constraint.GetMotorSettings();
				const JPH::SpringSettings motor_spring(JPH::ESpringMode::FrequencyAndDamping, component->motor_frequency, component->motor_damping);
				const f32				  target_velocity		 = math::degrees_to_radians(component->motor_target_velocity_degrees_per_second);
				const f32				  unclamped_target_angle = math::degrees_to_radians(component->motor_target_angle_degrees);
				const f32				  target_angle			 = constraint.HasLimits() ? math::clamp(unclamped_target_angle, constraint.GetLimitsMin(), constraint.GetLimitsMax()) : unclamped_target_angle;
				const JPH::EMotorState	  motor_state			 = to_jolt(component->motor_state);
				bool					  motor_changed			 = false;

				if (motor_settings.mSpringSettings.mMode != motor_spring.mMode || motor_settings.mSpringSettings.mFrequency != motor_spring.mFrequency || motor_settings.mSpringSettings.mDamping != motor_spring.mDamping)
				{
					motor_settings.mSpringSettings = motor_spring;
					motor_changed				   = true;
				}

				if (motor_settings.mMinTorqueLimit != -component->max_motor_torque || motor_settings.mMaxTorqueLimit != component->max_motor_torque)
				{
					motor_settings.SetTorqueLimit(component->max_motor_torque);
					motor_changed = true;
				}

				if (constraint.GetTargetAngularVelocity() != target_velocity)
				{
					constraint.SetTargetAngularVelocity(target_velocity);
					motor_changed = true;
				}

				if (constraint.GetTargetAngle() != target_angle)
				{
					constraint.SetTargetAngle(target_angle);
					motor_changed = true;
				}

				if (constraint.GetMotorState() != motor_state)
				{
					constraint.SetMotorState(motor_state);
					motor_changed = true;
				}

				if (motor_changed)
					wake_constraint_bodies(world, system, entity, slot);

				break;
			}
			case system_constraint_type_e::cone: {
				const ecs_component_table_t&	   table	 = world.get_component_table(type_id_t<component_cone_constraint_t>::value);
				const component_cone_constraint_t* component = ecs_helpers_t::table_find_as_const<component_cone_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::ConeConstraint& constraint		 = *static_cast<JPH::ConeConstraint*>(slot.constraint);
				const f32			 half_cone_angle = math::degrees_to_radians(component->half_cone_angle_degrees);

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (!math::almost_equal(constraint.GetCosHalfConeAngle(), math::cos(half_cone_angle)))
					constraint.SetHalfConeAngle(half_cone_angle);
				break;
			}
			case system_constraint_type_e::slider: {
				const ecs_component_table_t&		 table	   = world.get_component_table(type_id_t<component_slider_constraint_t>::value);
				const component_slider_constraint_t* component = ecs_helpers_t::table_find_as_const<component_slider_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::SliderConstraint& constraint = *static_cast<JPH::SliderConstraint*>(slot.constraint);

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (component->limit_min != constraint.GetLimitsMin() || component->limit_max != constraint.GetLimitsMax())
					constraint.SetLimits(component->limit_min, component->limit_max);

				const JPH::SpringSettings& spring = constraint.GetLimitsSpringSettings();

				if (spring.mMode != JPH::ESpringMode::FrequencyAndDamping || spring.mFrequency != component->spring_frequency || spring.mDamping != component->spring_damping)
					constraint.SetLimitsSpringSettings(JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, component->spring_frequency, component->spring_damping));

				if (constraint.GetMaxFrictionForce() != component->max_friction_force)
					constraint.SetMaxFrictionForce(component->max_friction_force);

				JPH::MotorSettings&		  motor_settings = constraint.GetMotorSettings();
				const JPH::SpringSettings motor_spring(JPH::ESpringMode::FrequencyAndDamping, component->motor_frequency, component->motor_damping);
				const f32				  target_position = constraint.HasLimits() ? math::clamp(component->motor_target_position, constraint.GetLimitsMin(), constraint.GetLimitsMax()) : component->motor_target_position;
				const JPH::EMotorState	  motor_state	  = to_jolt(component->motor_state);
				bool					  motor_changed	  = false;

				if (motor_settings.mSpringSettings.mMode != motor_spring.mMode || motor_settings.mSpringSettings.mFrequency != motor_spring.mFrequency || motor_settings.mSpringSettings.mDamping != motor_spring.mDamping)
				{
					motor_settings.mSpringSettings = motor_spring;
					motor_changed				   = true;
				}

				if (motor_settings.mMinForceLimit != -component->max_motor_force || motor_settings.mMaxForceLimit != component->max_motor_force)
				{
					motor_settings.SetForceLimit(component->max_motor_force);
					motor_changed = true;
				}

				if (constraint.GetTargetVelocity() != component->motor_target_velocity)
				{
					constraint.SetTargetVelocity(component->motor_target_velocity);
					motor_changed = true;
				}

				if (constraint.GetTargetPosition() != target_position)
				{
					constraint.SetTargetPosition(target_position);
					motor_changed = true;
				}

				if (constraint.GetMotorState() != motor_state)
				{
					constraint.SetMotorState(motor_state);
					motor_changed = true;
				}

				if (motor_changed)
					wake_constraint_bodies(world, system, entity, slot);

				break;
			}
			case system_constraint_type_e::swing_twist: {
				const ecs_component_table_t&			  table		= world.get_component_table(type_id_t<component_swing_twist_constraint_t>::value);
				const component_swing_twist_constraint_t* component = ecs_helpers_t::table_find_as_const<component_swing_twist_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::SwingTwistConstraint& constraint			  = *static_cast<JPH::SwingTwistConstraint*>(slot.constraint);
				const f32				   normal_half_cone_angle = math::degrees_to_radians(component->normal_half_cone_angle_degrees);
				const f32				   plane_half_cone_angle  = math::degrees_to_radians(component->plane_half_cone_angle_degrees);
				const f32				   twist_min_angle		  = math::degrees_to_radians(component->twist_min_angle_degrees);
				const f32				   twist_max_angle		  = math::degrees_to_radians(component->twist_max_angle_degrees);

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (constraint.GetNormalHalfConeAngle() != normal_half_cone_angle)
					constraint.SetNormalHalfConeAngle(normal_half_cone_angle);

				if (constraint.GetPlaneHalfConeAngle() != plane_half_cone_angle)
					constraint.SetPlaneHalfConeAngle(plane_half_cone_angle);

				if (constraint.GetTwistMinAngle() != twist_min_angle)
					constraint.SetTwistMinAngle(twist_min_angle);

				if (constraint.GetTwistMaxAngle() != twist_max_angle)
					constraint.SetTwistMaxAngle(twist_max_angle);

				if (constraint.GetMaxFrictionTorque() != component->max_friction_torque)
					constraint.SetMaxFrictionTorque(component->max_friction_torque);
				break;
			}
			case system_constraint_type_e::six_dof: {
				const ecs_component_table_t&		  table		= world.get_component_table(type_id_t<component_six_dof_constraint_t>::value);
				const component_six_dof_constraint_t* component = ecs_helpers_t::table_find_as_const<component_six_dof_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::SixDOFConstraint& constraint	   = *static_cast<JPH::SixDOFConstraint*>(slot.constraint);
				const JPH::Vec3		   translation_min = to_jolt(component->translation_limit_min);
				const JPH::Vec3		   translation_max = to_jolt(component->translation_limit_max);
				const JPH::Vec3 rotation_min = to_jolt({math::degrees_to_radians(component->rotation_limit_min_degrees.x), math::degrees_to_radians(component->rotation_limit_min_degrees.y), math::degrees_to_radians(component->rotation_limit_min_degrees.z)});
				const JPH::Vec3 rotation_max = to_jolt({math::degrees_to_radians(component->rotation_limit_max_degrees.x), math::degrees_to_radians(component->rotation_limit_max_degrees.y), math::degrees_to_radians(component->rotation_limit_max_degrees.z)});

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (constraint.GetLimitsMin(JPH::SixDOFConstraintSettings::TranslationX) != translation_min.GetX() || constraint.GetLimitsMin(JPH::SixDOFConstraintSettings::TranslationY) != translation_min.GetY() ||
					constraint.GetLimitsMin(JPH::SixDOFConstraintSettings::TranslationZ) != translation_min.GetZ() || constraint.GetLimitsMax(JPH::SixDOFConstraintSettings::TranslationX) != translation_max.GetX() ||
					constraint.GetLimitsMax(JPH::SixDOFConstraintSettings::TranslationY) != translation_max.GetY() || constraint.GetLimitsMax(JPH::SixDOFConstraintSettings::TranslationZ) != translation_max.GetZ())
					constraint.SetTranslationLimits(translation_min, translation_max);

				if (constraint.GetLimitsMin(JPH::SixDOFConstraintSettings::RotationX) != rotation_min.GetX() || constraint.GetLimitsMin(JPH::SixDOFConstraintSettings::RotationY) != rotation_min.GetY() ||
					constraint.GetLimitsMin(JPH::SixDOFConstraintSettings::RotationZ) != rotation_min.GetZ() || constraint.GetLimitsMax(JPH::SixDOFConstraintSettings::RotationX) != rotation_max.GetX() ||
					constraint.GetLimitsMax(JPH::SixDOFConstraintSettings::RotationY) != rotation_max.GetY() || constraint.GetLimitsMax(JPH::SixDOFConstraintSettings::RotationZ) != rotation_max.GetZ())
					constraint.SetRotationLimits(rotation_min, rotation_max);

				const f32 translation_friction[3] = {component->max_translation_friction.x, component->max_translation_friction.y, component->max_translation_friction.z};
				const f32 rotation_friction[3]	  = {component->max_rotation_friction.x, component->max_rotation_friction.y, component->max_rotation_friction.z};

				for (u32 axis_index = 0; axis_index < 3; ++axis_index)
				{
					const JPH::SixDOFConstraint::EAxis translation_axis = static_cast<JPH::SixDOFConstraint::EAxis>(JPH::SixDOFConstraintSettings::TranslationX + axis_index);
					const JPH::SixDOFConstraint::EAxis rotation_axis	= static_cast<JPH::SixDOFConstraint::EAxis>(JPH::SixDOFConstraintSettings::RotationX + axis_index);

					if (constraint.GetMaxFriction(translation_axis) != translation_friction[axis_index])
						constraint.SetMaxFriction(translation_axis, translation_friction[axis_index]);

					if (constraint.GetMaxFriction(rotation_axis) != rotation_friction[axis_index])
						constraint.SetMaxFriction(rotation_axis, rotation_friction[axis_index]);
				}
				break;
			}
			case system_constraint_type_e::pulley: {
				const ecs_component_table_t&		 table	   = world.get_component_table(type_id_t<component_pulley_constraint_t>::value);
				const component_pulley_constraint_t* component = ecs_helpers_t::table_find_as_const<component_pulley_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::PulleyConstraint& constraint = *static_cast<JPH::PulleyConstraint*>(slot.constraint);
				const f32			   min_length = component->min_length < 0.0f ? constraint.GetMinLength() : component->min_length;
				const f32			   max_length = component->max_length < 0.0f ? constraint.GetMaxLength() : component->max_length;

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (min_length != constraint.GetMinLength() || max_length != constraint.GetMaxLength())
					constraint.SetLength(min_length, max_length);
				break;
			}
			case system_constraint_type_e::vehicle: {
				const ecs_component_table_t&		  table		= world.get_component_table(type_id_t<component_vehicle_constraint_t>::value);
				const component_vehicle_constraint_t* component = ecs_helpers_t::table_find_as_const<component_vehicle_constraint_t>(table, entity);

				if (component == nullptr)
					break;

				JPH::VehicleConstraint& constraint			 = *static_cast<JPH::VehicleConstraint*>(slot.constraint);
				const f32				max_pitch_roll_angle = math::degrees_to_radians(component->max_pitch_roll_angle_degrees);

				if (constraint.GetEnabled() != (component->enabled != 0))
					constraint.SetEnabled(component->enabled != 0);

				if (!math::almost_equal(constraint.GetMaxPitchRollAngle(), max_pitch_roll_angle))
					constraint.SetMaxPitchRollAngle(max_pitch_roll_angle);

				const JPH::ObjectLayer object_layer = make_object_layer(component->collision_layer, physics_motion_type_e::dynamic_body);

				if (constraint.GetVehicleCollisionTester()->GetObjectLayer() != object_layer)
					constraint.SetVehicleCollisionTester(new JPH::VehicleCollisionTesterRay(object_layer));
				break;
			}
			case system_constraint_type_e::count:
				SFG_ASSERT(false);
				break;
			}
		}
	}
}
