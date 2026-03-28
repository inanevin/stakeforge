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

#include "comp_skybox.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

namespace SFG
{
	void comp_skybox::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_skybox>::value, 0, "component");
		m.set_title("skybox");
		m.set_category("fx");

		m.add_field<&comp_skybox::_start_color, comp_skybox>("start_color", reflected_field_type::rf_vector4, "");
		m.add_field<&comp_skybox::_mid_color, comp_skybox>("mid_color", reflected_field_type::rf_vector4, "");
		m.add_field<&comp_skybox::_end_color, comp_skybox>("end_color", reflected_field_type::rf_vector4, "");
		m.add_field<&comp_skybox::_fog_color, comp_skybox>("fog_color", reflected_field_type::rf_vector4, "");
		m.add_field<&comp_skybox::_fog_start, comp_skybox>("fog_start", reflected_field_type::rf_float, "");
		m.add_field<&comp_skybox::_fog_end, comp_skybox>("fog_end", reflected_field_type::rf_float, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_skybox* c = static_cast<comp_skybox*>(params.object_ptr);
			c->set_values(params.w, c->_start_color, c->_mid_color, c->_end_color, c->_fog_color, c->_fog_start, c->_fog_end);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_skybox* c = static_cast<comp_skybox*>(obj);
			c->set_values(w, c->_start_color, c->_mid_color, c->_end_color, c->_fog_color, c->_fog_start, c->_fog_end);
		});
	}

	void comp_skybox::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_skybox ev = {
			.start_color  = _start_color,
			.mid_color	  = _mid_color,
			.end_color	  = _end_color,
			.fog_color	  = _fog_color,
			.fog_start	  = _fog_start,
			.fog_end	  = _fog_end,
			.entity_index = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_skybox,
			},
			ev);
	}

	void comp_skybox::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_skybox,
		});
	}

	void comp_skybox::set_values(world& w, const vector4& start_color, const vector4& mid_color, const vector4& end_color, const vector4& fog_color, f32 fog_start, f32 fog_end)
	{
		_start_color = start_color;
		_mid_color	 = mid_color;
		_end_color	 = end_color;
		_fog_color	 = fog_color;
		_fog_start	 = fog_start;
		_fog_end	 = fog_end;

		const render_event_skybox ev = {
			.start_color  = _start_color,
			.mid_color	  = _mid_color,
			.end_color	  = _end_color,
			.fog_color	  = _fog_color,
			.fog_start	  = _fog_start,
			.fog_end	  = _fog_end,
			.entity_index = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_skybox,
			},
			ev);
	}
}
