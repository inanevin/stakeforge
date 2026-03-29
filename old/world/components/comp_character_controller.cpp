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

#include "comp_character_controller.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "physics/physics_world.hpp"
#include "physics/physics_convert.hpp"
#include "math/math.hpp"
#include "math/vector2.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Body/BodyFilter.h>

namespace SFG
{
	void comp_character_controller::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_character_controller>::value, 0, "component");
		m.set_title("character_controller");
		m.set_category("physics");

		m.add_field<&comp_character_controller::_radius, comp_character_controller>("radius", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_half_height, comp_character_controller>("half_height", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_shape_offset, comp_character_controller>("shape_offset", reflected_field_type::rf_vector3, "");
		m.add_field<&comp_character_controller::_max_slope_degrees, comp_character_controller>("max_slope_degrees", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_step_up, comp_character_controller>("step_up", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_step_down, comp_character_controller>("step_down", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_step_forward, comp_character_controller>("step_forward", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_step_forward_test, comp_character_controller>("step_forward_test", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_step_forward_contact_angle_degrees, comp_character_controller>("step_forward_contact_angle_degrees", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_mass, comp_character_controller>("mass", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_max_strength, comp_character_controller>("max_strength", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_character_padding, comp_character_controller>("character_padding", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_predictive_contact_distance, comp_character_controller>("predictive_contact_distance", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_penetration_recovery_speed, comp_character_controller>("penetration_recovery_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_character_controller::_enhanced_internal_edge_removal, comp_character_controller>("enhanced_internal_edge_removal", reflected_field_type::rf_bool, "");
		m.add_field<&comp_character_controller::_object_layer, comp_character_controller>("object_layer", reflected_field_type::rf_enum, "", 0.0f, 1.0f)->_enum_list = {"non_moving", "moving"};

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_character_controller* c = static_cast<comp_character_controller*>(params.object_ptr);
			if (c->_controller != nullptr)
				c->rebuild(params.w);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_character_controller* c = static_cast<comp_character_controller*>(obj);
			if (c->_controller != nullptr)
				c->rebuild(w);
			else
				c->create_controller(w);
		});
	}

	void comp_character_controller::on_add(world& w)
	{
	}

	void comp_character_controller::on_remove(world& w)
	{
		destroy_controller();
	}

	void comp_character_controller::set_position(world& w, const vector3& pos)
	{
		if (_controller == nullptr)
			create_controller(w);

		if (_controller == nullptr)
			return;

		_controller->SetPosition(JPH::RVec3(pos.x, pos.y, pos.z));
	}

	void comp_character_controller::update(world& w, f32 dt)
	{
		if (_controller == nullptr)
			create_controller(w);
		if (_controller == nullptr)
			return;

		if (dt <= 0.0f)
			return;

		physics_world& pw = w.get_physics_world();
		const vector3& g  = pw.get_gravity();

		vector3			velocity		 = _target_velocity;
		const JPH::Vec3 current_velocity = _controller->GetLinearVelocity();
		velocity.y						 = current_velocity.GetY();
		velocity += g * dt;

		_controller->SetLinearVelocity(to_jph_vec3(velocity));

		JPH::DefaultBroadPhaseLayerFilter bp_filter(pw.get_object_bp_layer_filter(), static_cast<JPH::ObjectLayer>(_object_layer));
		JPH::DefaultObjectLayerFilter	  obj_filter(pw.get_layer_filter(), static_cast<JPH::ObjectLayer>(_object_layer));
		JPH::BodyFilter					  body_filter;
		JPH::ShapeFilter				  shape_filter;

		JPH::CharacterVirtual::ExtendedUpdateSettings settings = {};
		settings.mStickToFloorStepDown						   = JPH::Vec3(0.0f, -_step_down, 0.0f);
		settings.mWalkStairsStepUp							   = JPH::Vec3(0.0f, _step_up, 0.0f);
		settings.mWalkStairsMinStepForward					   = _step_forward;
		settings.mWalkStairsStepForwardTest					   = _step_forward_test;
		settings.mWalkStairsCosAngleForwardContact			   = math::cos(math::degrees_to_radians(_step_forward_contact_angle_degrees));

		JPH::TempAllocatorImpl* allocator = pw.get_allocator();
		if (allocator)
			_controller->ExtendedUpdate(dt, to_jph_vec3(g), settings, bp_filter, obj_filter, body_filter, shape_filter, *allocator);
		entity_manager& em	 = w.get_entity_manager();
		const JPH::Vec3 jpos = JPH::Vec3(_controller->GetPosition());
		em.set_entity_position_abs(_header.entity, from_jph_vec3(jpos));
	}

	void comp_character_controller::rebuild(world& w)
	{
		destroy_controller();
		create_controller(w);
	}

	void comp_character_controller::create_controller(world& w)
	{
		if (_controller != nullptr)
			return;

		physics_world&	pw = w.get_physics_world();
		entity_manager& em = w.get_entity_manager();

		const vector3 pos	= em.get_entity_position_abs(_header.entity);
		const quat	  rot	= em.get_entity_rotation_abs(_header.entity);
		const vector3 scale = em.get_entity_scale_abs(_header.entity);

		f32 radius		= _radius * vector2(scale.x, scale.z).magnitude();
		f32 half_height = _half_height * scale.y;

		radius		= math::max(radius, 0.01f);
		half_height = math::max(half_height, 0.01f);

		JPH::CapsuleShapeSettings		capsule_settings(half_height, radius);
		JPH::ShapeSettings::ShapeResult capsule_result = capsule_settings.Create();
		JPH::RefConst<JPH::Shape>		shape_ref;
		if (!capsule_result.HasError())
			shape_ref = capsule_result.Get();

		if (shape_ref == nullptr)
		{
			JPH::CapsuleShapeSettings fallback(0.1f, 0.1f);
			shape_ref = fallback.Create().Get();
		}

		JPH::CharacterVirtualSettings settings = {};
		settings.mShape						   = shape_ref;
		settings.mMaxSlopeAngle				   = math::degrees_to_radians(_max_slope_degrees);
		settings.mMass						   = _mass;
		settings.mMaxStrength				   = _max_strength;
		settings.mCharacterPadding			   = _character_padding;
		settings.mPredictiveContactDistance	   = _predictive_contact_distance;
		settings.mPenetrationRecoverySpeed	   = _penetration_recovery_speed;
		settings.mShapeOffset				   = to_jph_vec3(_shape_offset);
		settings.mEnhancedInternalEdgeRemoval  = _enhanced_internal_edge_removal;

		_controller = new JPH::CharacterVirtual(&settings, to_jph_vec3(pos), to_jph_quat(rot), pw.get_system());
	}

	void comp_character_controller::destroy_controller()
	{
		if (_controller == nullptr)
			return;

		delete _controller;
		_controller = nullptr;
	}
}
