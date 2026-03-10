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

#include "comp_ssao.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

namespace SFG
{
	void comp_ssao::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_ssao>::value, 0, "component");
		m.set_title("ssao");
		m.set_category("fx");

		m.add_field<&comp_ssao::_radius_world, comp_ssao>("radius_world", reflected_field_type::rf_float, "");
		m.add_field<&comp_ssao::_bias, comp_ssao>("bias", reflected_field_type::rf_float, "");
		m.add_field<&comp_ssao::_intensity, comp_ssao>("intensity", reflected_field_type::rf_float, "");
		m.add_field<&comp_ssao::_power, comp_ssao>("power", reflected_field_type::rf_float, "");
		m.add_field<&comp_ssao::_num_dirs, comp_ssao>("num_dirs", reflected_field_type::rf_uint, "", 1.0f, 16.0f);
		m.add_field<&comp_ssao::_num_steps, comp_ssao>("num_steps", reflected_field_type::rf_uint, "", 1.0f, 16.0f);
		m.add_field<&comp_ssao::_random_rot_strength, comp_ssao>("random_rot_strength", reflected_field_type::rf_float, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_ssao* c = static_cast<comp_ssao*>(params.object_ptr);
			c->set_values(params.w, c->_radius_world, c->_bias, c->_intensity, c->_power, c->_num_dirs, c->_num_steps, c->_random_rot_strength);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_ssao* c = static_cast<comp_ssao*>(obj);
			c->set_values(w, c->_radius_world, c->_bias, c->_intensity, c->_power, c->_num_dirs, c->_num_steps, c->_random_rot_strength);
		});
	}

	void comp_ssao::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_ssao ev = {
			.radius_world		 = _radius_world,
			.bias				 = _bias,
			.intensity			 = _intensity,
			.power				 = _power,
			.num_dirs			 = _num_dirs,
			.num_steps			 = _num_steps,
			.random_rot_strength = _random_rot_strength,
			.entity_index		 = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_ssao,
			},
			ev);
	}

	void comp_ssao::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_ssao,
		});
	}

	void comp_ssao::set_values(world& w, f32 radius_world, f32 bias, f32 intensity, f32 power, u32 num_dirs, u32 num_steps, f32 random_rot_strength)
	{
		_radius_world		 = radius_world;
		_bias				 = bias;
		_intensity			 = intensity;
		_power				 = power;
		_num_dirs			 = num_dirs;
		_num_steps			 = num_steps;
		_random_rot_strength = random_rot_strength;

		const render_event_ssao ev = {
			.radius_world		 = _radius_world,
			.bias				 = _bias,
			.intensity			 = _intensity,
			.power				 = _power,
			.num_dirs			 = _num_dirs,
			.num_steps			 = _num_steps,
			.random_rot_strength = _random_rot_strength,
			.entity_index		 = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_ssao,
			},
			ev);
	}

}
