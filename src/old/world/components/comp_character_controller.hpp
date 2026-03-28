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

#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "physics/physics_types.hpp"
#include "math/vector3.hpp"

namespace JPH
{
	class CharacterVirtual;
}

namespace SFG
{
	class world;

	class comp_character_controller
	{
	public:
		static void reflect();

		void on_add(world& w);
		void on_remove(world& w);

		void update(world& w, f32 dt);
		void rebuild(world& w);
		void set_position(world& w, const vector3& pos);

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline JPH::CharacterVirtual* get_controller() const
		{
			return _controller;
		}

		inline f32 get_radius() const
		{
			return _radius;
		}

		inline f32 get_half_height() const
		{
			return _half_height;
		}

		inline const vector3& get_shape_offset() const
		{
			return _shape_offset;
		}

		inline void set_target_velocity(const vector3& v)
		{
			_target_velocity = v;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void create_controller(world& w);
		void destroy_controller();

	private:
		component_header	   _header			= {};
		JPH::CharacterVirtual* _controller		= nullptr;
		vector3				   _target_velocity = vector3::zero;

		vector3				  _shape_offset						  = vector3::zero;
		f32					  _radius							  = 0.4f;
		f32					  _half_height						  = 0.9f;
		f32					  _max_slope_degrees				  = 50.0f;
		f32					  _step_up							  = 0.4f;
		f32					  _step_down						  = 0.5f;
		f32					  _step_forward						  = 0.02f;
		f32					  _step_forward_test				  = 0.15f;
		f32					  _step_forward_contact_angle_degrees = 75.0f;
		f32					  _mass								  = 70.0f;
		f32					  _max_strength						  = 100.0f;
		f32					  _character_padding				  = 0.02f;
		f32					  _predictive_contact_distance		  = 0.1f;
		f32					  _penetration_recovery_speed		  = 1.0f;
		bool				  _enhanced_internal_edge_removal	  = false;
		physics_object_layers _object_layer						  = physics_object_layers::moving;
	};

	REFLECT_TYPE(comp_character_controller);
}
