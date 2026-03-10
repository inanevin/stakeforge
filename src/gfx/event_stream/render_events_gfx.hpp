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

#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/descriptions.hpp"
#include "data/static_vector.hpp"
#include "math/vector2ui16.hpp"
#include "math/matrix4x3.hpp"
#include "math/aabb.hpp"
#include "resources/primitive_raw.hpp"
#include "resources/common_resources.hpp"
#include "data/ostream.hpp"
#include "data/span.hpp"
#include "game/game_max_defines.hpp"
#include "world/particles/common_particles.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "data/string.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;

	struct render_event_vertex_index_buffer
	{

#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif
		u16 cpu_flags = 0;
		u16 gpu_flags = 0;
		u32 max_size  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_update_vertex_index_buffer
	{
		u8* data	   = nullptr;
		u32 size	   = 0;
		u32 item_count = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_texture
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		vector2ui16										size			   = {};
		u32												intermediate_size  = 0;
		u8												format			   = 0;
		bool											buffers_persistent = false;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_sampler
	{
		sampler_desc desc = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_shader
	{
		vector<compile_variant> compile_variants;
		vector<pso_variant>		pso_variants;
		gfx_id					layout			 = 0;
		bool					persistent_blobs = false;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_material
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif
		static_vector<resource_id, MAX_MATERIAL_TEXTURES> textures;
		resource_id										  sampler	   = NULL_RESOURCE_ID;
		resource_id										  shader_index = NULL_RESOURCE_ID;
		span<u8>										  data		   = {};
		u32												  flags;
		u16												  priority = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_update_material_sampler
	{
		resource_id sampler = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_update_material_textures
	{
		u8												  start = 0;
		static_vector<resource_id, MAX_MATERIAL_TEXTURES> textures;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_update_material_data
	{
		u32			padding = 0;
		const void* data	= nullptr;
		u32			size	= 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_mesh
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif

		aabb						  local_aabb;
		vector<primitive_static_raw>  primitives_static;
		vector<primitive_skinned_raw> primitives_skinned;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_material_reloaded
	{
	};

	struct render_event_resource_reloaded
	{
		resource_id prev_id;
		resource_id new_id;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_skin
	{
		vector<u16>		  nodes;
		vector<matrix4x3> matrices;
		i16				  root_index = -1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_particle_res
	{
		particle_emit_properties props = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
