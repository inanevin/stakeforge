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

#include "physics_world.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"
#include "world/world.hpp"
#include "physics/physics_convert.hpp"
#include "physics/physics_layer_filter.hpp"
#include "physics/physics_object_bp_layer_filter.hpp"
#include "physics/physics_bp_layer_interface.hpp"
#include "resources/physical_material.hpp"
#include "world/components/comp_physics.hpp"
#include "world/components/comp_character_controller.hpp"
#include "platform/time.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <regex>
#include <stdarg.h>
#include <tracy/Tracy.hpp>

#ifdef JPH_DEBUG_RENDERER
#include "physics_debug_renderer.hpp"
#endif

namespace
{
	static void trace_impl(const char* inFMT, ...)
	{
		va_list args;
		va_start(args, inFMT);

		char buffer[4096];
		vsnprintf(buffer, sizeof(buffer), inFMT, args);

		std::string formattedMessage(buffer);

		formattedMessage = std::regex_replace(formattedMessage, std::regex("\\{"), "{{");
		formattedMessage = std::regex_replace(formattedMessage, std::regex("\\}"), "}}");

		SFG_INFO(formattedMessage.c_str());

		va_end(args);
	}

#ifdef JPH_ENABLE_ASSERTS
	static bool assert_impl(const char* inExpression, const char* inMessage, const char* inFile, u32 inLine)
	{
		SFG_ASSERT(false, inExpression);
		return true;
	};

#endif

} // namespace

namespace SFG
{
	void physics_world::init()
	{
		JPH::RegisterDefaultAllocator();
		JPH::Trace = trace_impl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assert_impl);
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		_system		= new JPH::PhysicsSystem();
		_allocator	= new JPH::TempAllocatorImpl(10 * 1024 * 1024);
		_job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

		_layer_filter			= new physics_layer_filter();
		_object_bp_layer_filter = new physics_object_bp_layer_filter();
		_bp_layer_interface		= new physics_bp_layer_interface();

