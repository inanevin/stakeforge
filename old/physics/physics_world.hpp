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

#include "data/vector.hpp"
#include "math/vector3.hpp"
#include "physics/physics_material_settings.hpp"
#include "physics/physics_types.hpp"
#include "physics/physics_layer_filter.hpp"
#include "physics/physics_object_bp_layer_filter.hpp"

#include "world/world_constants.hpp"
#include "game/game_max_defines.hpp"
#include "resources/common_resources.hpp"

#include "game/app_defines.hpp"

namespace JPH
{
	class PhysicsSystem;
	class TempAllocatorImpl;
	class JobSystem;
	class BodyID;
	class Body;
	class Shape;
}

namespace SFG
{
	class world;
	class quat;

	class physics_layer_filter;
	class physics_object_bp_layer_filter;
	class physics_bp_layer_interface;

	class physics_world
	{
	public:
		physics_world() = delete;
		physics_world(world& w) : _game_world(w){};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void init_simulation();
		void uninit_simulation();
		void simulate(f32 rate);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		void	   add_bodies_to_world(JPH::BodyID* body_ids, u32 count);
		void	   add_body_to_world(const JPH::Body& body);
		void	   remove_body_from_world(const JPH::Body& body);
		void	   remove_bodies_from_world(JPH::BodyID* body_ids, u32 count);
		JPH::Body* create_body(physics_body_type body_type, physics_shape_type shape, const vector3& extents_or_rad_height, resource_handle physical_material, bool is_sensor, const vector3& pos, const quat& rot, const vector3& scale, JPH::Shape* mesh_shape);
		void	   destroy_body(JPH::Body* body);
		void	   set_gravity(const vector3& g);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline JPH::PhysicsSystem* get_system() const
		{
			return _system;
		}

		inline JPH::TempAllocatorImpl* get_allocator() const
		{
			return _allocator;
		}

		inline const physics_layer_filter& get_layer_filter() const
		{
			return *_layer_filter;
		}

		inline const physics_object_bp_layer_filter& get_object_bp_layer_filter() const
		{
			return *_object_bp_layer_filter;
		}

		inline const vector3& get_gravity() const
		{
			return _graivty;
		}

		inline world& get_game_world()
		{
			return _game_world;
		}

		inline physical_material_settings& get_default_material()
		{
			return _default_material;
		}

	private:
		JPH::PhysicsSystem*		_system		= nullptr;
		JPH::TempAllocatorImpl* _allocator	= nullptr;
		JPH::JobSystem*			_job_system = nullptr;

		vector<u32>				   _added_bodies = {};
		world&					   _game_world;
		physical_material_settings _default_material = {};
		vector3					   _graivty			 = vector3::zero;

		physics_layer_filter*			_layer_filter			= nullptr;
		physics_object_bp_layer_filter* _object_bp_layer_filter = nullptr;
		physics_bp_layer_interface*		_bp_layer_interface		= nullptr;

#if !USE_FIXED_FRAMERATE
		f32 _dt_counter = 0.0f;
#endif
	};
}
