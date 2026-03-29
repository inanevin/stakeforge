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

#include "comp_bloom.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

namespace SFG
{
	void comp_bloom::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_bloom>::value, 0, "component");
		m.set_title("bloom");
		m.set_category("fx");

		m.add_field<&comp_bloom::_filter_radius, comp_bloom>("filter_radius", reflected_field_type::rf_float, "", 0.001f, 0.2f);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_bloom* c = static_cast<comp_bloom*>(params.object_ptr);
			c->set_values(params.w, c->_filter_radius);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_bloom* c = static_cast<comp_bloom*>(obj);
			c->set_values(w, c->_filter_radius);
		});
	}

	void comp_bloom::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_bloom ev = {
			.filter_radius = _filter_radius,
			.entity_index  = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_bloom,
			},
			ev);
	}

	void comp_bloom::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_bloom,
		});
	}

	void comp_bloom::set_values(world& w, f32 filter_radius)
	{
		_filter_radius = filter_radius;

		const render_event_bloom ev = {
			.filter_radius = _filter_radius,
			.entity_index  = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_bloom,
			},
			ev);
	}

}