		const u32 cMaxBodies			 = 1024;
		const u32 cNumBodyMutexes		 = 0;
		const u32 cMaxBodyPairs			 = 1024;
		const u32 cMaxContactConstraints = 1024;
		_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *_bp_layer_interface, *_object_bp_layer_filter, *_layer_filter);
		set_gravity(vector3(0.0f, -9.81f, 0.0f));
		_added_bodies.reserve(MAX_ENTITIES);
	}

	void physics_world::uninit()
	{
		delete _job_system;
		delete _allocator;
		delete _system;
		delete _layer_filter;
		delete _object_bp_layer_filter;
		delete _bp_layer_interface;

		_layer_filter			= nullptr;
		_object_bp_layer_filter = nullptr;
		_bp_layer_interface		= nullptr;
		_job_system				= nullptr;
		_allocator				= nullptr;
		_system					= nullptr;

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void physics_world::init_simulation()
	{
		static vector<JPH::BodyID> reuse_body_ids;
		reuse_body_ids.resize(0);

		component_manager& cm = _game_world.get_comp_manager();
		cm.view<comp_physics>([this](comp_physics& c) -> comp_view_result {
			SFG_ASSERT(c.get_body() == nullptr);

			JPH::Body* body = c.create_body(_game_world);

			c.set_is_in_simulation(true);
			reuse_body_ids.push_back(body->GetID());
			return comp_view_result::cont;
		});

		if (!reuse_body_ids.empty())
			add_bodies_to_world(reuse_body_ids.data(), static_cast<u32>(reuse_body_ids.size()));

		_system->OptimizeBroadPhase();
	}

	void physics_world::uninit_simulation()
	{
		JPH::BodyInterface&		   body_interface = _system->GetBodyInterface();
		static vector<JPH::BodyID> reuse_body_ids;
		reuse_body_ids.resize(0);

		component_manager& cm = _game_world.get_comp_manager();
		cm.view<comp_physics>([this](comp_physics& c) -> comp_view_result {
			JPH::Body* body = c.get_body();
			SFG_ASSERT(body != nullptr);
			reuse_body_ids.push_back(body->GetID());
			c.set_is_in_simulation(false);
			return comp_view_result::cont;
		});

		if (!reuse_body_ids.empty())
			body_interface.RemoveBodies(reuse_body_ids.data(), static_cast<i32>(reuse_body_ids.size()));

		cm.view<comp_physics>([this](comp_physics& c) -> comp_view_result {
			c.destroy_body(_game_world);
			return comp_view_result::cont;
		});
	}

	void physics_world::simulate(f32 rate)
	{
		ZoneScoped;
		constexpr int collision_steps = 2;

#if !FIXED_FRAMERATE_ENABLED
		const f32 used_rate = PHYSICS_RATE_WITHOUT_FIXED_FRAMERATE_S;
		if (_dt_counter < used_rate)
		{
			_dt_counter += rate;
			return;
		}
		_dt_counter -= used_rate;
#else
		const f32 used_rate = rate;
#endif

		_system->Update(used_rate, collision_steps, _allocator, _job_system);

		component_manager& cm = _game_world.get_comp_manager();
		entity_manager&	   em = _game_world.get_entity_manager();

		auto& controllers = cm.underlying_pool<comp_cache<comp_character_controller, MAX_WORLD_COMP_CHARACTER_CONTROLLERS>, comp_character_controller>();
		for (comp_character_controller& c : controllers)
			c.update(_game_world, used_rate);

		auto& phys = cm.underlying_pool<comp_cache<comp_physics, MAX_WORLD_COMP_PHYSICS>, comp_physics>();
		for (comp_physics& c : phys)
		{
			JPH::Body* body = c.get_body();
			SFG_ASSERT(body != nullptr);

			const world_handle e_handle = c.get_header().entity;
			em.set_entity_position_abs(e_handle, from_jph_vec3(body->GetPosition()) - c.get_offset());
			em.set_entity_rotation_abs(e_handle, from_jph_quat(body->GetRotation()));
		}
	}

	void physics_world::add_bodies_to_world(JPH::BodyID* body_ids, u32 count)
	{
		JPH::BodyInterface&				   body_interface = _system->GetBodyInterface();
		const JPH::BodyInterface::AddState add_state	  = body_interface.AddBodiesPrepare(body_ids, static_cast<int>(count));
		body_interface.AddBodiesFinalize(body_ids, static_cast<int>(count), add_state, JPH::EActivation::Activate);

		for (u32 i = 0; i < count; i++)
			_added_bodies.push_back(body_ids[i].GetIndexAndSequenceNumber());
	}

	void physics_world::add_body_to_world(const JPH::Body& body)
	{
		SFG_ASSERT(false);
		JPH::BodyID jph_id = body.GetID();
		add_bodies_to_world(&jph_id, 1);
	}

	void physics_world::remove_body_from_world(const JPH::Body& body)
	{
		const u32 body_id = body.GetID().GetIndexAndSequenceNumber();
		auto	  it	  = std::find_if(_added_bodies.begin(), _added_bodies.end(), [body_id](u32 id) -> bool { return id == body_id; });
		if (it == _added_bodies.end())
			return;

		_added_bodies.erase(it);
		JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		body_interface.RemoveBody(body.GetID());
	}

	void physics_world::remove_bodies_from_world(JPH::BodyID* body_ids, u32 count)
	{
		JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		body_interface.RemoveBodies(body_ids, static_cast<int>(count));
	}

	JPH::Body* physics_world::create_body(physics_body_type body_type, physics_shape_type shape, const vector3& extents_or_rad_height, resource_handle mat, bool is_sensor, const vector3& pos, const quat& rot, const vector3& scale, JPH::Shape* mesh_shape)
	{
		resource_manager& rm = _game_world.get_resource_manager();

		physical_material_settings mat_settings = _default_material;

		if (!mat.is_null())
		{
			physical_material& phy_mat = rm.get_resource<physical_material>(mat);
			mat_settings			   = phy_mat.get_settings();
		}

		JPH::BodyInterface& body_interface = _system->GetBodyInterface();

		JPH::BodyCreationSettings body_settings = {};
		body_settings.mIsSensor					= is_sensor;

		JPH::Ref<JPH::Shape> shape_ref;

		if (shape == physics_shape_type::box)
		{
			JPH::BoxShapeSettings boxShapeSettings(to_jph_vec3(extents_or_rad_height * scale));
			shape_ref = boxShapeSettings.Create().Get();
		}
		else if (shape == physics_shape_type::sphere)
		{
			JPH::SphereShapeSettings sphere(extents_or_rad_height.x * scale.magnitude());
			shape_ref = sphere.Create().Get();
		}
		else if (shape == physics_shape_type::capsule)
		{
			JPH::CapsuleShapeSettings capsuleShapeSettings(extents_or_rad_height.y * 0.5f * scale.y, extents_or_rad_height.x * vector2(scale.x, scale.z).magnitude());
			shape_ref = capsuleShapeSettings.Create().Get();
		}
		else if (shape == physics_shape_type::cylinder)
		{
			JPH::CylinderShapeSettings cylinderShapeSettings(extents_or_rad_height.y * scale.y, extents_or_rad_height.x * vector2(scale.x, scale.z).magnitude());
			shape_ref = cylinderShapeSettings.Create().Get();
		}
		else if (shape == physics_shape_type::plane)
		{
			JPH::Plane p = JPH::Plane(to_jph_vec3(rot.get_up()), 0.0f);
			p			 = p.Scaled({extents_or_rad_height.x, 1.0f, extents_or_rad_height.z});
			JPH::PlaneShapeSettings plane(p);
			shape_ref = plane.Create().Get();
		}
		else if (shape == physics_shape_type::mesh)
		{
			if (mesh_shape != nullptr)
			{
				if (!scale.equals(vector3::one))
				{
					JPH::ScaledShapeSettings		scaled_settings(mesh_shape, to_jph_vec3(scale));
					JPH::ShapeSettings::ShapeResult result = scaled_settings.Create();
					if (!result.HasError())
						shape_ref = result.Get();
				}
				else
				{
					shape_ref = mesh_shape;
				}
			}
		}
		else
		{
			SFG_ASSERT(false);
		}

		if (shape_ref == nullptr)
		{
			JPH::BoxShapeSettings fallback(to_jph_vec3(extents_or_rad_height * scale));
			shape_ref = fallback.Create().Get();
		}

		body_settings.mRestitution					= mat_settings.restitution;
		body_settings.mFriction						= mat_settings.friction;
		body_settings.mAngularDamping				= mat_settings.angular_damp;
		body_settings.mLinearDamping				= mat_settings.linear_damp;
		body_settings.mOverrideMassProperties		= JPH::EOverrideMassProperties::CalculateInertia;
		body_settings.mMassPropertiesOverride.mMass = mat_settings.mass;
		body_settings.mGravityFactor				= mat_settings.gravity_multiplier;
		body_settings.mMotionType					= static_cast<JPH::EMotionType>(body_type);
		body_settings.mObjectLayer					= body_type == physics_body_type::static_body ? static_cast<u16>(physics_object_layers::non_moving) : static_cast<u16>(physics_object_layers::moving);
		body_settings.mPosition						= to_jph_vec3(pos);
		body_settings.mRotation						= to_jph_quat(rot);
		body_settings.SetShape(shape_ref);

		JPH::Body* b = nullptr;
		return body_interface.CreateBody(body_settings);
	}

	void physics_world::destroy_body(JPH::Body* body)
	{
		JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		body_interface.DestroyBody(body->GetID());
	}

	void physics_world::set_gravity(const vector3& g)
	{
		_graivty = g;
		_system->SetGravity(to_jph_vec3(g));
	}
}
