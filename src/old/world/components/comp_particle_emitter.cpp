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

#include "comp_particle_emitter.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "resources/particle_properties.hpp"
#include "resources/material.hpp"

namespace SFG
{
	void comp_particle_emitter::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_particle_emitter>::value, 0, "component");
		m.set_title("particle_emitter");
		m.set_category("rendering");

		m.add_field<&comp_particle_emitter::_particle_resource, comp_particle_emitter>("particle", reflected_field_type::rf_resource, "", type_id<particle_properties>::value);
		m.add_field<&comp_particle_emitter::_material, comp_particle_emitter>("material", reflected_field_type::rf_resource, "", type_id<material>::value);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_particle_emitter* c = static_cast<comp_particle_emitter*>(params.object_ptr);
			c->set_values(params.w, c->_particle_resource, c->_material);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_particle_emitter* c = static_cast<comp_particle_emitter*>(obj);
			c->set_values(w, c->_particle_resource, c->_material);
		});

		m.add_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, [](void* obj, vector<resource_handle_and_type>& h) {
			comp_particle_emitter* c = static_cast<comp_particle_emitter*>(obj);
			h.push_back({.handle = c->_particle_resource, .type_id = type_id<particle_properties>::value});
			h.push_back({.handle = c->_material, .type_id = type_id<material>::value});
		});
	}

	void comp_particle_emitter::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_particle_emitter ev = {
			.entity		  = _header.entity.index,
			.particle_res = NULL_RESOURCE_ID,
			.material	  = NULL_RESOURCE_ID,
		};
		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::particle_emitter}, ev);
	}

	void comp_particle_emitter::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::remove_particle_emitter});
	}

	void comp_particle_emitter::set_values(world& w, resource_handle particle_res, resource_handle material)
	{
		const render_event_particle_emitter ev = {
			.entity		  = _header.entity.index,
			.particle_res = particle_res.is_null() ? NULL_RESOURCE_ID : particle_res.index,
			.material	  = material.is_null() ? NULL_RESOURCE_ID : material.index,
		};
		_material		   = material;
		_particle_resource = particle_res;
		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::particle_emitter}, ev);
	}

	void comp_particle_emitter::restart(world& w)
	{
		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::reset_particle_emitter});
	}

}
