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

#include "particle_properties.hpp"
#include "particle_properties_raw.hpp"
#include "gui/vekt.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void particle_properties::reflect()
	{
		reflection::get().register_meta(type_id<particle_properties>::value, 0, "stkparticle");
	}
	particle_properties::~particle_properties()
	{
		SFG_ASSERT(!_flags.is_set(particle_properties::flags::created));
	}

	void particle_properties::create_from_loader(const particle_properties_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(particle_properties::flags::created));
		_flags.set(particle_properties::flags::created);

		_emit = raw.props;
		update_data(w, handle);
	}

	void particle_properties::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(particle_properties::flags::created))
			return;
		_flags.remove(particle_properties::flags::created);

		w.get_render_stream().add_event({.index = handle.index, .event_type = render_event_type::destroy_particle_res});
	}
	void particle_properties::update_data(world& w, resource_handle handle)
	{
		const render_event_particle_res ev = {
			.props = _emit,
		};
		w.get_render_stream().add_event({.index = handle.index, .event_type = render_event_type::particle_res}, ev);
	}
}
