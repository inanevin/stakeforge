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

#include "material.hpp"
#include "material_raw.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "io/assert.hpp"
#include "world/world.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void material::reflect()
	{
		reflection::get().register_meta(type_id<material>::value, 0, "stkmat");
	}
	material::~material()
	{
		SFG_ASSERT(!_flags.is_set(material_flags::material_flags_created));
	}

	void material::create_manual(world& w, const material_creation_params& p)
	{
		render_event_stream& stream = w.get_render_stream();

		SFG_ASSERT(!p.handle.is_null() && !p.shader.is_null());

		render_event_material ev = {};
		ev.sampler				 = p.sampler.index;
		ev.shader_index			 = p.shader.index;

#ifndef SFG_STRIP_DEBUG_NAMES
		ev.name = p.name;
#endif
		_flags = 0;
		_flags.set(material_flags::material_flags_created);
		_flags.set(material_flags::material_flags_is_gbuffer, p.pass_mode == material_pass_mode::gbuffer);
		_flags.set(material_flags::material_flags_is_forward, p.pass_mode == material_pass_mode::forward);
		_flags.set(material_flags::material_flags_is_gui, p.pass_mode == material_pass_mode::gui);
		_flags.set(material_flags::material_flags_is_particle, p.pass_mode == material_pass_mode::particle);
		_flags.set(material_flags::material_flags_is_sprite, p.pass_mode == material_pass_mode::sprite);

		for (u8 i = 0; i < p.textures_count; i++)
		{
			if (p.textures[i].index == NULL_RESOURCE_ID)
				ev.textures.push_back(DUMMY_COLOR_TEXTURE_SID);
			else
				ev.textures.push_back(p.textures[i].index);
		}

		if (p.data && p.data_size != 0)
		{
			ev.data.data = reinterpret_cast<u8*>(SFG_MALLOC(p.data_size));
			ev.data.size = p.data_size;
			if (ev.data.data != nullptr)
				SFG_MEMCPY(ev.data.data, p.data, p.data_size);
		}

		ev.flags = _flags.value();

		stream.add_event(
			{
				.index		= static_cast<u32>(p.handle.index),
				.event_type = render_event_type::create_material,
			},
			ev);
	}

	void material::create_from_loader(material_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(material_flags::material_flags_created));
		_flags.set(material_flags::material_flags_created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		const u8 texture_count = static_cast<u8>(raw.textures.size());
		_flags.set(material_flags::material_flags_is_gbuffer, raw.pass_mode == material_pass_mode::gbuffer);
		_flags.set(material_flags::material_flags_is_forward, raw.pass_mode == material_pass_mode::forward);
		_flags.set(material_flags::material_flags_is_alpha_cutoff, raw.use_alpha_cutoff);
		_flags.set(material_flags::material_flags_is_double_sided, raw.double_sided);
		_flags.set(material_flags::material_flags_is_gui, raw.pass_mode == material_pass_mode::gui);
		_flags.set(material_flags::material_flags_is_sprite, raw.pass_mode == material_pass_mode::sprite);
		SFG_ASSERT(raw.shader != 0);

		render_event_material ev = {};
		ev.data.data			 = raw.material_data.get_raw();
		ev.data.size			 = raw.material_data.get_size();
		ev.flags				 = _flags.value();
		ev.priority				 = raw.draw_priority;

		if (!raw.textures.empty())
		{
			const resource_handle sampler_handle = rm.get_or_add_sampler(raw.sampler_definition);
			ev.sampler							 = sampler_handle.index;
		}

		for (string_id sid : raw.textures)
		{
			const resource_handle handle = rm.get_resource_handle_by_hash_if_exists<texture>(sid);

			if (handle.is_null())
			{
				const resource_handle dummy = rm.get_resource_handle_by_hash_if_exists<texture>(DUMMY_COLOR_TEXTURE_SID);
				ev.textures.push_back(dummy.index);
			}
			else
				ev.textures.push_back(handle.index);
		}
		const resource_handle shader_handle = rm.get_resource_handle_by_hash_if_exists<shader>(raw.shader);
		ev.shader_index						= shader_handle.is_null() ? NULL_RESOURCE_ID : shader_handle.index;

#ifndef SFG_STRIP_DEBUG_NAMES
		ev.name = raw.name;
#endif
		stream.add_event(
			{
				.index		= static_cast<u32>(handle.index),
				.event_type = render_event_type::create_material,
			},
			ev);

		raw.material_data.destroy();
	}

	void material::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(material_flags::material_flags_created))
			return;
		_flags.remove(material_flags::material_flags_created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.index		= static_cast<u32>(handle.index),
			.event_type = render_event_type::destroy_material,
		});
	}

	void material::update_data(world& w, resource_handle handle, size_t padding, const void* data, size_t size)
	{
		const render_event_update_material_data ev = {
			.padding = static_cast<u32>(padding),
			.data	 = data,
			.size	 = static_cast<u32>(size),
		};

		w.get_render_stream().add_event(
			{
				.index		= static_cast<u32>(handle.index),
				.event_type = render_event_type::update_material_data,
			},
			ev);
	}
	void material::update_sampler(world& w, resource_handle material_own_handle, resource_handle sampler)
	{
		const render_event_update_material_sampler ev = {
			.sampler = sampler.index,
		};
		w.get_render_stream().add_event(
			{
				.index		= static_cast<u32>(material_own_handle.index),
				.event_type = render_event_type::update_material_sampler,
			},
			ev);
	}
	void material::update_textures(world& w, resource_handle material_own_handle, const resource_handle* textures, u8 start, u8 count)
	{
		render_event_update_material_textures ev = {};
		ev.textures.resize(count);
		ev.start = start;
		for (u8 i = 0; i < count; i++)
			ev.textures[i] = textures[i].index;

		w.get_render_stream().add_event(
			{
				.index		= static_cast<u32>(material_own_handle.index),
				.event_type = render_event_type::update_material_textures,
			},
			ev);
	}
}
