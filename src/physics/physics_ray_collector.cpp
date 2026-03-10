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

#include "physics_ray_collector.hpp"
#include "physics/physics_convert.hpp"
#include "world/world.hpp"
#include "physics_world.hpp"

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace SFG
{
	void physics_raycast_single::AddHit(const JPH::RayCastResult& inResult)
	{
	}

	bool physics_raycast_single::cast(physics_world* world, const vector3& position, const vector3& normal, f32 maxDistance)
	{
		_result						 = {};
		const vector3	   direction = normal * maxDistance;
		JPH::RRayCast	   ray(to_jph_vec3(position), to_jph_vec3(direction));
		JPH::RayCastResult ioHit;
		const bool		   hit = world->get_system()->GetNarrowPhaseQuery().CastRay(ray, ioHit);

		if (hit)
		{
			_result.hit_point	 = position + normal * ioHit.mFraction;
			_result.hit_entity	 = world->get_game_world().get_entity_manager().get_valid_handle_by_index(0);
			_result.hit_distance = ioHit.mFraction * normal.magnitude();
		}

		return hit;
	}

	void physics_raycast_multi::AddHit(const JPH::RayCastResult& res)
	{
		_jph_results.push_back(res);
	}

	bool physics_raycast_multi::cast(physics_world* world, const vector3& position, const vector3& normal, f32 max_distance)
	{
		_jph_results.resize(0);
		_results.resize(0);

		const vector3 direction = normal * max_distance;
		JPH::RRayCast ray(to_jph_vec3(position), to_jph_vec3(direction));

		JPH::RayCastSettings settings;
		world->get_system()->GetNarrowPhaseQuery().CastRay(ray, settings, *this);

		_results.reserve(_jph_results.size());

		for (const JPH::RayCastResult& jph_res : _jph_results)
		{
			ray_result res = {
				.hit_entity	  = world->get_game_world().get_entity_manager().get_valid_handle_by_index(0),
				.hit_point	  = position + normal * jph_res.mFraction,
				.hit_distance = jph_res.mFraction * direction.magnitude(),
			};
			_results.push_back(res);
		}

		return _results.empty();
	}

	void physics_broadphase::AddHit(const JPH::BroadPhaseCastResult& res)
	{
		_jph_results.push_back(res);
	}

	bool physics_broadphase::cast(physics_world* world, const vector3& position, const vector3& normal, f32 max_distance)
	{
		_jph_results.resize(0);
		_results.resize(0);

		const vector3 direction = normal * max_distance;
		JPH::RayCast  ray(to_jph_vec3(position), to_jph_vec3(direction));

		JPH::RayCastSettings settings;
		world->get_system()->GetBroadPhaseQuery().CastRay(ray, *this);

		for (const JPH::BroadPhaseCastResult& jph_res : _jph_results)
		{
			ray_result res = {
				.hit_entity	  = world->get_game_world().get_entity_manager().get_valid_handle_by_index(0),
				.hit_point	  = position + normal * jph_res.mFraction,
				.hit_distance = jph_res.mFraction * direction.magnitude(),
			};
			_results.push_back(res);
		}

		return _results.empty();
	}

}
