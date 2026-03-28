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

#include "physics/common_physics.hpp"
#include "physics/physics_ray_result.hpp"
#include "data/vector.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollector.h>

namespace SFG
{
	class physics_world;
	class vector3;

	class physics_raycast_single : public JPH::CollisionCollector<JPH::RayCastResult, JPH::CollisionCollectorTraitsCastRay>
	{
	protected:
		virtual void AddHit(const JPH::RayCastResult& inResult) override;

	public:
		bool cast(physics_world* world, const vector3& position, const vector3& normal, f32 max_distance);

		const ray_result& get_result() const
		{
			return _result;
		}

	private:
		ray_result _result = {};
	};

	class physics_raycast_multi : public JPH::CollisionCollector<JPH::RayCastResult, JPH::CollisionCollectorTraitsCastRay>
	{
	protected:
		virtual void AddHit(const JPH::RayCastResult& inResult) override;

	public:
		bool cast(physics_world* world, const vector3& position, const vector3& normal, f32 max_distance);

		inline const vector<ray_result>& get_results() const
		{
			return _results;
		}

	private:
		vector<JPH::RayCastResult> _jph_results;
		vector<ray_result>		   _results = {};
	};

	class physics_broadphase : public JPH::CollisionCollector<JPH::BroadPhaseCastResult, JPH::CollisionCollectorTraitsCastRay>
	{
	protected:
		virtual void AddHit(const JPH::BroadPhaseCastResult& inResult) override;

	public:
		bool cast(physics_world* world, const vector3& position, const vector3& normal, f32 max_distance);

	private:
		vector<JPH::BroadPhaseCastResult> _jph_results;
		vector<ray_result>				  _results = {};
	};

}
