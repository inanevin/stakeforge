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

#include "skin.hpp"
#include "skin_raw.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"

#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"

namespace SFG
{
	void skin::reflect()
	{
		reflection::get().register_meta(type_id<skin>::value, 0, "");
	}

	void skin::create_from_loader(const skin_raw& raw, world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_root		  = raw.root_joint;
		_joints_count = static_cast<u16>(raw.joints.size());
		_joints		  = alloc.allocate<skin_joint>(raw.joints.size());

		skin_joint* ptr	  = reinterpret_cast<skin_joint*>(alloc.get(_joints.head));
		const u32	count = static_cast<u32>(raw.joints.size());

		render_event_skin ev = {};
		ev.root_index		 = raw.root_joint;

		for (u32 i = 0; i < count; i++)
		{
			const skin_joint& joint		   = raw.joints[i];
			const matrix4x3	  inverse_bind = joint.inverse_bind_matrix;
			ptr[i]						   = joint;

			ev.nodes.push_back(joint.model_node_index);
			ev.matrices.push_back(joint.inverse_bind_matrix);
		}

		w.get_render_stream().add_event(
			{
				.index		= handle.index,
				.event_type = render_event_type::create_skin,
			},
			ev);
	}

	void skin::destroy(world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		alloc.free(_joints);

		_joints = {};

		w.get_render_stream().add_event({
			.index		= handle.index,
			.event_type = render_event_type::destroy_skin,
		});
	}

}
