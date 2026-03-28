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

#include "render_events_gfx.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"

namespace SFG
{
	void render_event_texture::serialize(ostream& stream) const
	{
		stream << size;
		stream << intermediate_size;
		stream << format;
		stream << buffers_persistent;
		stream << static_cast<u32>(buffers.size());

		for (const texture_buffer& b : buffers)
		{
			b.serialize(stream, true);
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif
	}

	void render_event_texture::deserialize(istream& stream)
	{
		u32 sz = 0;
		stream >> size;
		stream >> intermediate_size;
		stream >> format;
		stream >> buffers_persistent;
		stream >> sz;
		buffers.resize(sz);
		for (u32 i = 0; i < sz; i++)
		{
			texture_buffer& b = buffers[i];
			b.deserialize(stream, true);
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif
	}

	void render_event_sampler::serialize(ostream& stream) const
	{
		desc.serialize(stream);
	}

	void render_event_sampler::deserialize(istream& stream)
	{
		desc.deserialize(stream);
	}

	void render_event_shader::serialize(ostream& stream) const
	{
		stream << persistent_blobs;
		stream << layout;
		stream << pso_variants;

		const u32 compile_variants_sz = static_cast<u32>(compile_variants.size());
		stream << compile_variants_sz;

		for (const compile_variant& v : compile_variants)
		{
			v.serialize(stream, true);
		}
	}

	void render_event_shader::deserialize(istream& stream)
	{
		stream >> persistent_blobs;
		stream >> layout;
		stream >> pso_variants;

		u32 compile_variants_size = 0;
		stream >> compile_variants_size;

		compile_variants.resize(compile_variants_size);
		for (u32 i = 0; i < compile_variants_size; i++)
		{
			compile_variant& v = compile_variants[i];
			v.deserialize(stream, true);
		}
	}

	void render_event_material::serialize(ostream& stream) const
	{
		const u32 txt_sz = static_cast<u32>(textures.size());
		stream << txt_sz;
		stream << flags;
		stream << shader_index;
		stream << priority;

		for (resource_id h : textures)
		{
			stream << h;
		}

		stream << sampler;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif

		stream << static_cast<u32>(data.size);
		stream.write_raw(data.data, data.size);
	}

	void render_event_material::deserialize(istream& stream)
	{
		u32 txt_sz = 0;
		stream >> txt_sz;
		stream >> flags;
		stream >> shader_index;
		stream >> priority;
		textures.resize(txt_sz);

		for (u32 i = 0; i < txt_sz; i++)
		{
			stream >> textures[i];
		}

		stream >> sampler;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif

		u64 addr = 0;
		u32 sz	 = 0;
		stream >> sz;
		data.data = stream.get_data_current();
		data.size = sz;
		stream.skip_by(sz);
	}

	void render_event_mesh::serialize(ostream& stream) const
	{
		stream << primitives_static;
		stream << primitives_skinned;
		stream << local_aabb;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif
	}

	void render_event_mesh::deserialize(istream& stream)
	{
		stream >> primitives_static;
		stream >> primitives_skinned;
		stream >> local_aabb;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif
	}

	void render_event_resource_reloaded::serialize(ostream& stream) const
	{
		stream << prev_id;
		stream << new_id;
	}

	void render_event_resource_reloaded::deserialize(istream& stream)
	{
		stream >> prev_id;
		stream >> new_id;
	}

	void render_event_vertex_index_buffer::serialize(ostream& stream) const
	{
		stream << max_size;
		stream << cpu_flags;
		stream << gpu_flags;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif
	}

	void render_event_vertex_index_buffer::deserialize(istream& stream)
	{
		stream >> max_size;
		stream >> cpu_flags;
		stream >> gpu_flags;
#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif
	}

	void render_event_update_vertex_index_buffer::serialize(ostream& stream) const
	{
		SFG_ASSERT(size != 0);
		stream << item_count;
		stream << size;
		stream.write_raw(data, static_cast<size_t>(size));
	}

	void render_event_update_vertex_index_buffer::deserialize(istream& stream)
	{
		stream >> size;
		stream >> item_count;
		data = stream.get_data_current();
		stream.skip_by(size);
	}

	void render_event_skin::serialize(ostream& stream) const
	{
		stream << root_index;
		stream << nodes;
		stream << matrices;
	}

	void render_event_skin::deserialize(istream& stream)
	{
		stream >> root_index;
		stream >> nodes;
		stream >> matrices;
	}

	void render_event_update_material_sampler::serialize(ostream& stream) const
	{
		stream << sampler;
	}

	void render_event_update_material_sampler::deserialize(istream& stream)
	{
		stream >> sampler;
	}

	void render_event_update_material_data::serialize(ostream& stream) const
	{
		stream << padding;
		stream << size;
		stream.write_raw((u8*)data, static_cast<size_t>(size));
	}

	void render_event_update_material_data::deserialize(istream& stream)
	{
		stream >> padding;
		stream >> size;
		data = stream.get_data_current();
		stream.skip_by(size);
	}

	void render_event_update_material_textures::serialize(ostream& stream) const
	{
		stream << start;
		stream << static_cast<u32>(textures.size());
		for (resource_id id : textures)
			stream << id;
	}

	void render_event_update_material_textures::deserialize(istream& stream)
	{
		stream >> start;
		u32 sz = 0;
		stream >> sz;
		textures.resize(sz);
		for (u32 i = 0; i < sz; i++)
			stream >> textures[i];
	}

	void render_event_particle_res::serialize(ostream& stream) const
	{
		stream << props;
	}

	void render_event_particle_res::deserialize(istream& stream)
	{
		stream >> props;
	}

}
