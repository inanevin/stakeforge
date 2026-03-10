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

#include "animation_graph.hpp"
#include "animation_pose.hpp"
#include "math/math.hpp"
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"
#include "io/log.hpp"
#include "resources/animation.hpp"
#include "resources/res_state_machine_raw.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	animation_graph::animation_graph()
	{
		_states		 = new states_type();
		_transitions = new transitions_type();
		_params		 = new params_type();
		_masks		 = new masks_type();
		_machines	 = new state_machines_type();
		_samples	 = new state_samples_type();
	}

	animation_graph::~animation_graph()
	{
		delete _states;
		delete _transitions;
		delete _params;
		delete _masks;
		delete _machines;
		delete _samples;
		_states		 = nullptr;
		_transitions = nullptr;
		_params		 = nullptr;
		_masks		 = nullptr;
		_machines	 = nullptr;
		_samples	 = nullptr;
	}

	void animation_graph::init()
	{
	}

	void animation_graph::uninit()
	{
		_states->reset();
		_masks->reset();
		_transitions->reset();
		_params->reset();
		_machines->reset();
		_samples->reset();
	}

	void animation_graph::tick(world& w, f32 dt)
	{
		ZoneScoped;

		auto& machines	  = *_machines;
		auto& transitions = *_transitions;
		auto& states	  = *_states;
		auto& samples	  = *_samples;

		animation_pose transition_blend_pose = {};
		animation_pose final_pose			 = {};

		entity_manager& em = w.get_entity_manager();

		const world_handle camera_entity = em.get_main_camera_entity();
		const world_handle camera_comp	 = em.get_main_camera_comp();

		if (camera_entity.is_null() || camera_comp.is_null())
			return;

		const comp_camera& cam		 = w.get_comp_manager().get_component<comp_camera>(camera_comp);
		const f32		   dot_limit = 0.8f - (cam.get_fov_degrees() / 90.0f); // 0.2 runway

		const vector3 camera_position = em.get_entity_position_abs(camera_entity);
		const quat	  camera_rotation = em.get_entity_rotation_abs(camera_entity);

		for (auto it = machines.handles_begin(); it != machines.handles_end(); ++it)
		{
			const pool_handle16		 machine_handle = *it;
			animation_state_machine& m				= machines.get(machine_handle);

			if (m.active_state.is_null() || m.joint_entities.size == 0 || m.entity.is_null())
				continue;

			const vector3 machine_position = em.get_entity_position_abs(m.entity);
			const f32	  dot			   = vector3::dot((machine_position - camera_position).normalized(), camera_rotation.get_forward());
			const f32	  dist_from_cam	   = vector3::distance_sqr(machine_position, camera_position);
			const bool	  dist_fails	   = dist_from_cam > _throttle_distance_sqr && m._throttle_count < _throttle_max_frames;
			const bool	  skip_pose		   = dot < dot_limit || dist_fails;
			m._throttle_count			   = dist_fails ? (m._throttle_count + 1) : 0;

			animation_state& state = states.get(m.active_state);

			// get state's pose.
			final_pose.reset();

			if (!skip_pose)
				calculate_pose_for_state(w, samples, final_pose, state);

			progress_state(state, dt * state.speed);

			pool_handle16 existing_transition_handle = m._active_transition;
			pool_handle16 target_transition_handle	 = state._first_out_transition;

			while (!target_transition_handle.is_null())
			{
				animation_transition& t = transitions.get(target_transition_handle);

				// transition fails, switch to next one.
				if (!check_transition(t))
				{
					target_transition_handle = t._next_transition;
					continue;
				}

				// transition passed, and no on-going transitions alive, make this on-going, switch  to next one.
				if (existing_transition_handle.is_null() || existing_transition_handle == target_transition_handle)
				{
					existing_transition_handle = target_transition_handle;
					target_transition_handle   = t._next_transition;
					continue;
				}

				// we have an on-going transition, and also current one pass, check if current one is less important, if so, continue to next one.
				animation_transition& existing = transitions.get(existing_transition_handle);
				if (t.priority < existing.priority)
				{
					target_transition_handle = t._next_transition;
					continue;
				}

				// we have an on-going transition, but current one is more important, reset on-going & switch to this
				reset_transition(existing);
				existing_transition_handle = target_transition_handle;
				target_transition_handle   = t._next_transition;
			}

			m._active_transition = existing_transition_handle;
			if (m._active_transition.is_null())
			{
				if (!skip_pose)
					apply_pose(w, m, final_pose);
				continue;
			}

			// Sample the target state of the active transition and blend into the current final pose using transition percentage.
			animation_transition& active	   = transitions.get(m._active_transition);
			animation_state&	  target_state = states.get(active.to_state);

			const f32 ratio = math::almost_equal(active.duration, 0.0f) ? 1.0f : progress_transition(active, dt);
			transition_blend_pose.reset();

			if (!skip_pose)
			{
				calculate_pose_for_state(w, samples, transition_blend_pose, target_state);
				final_pose.blend_from(transition_blend_pose, ratio);
				apply_pose(w, m, final_pose);
			}

			progress_state(target_state, dt * state.speed);

			// reset transition if complete & switch state
			if (math::almost_equal(ratio, 1.0f, 0.001f))
			{
				m._active_transition = {};
				set_machine_active_state(machine_handle, active.to_state);
				reset_transition(active);
			}
		}
	}

	// -----------------------------------------------------------------------------
	// state/transition/parameter management
	// -----------------------------------------------------------------------------

	pool_handle16 animation_graph::create_state_machine_from_raw(world& w, const res_state_machine_raw& r)
	{
		resource_manager& rm = w.get_resource_manager();

		const pool_handle16 sm = add_state_machine();

		for (const res_state_machine_parameter_raw& pr : r.parameters)
		{
			add_parameter(sm, pr.name.c_str(), pr.value);
		}

		for (const res_state_machine_mask_raw& sm : r.masks)
		{
			add_mask(sm.name.c_str());
		}

		for (const res_state_machine_state_raw& sr : r.states)
		{
			const pool_handle h		= add_state(sm, sr.name.c_str());
			animation_state&  state = get_state(h);
			state.speed				= sr.speed;
			state.duration			= sr.duration;
			state.flags.set(animation_state_flags_is_2d, sr.blend_type == 2);
			state.flags.set(animation_state_flags_is_1d, sr.blend_type == 1);
			state.flags.set(animation_state_flags_is_looping, sr.is_looping);
			state.blend_weight_param_x = get_parameter_handle(sm, sr.name.c_str());
			state.blend_weight_param_y = get_parameter_handle(sm, sr.name.c_str());
			state.mask				   = get_mask_handle(sr.mask.c_str());

			const u8 zero_dur = math::almost_equal(state.duration, 0.0f);

			for (const res_state_machine_sample_raw& sample : sr.samples)
			{
				const resource_handle ah = rm.get_resource_handle_by_hash<animation>(sample.animation_sid);
				add_state_sample(h, ah, sample.blend_point);

				if (zero_dur)
				{
					const animation& a		  = rm.get_resource<animation>(ah);
					const f32		 anim_dur = a.get_duration();
					state.duration			  = math::max(state.duration, anim_dur);
				}
			}
		}

		for (const res_state_machine_transition_raw& tr : r.transitions)
		{
			const pool_handle16	  h = add_transition(get_state_handle(sm, tr.from_state.c_str()), get_state_handle(sm, tr.to_state.c_str()));
			animation_transition& t = get_transition(h);
			t.target_value			= tr.target;
			t.priority				= tr.priority;
			t.duration				= tr.duration;
			t.compare				= tr.compare;
			t.parameter				= get_parameter_handle(sm, tr.parameter.c_str());
		}

		animation_state_machine& mac = get_state_machine(sm);
		mac.active_state			 = get_state_handle(sm, r.initial_state.c_str());
		return sm;
	}

	pool_handle16 animation_graph::add_state_machine()
	{
		return _machines->add();
	}

	pool_handle16 animation_graph::get_state_handle(pool_handle16 state_machine_handle, const char* name)
	{
		animation_state_machine& machine = _machines->get(state_machine_handle);

		if (machine._first_state.is_null())
		{
			return {};
		}

		pool_handle16	next = machine._first_state;
		const string_id sid	 = TO_SID(name);
		while (true)
		{
			animation_state& target = _states->get(next);
			if (target.sid == sid)
				return next;

			const pool_handle16 h = target._next_state;
			if (!h.is_null())
			{
				next = h;
				continue;
			}

			break;
		}

		return {};
	}

	pool_handle16 animation_graph::add_state(pool_handle16 state_machine_handle, const char* name)
	{
		return add_state(state_machine_handle, name, {}, {}, {}, 1.0f, animation_state_flags_is_looping);
	}

	pool_handle16 animation_graph::add_state(pool_handle16 state_machine_handle, const char* name, pool_handle16 mask, pool_handle16 blend_param_x, pool_handle16 blend_param_y, f32 duration, u8 flags)
	{
		const pool_handle16 new_state = _states->add();

		animation_state_machine& machine = _machines->get(state_machine_handle);
		if (machine._first_state.is_null())
		{
			machine._first_state = new_state;
		}
		else
		{

			pool_handle16 next = machine._first_state;
			while (true)
			{
				animation_state&	target = _states->get(next);
				const pool_handle16 h	   = target._next_state;
				if (!h.is_null())
				{
					next = h;
					continue;
				}

				target._next_state = new_state;
				break;
			}
		}

		animation_state& state	   = get_state(new_state);
		state.blend_weight_param_x = blend_param_x;
		state.blend_weight_param_y = blend_param_y;
		state.mask				   = mask;
		state.duration			   = duration;
		state.flags				   = flags;
		state.sid				   = TO_SID(name);
		return new_state;
	}

	pool_handle16 animation_graph::add_state_sample(pool_handle16 state_handle, resource_handle animation, const vector2& blend_point)
	{
		const pool_handle16 new_sample = _samples->add();

		animation_state& state = _states->get(state_handle);

		if (state._first_sample.is_null())
		{
			state._first_sample = new_sample;
		}
		else
		{

			pool_handle16 next = state._first_sample;
			while (true)
			{
				animation_state_sample& sample = get_sample(next);
				const pool_handle16		h	   = sample._next_sample;
				if (!h.is_null())
				{
					next = h;
					continue;
				}

				sample._next_sample = new_sample;
				break;
			}
		}

		animation_state_sample& smp = get_sample(new_sample);
		smp.animation				= animation;
		smp.blend_point				= blend_point;
		return new_sample;
	}

	pool_handle16 animation_graph::add_state_sample(pool_handle16 state_handle)
	{
		return add_state_sample(state_handle, {}, {});
	}

	pool_handle16 animation_graph::add_parameter(pool_handle16 state_machine_handle, const char* name, f32 val)
	{
		const pool_handle16 new_param = _params->add();

		animation_state_machine& machine = _machines->get(state_machine_handle);
		if (machine._first_parameter.is_null())
		{
			machine._first_parameter = new_param;
		}
		else
		{
			pool_handle16 next = machine._first_parameter;
			while (true)
			{
				animation_parameter& target = _params->get(next);
				const pool_handle16	 h		= target._next_param;
				if (!h.is_null())
				{
					next = h;
					continue;
				}
				target._next_param = new_param;
				break;
			}
		}

		animation_parameter& p = get_parameter(new_param);
		p.value				   = val;
		p.sid				   = TO_SID(name);

		return new_param;
	}

	pool_handle16 animation_graph::get_parameter_handle(pool_handle16 state_machine_handle, const char* name)
	{
		animation_state_machine& machine = _machines->get(state_machine_handle);
		if (machine._first_parameter.is_null())
			return {};

		const string_id sid	 = TO_SID(name);
		pool_handle16	next = machine._first_parameter;
		while (true)
		{
			animation_parameter& target = _params->get(next);
			if (target.sid == sid)
				return next;

			const pool_handle16 h = target._next_param;
			if (!h.is_null())
			{
				next = h;
				continue;
			}
			break;
		}

		return {};
	}

	pool_handle16 animation_graph::add_transition(pool_handle16 state_handle, pool_handle16 to_state_handle)
	{
		return add_transition(state_handle, to_state_handle, {}, 0.0f, 0.0f, animation_transition_compare::greater, 0);
	}

	pool_handle16 animation_graph::add_transition(pool_handle16 from_state_handle, pool_handle16 to_state_handle, pool_handle16 param_handle, f32 duration, f32 target_value, animation_transition_compare compare, u8 priority)
	{
		const pool_handle16 transition = _transitions->add();

		// add if first transition out of the state
		animation_state& state = _states->get(from_state_handle);
		pool_handle16	 next  = state._first_out_transition;
		if (next.is_null())
		{
			state._first_out_transition = transition;
		}
		else
		{
			// if not add into the chain.
			while (true)
			{
				animation_transition& target = _transitions->get(next);
				const pool_handle16	  h		 = target._next_transition;
				if (!h.is_null())
				{
					next = h;
					continue;
				}

				target._next_transition = transition;
				break;
			}
		}

		animation_transition& t = get_transition(transition);
		t.to_state				= to_state_handle;
		t.parameter				= param_handle;
		t.duration				= duration;
		t.target_value			= target_value;
		t.compare				= compare;
		t.priority				= priority;

		return transition;
	}

	pool_handle16 animation_graph::add_mask(const char* name)
	{
		const string_id sid = TO_SID(name);
		for (auto it = _masks->handles_begin(); it != _masks->handles_end(); ++it)
		{
			const pool_handle16	  h = *it;
			const animation_mask& m = get_mask(h);
			if (m.sid == sid)
				return h;
		}
		const pool_handle16 m  = _masks->add();
		animation_mask&		nm = get_mask(m);
		nm.sid				   = sid;
		return m;
	}

	pool_handle16 animation_graph::get_mask_handle(const char* name)
	{
		const string_id sid = TO_SID(name);
		for (auto it = _masks->handles_begin(); it != _masks->handles_end(); ++it)
		{
			const pool_handle16	  h = *it;
			const animation_mask& m = get_mask(h);
			if (m.sid == sid)
				return h;
		}
		return {};
	}

	void animation_graph::remove_state_machine(pool_handle16 handle)
	{
		animation_state_machine& machine = _machines->get(handle);

		// clear all parameters
		pool_handle16 target_param = machine._first_parameter;
		while (!target_param.is_null())
		{
			animation_parameter& param		= get_parameter(target_param);
			const pool_handle16	 next_param = param._next_param;
			_params->remove(target_param);
			target_param = next_param;
		}

		// clear all states
		pool_handle16 target_state = machine._first_state;
		while (!target_state.is_null())
		{
			animation_state&	state	   = get_state(target_state);
			const pool_handle16 next_state = state._next_state;

			// clear all state transitions
			pool_handle16 target_transition = state._first_out_transition;
			while (!target_transition.is_null())
			{
				animation_transition& transition	  = get_transition(target_transition);
				const pool_handle16	  next_transition = transition._next_transition;
				_transitions->remove(target_transition);
				target_transition = next_transition;
			}

			// clear all state samples
			pool_handle16 target_sample = state._first_sample;
			while (!target_sample.is_null())
			{
				animation_state_sample& sample		= get_sample(target_sample);
				const pool_handle16		next_sample = sample._next_sample;
				_samples->remove(target_sample);
				target_sample = next_sample;
			}

			_states->remove(target_state);
			target_state = next_state;
		}

		_machines->remove(handle);
	}

	void animation_graph::remove_mask(pool_handle16 handle)
	{
		_masks->remove(handle);
	}

	animation_state& animation_graph::get_state(pool_handle16 handle)
	{
		return _states->get(handle);
	}

	animation_state_machine& animation_graph::get_state_machine(pool_handle16 handle)
	{
		return _machines->get(handle);
	}

	animation_transition& animation_graph::get_transition(pool_handle16 handle)
	{
		return _transitions->get(handle);
	}

	animation_parameter& animation_graph::get_parameter(pool_handle16 handle)
	{
		return _params->get(handle);
	}

	animation_mask& animation_graph::get_mask(pool_handle16 handle)
	{
		return _masks->get(handle);
	}

	animation_state_sample& animation_graph::get_sample(pool_handle16 handle)
	{
		return _samples->get(handle);
	}

	void animation_graph::set_machine_active_state(pool_handle16 machine, pool_handle16 state)
	{
		animation_state_machine& m = _machines->get(machine);

		// reset state and it's transitions
		if (!m.active_state.is_null())
		{
			animation_state& active = _states->get(m.active_state);

			pool_handle16 transition = active._first_out_transition;
			while (!transition.is_null())
			{
				animation_transition& t = _transitions->get(transition);
				reset_transition(t);
				transition = t._next_transition;
			}

			reset_state(active);
		}

		m.active_state = state;
	}

	void animation_graph::apply_pose(world& w, const animation_state_machine& m, const animation_pose& pose)
	{
		ZoneScoped;

		entity_manager&	   em  = w.get_entity_manager();
		chunk_allocator32& aux = w.get_comp_manager().get_aux();

		if (m.joint_entities.size == 0)
			return;

		world_handle* entity_handles = aux.get<world_handle>(m.joint_entities);

		const u16		  count = pose.get_joint_count();
		const joint_pose* p		= pose.get_joint_poses();

		for (u16 i = 0; i < count; i++)
		{
			const joint_pose& jp = p[i];
			if (jp.flags == 0)
				continue;

			const world_handle entity = entity_handles[i];
			if (!em.is_valid(entity))
				continue;

			if (jp.flags.is_set(joint_pose_flags::has_position))
				em.set_entity_position(entity, jp.pos);

			if (jp.flags.is_set(joint_pose_flags::has_rotation))
				em.set_entity_rotation(entity, jp.rot);

			if (jp.flags.is_set(joint_pose_flags::has_scale))
				em.set_entity_scale(entity, jp.scale);
		}
	}

	void animation_graph::calculate_pose_for_state(world& w, const state_samples_type& samples, animation_pose& out_pose, const animation_state& state)
	{
		ZoneScoped;

		resource_manager&  rm	   = w.get_resource_manager();
		chunk_allocator32& res_aux = rm.get_aux();

		static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS> state_animations = {};
		static_vector<f32, MAX_WORLD_BLEND_STATE_ANIMS>				state_weights	 = {};

		if (state.flags.is_set(animation_state_flags_is_1d))
		{
			compute_state_weights_1d(state, state_animations, state_weights);
		}
		else if (state.flags.is_set(animation_state_flags_is_2d))
		{
			compute_state_weights_2d(state, state_animations, state_weights);
		}
		else
		{
			animation_state_sample& smp = _samples->get(state._first_sample);
			state_animations.push_back(smp.animation);
			state_weights.push_back(1.0f);
		}

		animation_mask* mask = nullptr;
		if (!state.mask.is_null())
			mask = &get_mask(state.mask);

		bool init	 = false;
		f32	 total_w = 0.0f;

		const u16 anims_size = static_cast<u16>(state_animations.size());

		animation_pose pose_i = {};

		for (u16 i = 0; i < anims_size; i++)
		{
			const f32 wi = state_weights[i];
			if (math::almost_equal(wi, 0.0f))
				continue;

			const resource_handle anim_handle = state_animations[i];

			pose_i.reset();
			pose_i.sample_from_animation(w, anim_handle, state._current_time, mask);

			if (!init)
			{
				total_w	 = wi;
				out_pose = pose_i;
				init	 = true;
			}
			else
			{
				const f32 t = wi / (total_w + wi);
				out_pose.blend_from(pose_i, t);
				total_w += wi;
			}
		}
	}

	void animation_graph::progress_state(animation_state& state, f32 dt)
	{
		if (state.flags.is_set(animation_state_flags_is_looping) && state.duration > 0.0f)
		{
			state._current_time = math::fmodf(state._current_time + dt, state.duration);
			return;
		}
		state._current_time = math::clamp(state._current_time + dt, 0.0f, state.duration);
	}

	void animation_graph::reset_state(animation_state& state)
	{
		state._current_time = 0.0f;
	}

	void animation_graph::compute_state_weights_1d(const animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<f32, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights)
	{
		ZoneScoped;

		const vector2 blend_point = vector2(get_parameter(state.blend_weight_param_x).value, get_parameter(state.blend_weight_param_y).value);
		constexpr f32 epsilon	  = 1e-10f;
		constexpr f32 epsilon2	  = epsilon * epsilon;
		const f32	  power		  = 2.0f;
		u16			  exact_index = UINT16_MAX;

		pool_handle16 sample_handle = state._first_sample;
		SFG_ASSERT(!sample_handle.is_null());

		f32 weights_sum = 0.0f;

		while (!sample_handle.is_null() && !out_anims.full())
		{
			const animation_state_sample& smp	= _samples->get(sample_handle);
			const f32					  d		= blend_point.x - smp.blend_point.x;
			const f32					  dist2 = d * d;

			if (dist2 < epsilon2)
			{
				out_anims.clear();
				out_weights.clear();
				out_anims.push_back(smp.animation);
				out_weights.push_back(1.0f);
				return;
			}

			const f32 dist	 = math::sqrt(dist2);
			const f32 denom	 = dist + epsilon;
			const f32 weight = 1.0f / (denom * denom);
			weights_sum += weight;
			out_weights.push_back(weight);
			out_anims.push_back(smp.animation);
			sample_handle = smp._next_sample;
		}

		const u16 sz	  = static_cast<u16>(out_weights.size());
		const f32 inv_sum = 1.0f / weights_sum;
		for (u16 i = 0; i < sz; ++i)
			out_weights[i] *= inv_sum;
	}

	void animation_graph::compute_state_weights_2d(const animation_state& state, static_vector<resource_handle, MAX_WORLD_BLEND_STATE_ANIMS>& out_anims, static_vector<f32, MAX_WORLD_BLEND_STATE_ANIMS>& out_weights)
	{
		ZoneScoped;

		const vector2 blend_point = vector2(get_parameter(state.blend_weight_param_x).value, get_parameter(state.blend_weight_param_y).value);
		constexpr f32 epsilon	  = 1e-10f;
		constexpr f32 epsilon2	  = epsilon * epsilon;
		const f32	  power		  = 2.0f;
		u16			  exact_index = UINT16_MAX;

		pool_handle16 sample_handle = state._first_sample;
		SFG_ASSERT(!sample_handle.is_null());

		f32 weights_sum = 0.0f;

		while (!sample_handle.is_null() && !out_anims.full())
		{
			const animation_state_sample& smp	= _samples->get(sample_handle);
			const vector2				  d		= blend_point - smp.blend_point;
			const f32					  dist2 = vector2::dot(d, d);

			if (dist2 < epsilon2)
			{
				out_anims.clear();
				out_weights.clear();
				out_anims.push_back(smp.animation);
				out_weights.push_back(1.0f);
				return;
			}

			const f32 dist	 = math::sqrt(dist2);
			const f32 denom	 = dist + epsilon;
			const f32 weight = 1.0f / (denom * denom);
			weights_sum += weight;
			out_weights.push_back(weight);
			out_anims.push_back(smp.animation);
			sample_handle = smp._next_sample;
		}

		const u16 sz	  = static_cast<u16>(out_weights.size());
		const f32 inv_sum = 1.0f / weights_sum;
		for (u16 i = 0; i < sz; ++i)
			out_weights[i] *= inv_sum;
	}

	// -----------------------------------------------------------------------------
	// transitions
	// -----------------------------------------------------------------------------

	bool animation_graph::check_transition(const animation_transition& t)
	{
		const f32 value = get_parameter(t.parameter).value;

		switch (t.compare)
		{
		case animation_transition_compare::equals:
			return math::almost_equal(value, t.target_value);
		case animation_transition_compare::not_equals:
			return !math::almost_equal(value, t.target_value);
		case animation_transition_compare::greater:
			return value > t.target_value;
		case animation_transition_compare::lesser:
			return value < t.target_value;
		case animation_transition_compare::gequals:
			return value >= t.target_value;
		case animation_transition_compare::lequals:
			return value <= t.target_value;
		}
		return false;
	}

	bool animation_graph::is_transition_complete(const animation_transition& t)
	{
		const f32 ratio = t._current_time / t.duration;
		return math::almost_equal(ratio, 1.0f);
	}

	void animation_graph::reset_transition(animation_transition& t)
	{
		t._current_time = 0.0f;
	}

	f32 animation_graph::progress_transition(animation_transition& t, f32 dt)
	{
		t._current_time = math::clamp(t._current_time + dt, 0.0f, t.duration);
		return t._current_time / t.duration;
	}
}