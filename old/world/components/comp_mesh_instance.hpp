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

#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"
#include "data/vector.hpp"

namespace SFG
{
	class world;
	class resource_manager;

	class comp_mesh_instance
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& we);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void set_mesh(world& w, resource_handle mesh, resource_handle skin, resource_handle* materials, u16 materials_count, world_handle* skin_node_entities, u16 skin_node_entity_count);
		void set_material(world& w, resource_handle material, u32 index);

		inline resource_handle get_mesh() const
		{
			return _target_mesh;
		}

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline const vector<resource_handle>& get_materials() const
		{
			return _materials;
		}

		inline u16 get_materials_count() const
		{
			return static_cast<u16>(_materials.size());
		}

		inline const vector<world_handle>& get_skin_entities() const
		{
			return _skin_entities;
		}

		inline u16 get_skin_entities_count() const
		{
			return static_cast<u16>(_skin_entities.size());
		}

	private:
		template <typename T, int> friend class comp_cache;

		void fetch_refs(resource_manager& res, string_id& out_target, string_id& out_target_mesh) const;
		void fill_refs(resource_manager& res, string_id target, string_id target_mesh);

	private:
		component_header		_header		   = {};
		resource_handle			_target_mesh   = {};
		resource_handle			_target_skin   = {};
		vector<resource_handle> _materials	   = {};
		vector<world_handle>	_skin_entities = {};
	};

	REFLECT_TYPE(comp_mesh_instance);
}
