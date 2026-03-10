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

#include "common/size_definitions.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "game/game_max_defines.hpp"
#include "data/static_vector.hpp"
#include "animation_state.hpp"
#include "animation_transition.hpp"
#include "animation_mask.hpp"
#include "animation_state_machine.hpp"

namespace SFG
{

	struct animation_parameter
	{
		string_id	  sid		  = 0;
		pool_handle16 _next_param = {};
		f32			  value		  = 0.0f;
	};

	struct animation_pose;
	class world;
	class res_state_machine_raw;

	class animation_graph
	{

	private:
		using states_type		  = pool_allocator_gen<animation_state, u16, MAX_WORLD_ANIM_GRAPH_STATES>;
		using transitions_type	  = pool_allocator_gen<animation_transition, u16, MAX_WORLD_ANIM_GRAPH_TRANSITION>;
		using params_type		  = pool_allocator_gen<animation_parameter, u16, MAX_WORLD_ANIM_GRAPH_PARAMETER>;
		using masks_type		  = pool_allocator_gen<animation_mask, u16, MAX_WORLD_ANIM_GRAPH_MASK>;
		using state_machines_type = pool_allocator_gen<animation_state_machine, u16, MAX_WORLD_COMP_ANIMS>;
		using state_samples_type  = pool_allocator_gen<animation_state_sample, u16, MAX_WORLD_ANIM_GRAPH_STATE_SAMPLES>;

	public:
		animation_graph();
		~animation_graph();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void tick(world& w, f32 dt);

		// -----------------------------------------------------------------------------
		// state/transition/parameter management
		// -----------------------------------------------------------------------------

		pool_handle16			 create_state_machine_from_raw(world& w, const res_state_machine_raw& r);
		pool_handle16			 add_state_machine();
		pool_handle16			 add_state(pool_handle16 state_machine_handle, const char* name);
		pool_handle16			 get_state_handle(pool_handle16 state_machine_handle, const char* name);
		pool_handle16			 add_state(pool_handle16 state_machine_handle, const char* name, pool_handle16 mask, pool_handle16 blend_param_x, pool_handle16 blend_param_y, f32 duration, u8 flags);
		pool_handle16			 add_state_sample(pool_handle16 state_handle, resource_handle animation, const vector2& blend_point);
		pool_handle16			 add_state_sample(pool_handle16 state_handle);
		pool_handle16			 add_parameter(pool_handle16 state_machine_handle, const char* name, f32 val = 0.0f);
		pool_handle16			 get_parameter_handle(pool_handle16 state_machine_handle, const char* name);
		pool_handle16			 add_transition(pool_handle16 from_state_handle, pool_handle16 to_state_handle);
		pool_handle16			 add_transition(pool_handle16 from_state_handle, pool_handle16 to_state_handle, pool_handle16 param_handle, f32 duration, f32 target_value, animation_transition_compare compare, u8 priority);
		pool_handle16			 add_mask(const char* name);
		pool_handle16			 get_mask_handle(const char* name);
		void					 remove_state_machine(pool_handle16 handle);
		void					 remove_mask(pool_handle16 handle);
		animation_state&		 get_state(pool_handle16 handle);
		animation_state_machine& get_state_machine(pool_handle16 handle);
		animation_transition&	 get_transition(pool_handle16 handle);
		animation_parameter&	 get_parameter(pool_handle16 handle);
		animation_mask&			 get_mask(pool_handle16 handle);
		animation_state_sample&	 get_sample(pool_handle16 handle);
		void					 set_machine_active_state(pool_handle16 machine, pool_handle16 state);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_throttle_values(f32 max_distance_sqr, u8 max_frames)
		{
			_throttle_distance_sqr = max_distance_sqr;
			_throttle_max_frames   = max_frames;
		}

	private:
		// -----------------------------------------------------------------------------
		// machine
		// -----------------------------------------------------------------------------

		void apply_pose(world& w, const animation_state_machine& m, const animation_pose& pose);

		// -----------------------------------------------------------------------------
		// states
		// -----------------------------------------------------------------------------

		void calculate_pose_for_state(world& w, const state_samples_type& samples, animation_pose& out_pose, const animation_state& state);
		void progress_state(animation_state& state, f32 dt);
		void reset_state(animation_state& state);

		// -----------------------------------------------------------------------------
		// weights
		// -----------------------------------------------------------------------------

		void compute_state_weights_1d(const animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<f32, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights);
		void compute_state_weights_2d(const animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<f32, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights);

		// -----------------------------------------------------------------------------
		// transitions
		// -----------------------------------------------------------------------------

		bool check_transition(const animation_transition& t);
		bool is_transition_complete(const animation_transition& t);
		void reset_transition(animation_transition& t);
		f32	 progress_transition(animation_transition& t, f32 dt);

	private:
		states_type*		 _states	  = nullptr;
		transitions_type*	 _transitions = nullptr;
		params_type*		 _params	  = nullptr;
		masks_type*			 _masks		  = nullptr;
		state_machines_type* _machines	  = nullptr;
		state_samples_type*	 _samples	  = nullptr;

		f32 _throttle_distance_sqr = 3000.0f;
		u8	_throttle_max_frames   = 4;
	};
}
