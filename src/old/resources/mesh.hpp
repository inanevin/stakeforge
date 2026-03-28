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
#include "resources/common_resources.hpp"
#include "reflection/type_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct mesh_raw;
	class world;
	class resource_manager;
}

namespace JPH
{
	class Shape;
}

namespace SFG
{

	class mesh
	{
	public:
		static void reflect();
		enum flags
		{
			created = 1 << 0,
		};

		~mesh();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const mesh_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline u16 get_node_index() const
		{
			return _node_index;
		}

		inline u16 get_skin_index() const
		{
			return _skin_index;
		}

		inline string_id get_sid() const
		{
			return _sid;
		}

		inline chunk_handle32 get_material_indices() const
		{
			return _material_indices;
		}

		inline u16 get_material_count() const
		{
			return _material_count;
		}

		inline chunk_handle32 get_collider_vertices() const
		{
			return _collider_vertices;
		}

		inline chunk_handle32 get_collider_indices() const
		{
			return _collider_indices;
		}

		inline u32 get_collider_vertex_count() const
		{
			return _collider_vertex_count;
		}

		inline u32 get_collider_index_count() const
		{
			return _collider_index_count;
		}

		inline chunk_handle32 get_mesh_shape_handle() const
		{
			return _mesh_shape;
		}

		JPH::Shape* get_mesh_shape(resource_manager& rm) const;

	private:
		friend class model;

	private:
		string_id _sid = 0;

#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _material_indices	  = {};
		u16			   _material_count		  = 0;
		u16			   _node_index			  = 0;
		i16			   _skin_index			  = 0;
		chunk_handle32 _collider_vertices	  = {};
		chunk_handle32 _collider_indices	  = {};
		chunk_handle32 _mesh_shape			  = {};
		u32			   _collider_vertex_count = 0;
		u32			   _collider_index_count  = 0;
		bitmask<u8>	   _flags				  = 0;
	};

	REFLECT_TYPE(mesh);

}
