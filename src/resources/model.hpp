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

#include "data/bitmask.hpp"
#include "math/aabb.hpp"
#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"
#include "reflection/type_reflection.hpp"

namespace SFG
{
	struct model_raw;
	class world;

	class model
	{
	public:
		static void reflect();
		enum flags
		{
			created = 1 << 0,
		};

		~model();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(model_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bitmask<u8>& get_flags()
		{
			return _flags;
		}

		inline const aabb& get_total_aabb() const
		{
			return _total_aabb;
		}

		inline u16 get_material_count() const
		{
			return _materials_count;
		}

		inline u16 get_texture_count() const
		{
			return _textures_count;
		}

		inline u16 get_node_count() const
		{
			return _nodes_count;
		}

		inline u16 get_mesh_count() const
		{
			return _meshes_count;
		}

		inline u16 get_light_count() const
		{
			return _lights_count;
		}

		inline u16 get_skin_count() const
		{
			return _skins_count;
		}

		inline chunk_handle32 get_created_meshes() const
		{
			return _created_meshes;
		}

		inline chunk_handle32 get_created_nodes() const
		{
			return _nodes;
		}

		inline chunk_handle32 get_created_materials() const
		{
			return _created_materials;
		}

		inline chunk_handle32 get_created_skins() const
		{
			return _created_skins;
		}

		inline chunk_handle32 get_created_anims() const
		{
			return _created_anims;
		}

		inline chunk_handle32 get_created_textures() const
		{
			return _created_textures;
		}

		inline chunk_handle32 get_created_lights() const
		{
			return _created_lights;
		}

		inline chunk_handle32 get_name() const
		{
			return _name;
		}

	private:
#ifdef SFG_TOOLMODE
		friend struct model_raw;
#endif
		chunk_handle32 _name;
		chunk_handle32 _nodes;
		chunk_handle32 _created_meshes;
		chunk_handle32 _created_skins;
		chunk_handle32 _created_anims;
		chunk_handle32 _created_materials;
		chunk_handle32 _created_textures;
		chunk_handle32 _created_lights;
		aabb		   _total_aabb;
		u16		   _nodes_count		= 0;
		u16		   _meshes_count	= 0;
		u16		   _skins_count		= 0;
		u16		   _anims_count		= 0;
		u16		   _materials_count = 0;
		u16		   _textures_count	= 0;
		u16		   _lights_count	= 0;
		bitmask<u8> _flags;
	};

	REFLECT_TYPE(model);

}
