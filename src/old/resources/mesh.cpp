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

#include "mesh.hpp"
#include "mesh_raw.hpp"
#include "primitive_raw.hpp"
#include "primitive.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "reflection/reflection.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace SFG
{
	void mesh::reflect()
	{
		reflection::get().register_meta(type_id<mesh>::value, 0, "");
	}
	mesh::~mesh()
	{
		SFG_ASSERT(!_flags.is_set(mesh::flags::created));
	}

	JPH::Shape* mesh::get_mesh_shape(resource_manager& rm) const
	{
		if (_mesh_shape.size == 0)
			return nullptr;

		JPH::Shape** shape = rm.get_aux().get<JPH::Shape*>(_mesh_shape);
		return shape != nullptr ? shape[0] : nullptr;
	}

	void mesh::create_from_loader(const mesh_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(mesh::flags::created));
		_flags.set(mesh::flags::created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_node_index = raw.node_index;
		_skin_index = raw.skin_index;
		_sid		= raw.sid;

		const size_t m_size = raw.materials.size();
		_material_count		= static_cast<u16>(m_size);
		if (m_size != 0)
		{
			_material_indices = alloc.allocate<i16>(raw.materials.size());
			u16* idx		  = alloc.get<u16>(_material_indices);
			for (u32 i = 0; i < m_size; i++)
			{
				const i16 mat = raw.materials[i];
				idx[i]		  = mat >= 0 ? static_cast<u16>(raw.materials[i]) : 0;
			}
		}

		if (!raw.collider_vertices.empty() && !raw.collider_indices.empty())
		{
			_collider_vertex_count = static_cast<u32>(raw.collider_vertices.size());
			_collider_index_count  = static_cast<u32>(raw.collider_indices.size());

			_collider_vertices = alloc.allocate<vector3>(_collider_vertex_count);
			vector3* vtx	   = alloc.get<vector3>(_collider_vertices);
			for (u32 i = 0; i < _collider_vertex_count; i++)
				vtx[i] = raw.collider_vertices[i];

			_collider_indices	 = alloc.allocate<primitive_index>(_collider_index_count);
			primitive_index* idx = alloc.get<primitive_index>(_collider_indices);
			for (u32 i = 0; i < _collider_index_count; i++)
				idx[i] = raw.collider_indices[i];

			JPH::VertexList vertices;
			vertices.reserve(_collider_vertex_count);
			for (u32 i = 0; i < _collider_vertex_count; i++)
				vertices.push_back(JPH::Float3(vtx[i].x, vtx[i].y, vtx[i].z));

			JPH::IndexedTriangleList triangles;
			const u32				 tri_count = _collider_index_count / 3;
			triangles.reserve(tri_count);
			for (u32 i = 0; i < tri_count; i++)
			{
				const u32 base = i * 3;
				triangles.push_back(JPH::IndexedTriangle(idx[base], idx[base + 1], idx[base + 2]));
			}

			JPH::MeshShapeSettings			settings(vertices, triangles);
			JPH::ShapeSettings::ShapeResult result = settings.Create();
			if (!result.HasError())
			{
				JPH::Shape* shape = result.Get().GetPtr();
				if (shape != nullptr)
				{
					shape->AddRef();
					_mesh_shape			   = alloc.allocate<JPH::Shape*>(1);
					JPH::Shape** shape_ptr = alloc.get<JPH::Shape*>(_mesh_shape);
					shape_ptr[0]		   = shape;
				}
			}
		}

		render_event_mesh ev  = {};
		ev.primitives_static  = raw.primitives_static;
		ev.primitives_skinned = raw.primitives_skinned;
		ev.local_aabb		  = raw.local_aabb;

#ifndef SFG_STRIP_DEBUG_NAMES
		ev.name = raw.name;
#endif

		stream.add_event(
			{
				.index		= static_cast<u32>(handle.index),
				.event_type = render_event_type::create_mesh,
			},
			ev);
	}

	void mesh::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(mesh::flags::created))
			return;

		_flags.remove(mesh::flags::created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		if (_material_indices.size != 0)
			alloc.free(_material_indices);
		_material_indices = {};

		if (_mesh_shape.size != 0)
		{
			JPH::Shape** shape_ptr = alloc.get<JPH::Shape*>(_mesh_shape);
			if (shape_ptr != nullptr && shape_ptr[0] != nullptr)
				shape_ptr[0]->Release();
			alloc.free(_mesh_shape);
		}
		_mesh_shape = {};

		if (_collider_vertices.size != 0)
			alloc.free(_collider_vertices);
		_collider_vertices = {};

		if (_collider_indices.size != 0)
			alloc.free(_collider_indices);
		_collider_indices = {};

		_collider_vertex_count = 0;
		_collider_index_count  = 0;

		stream.add_event({
			.index		= static_cast<u32>(handle.index),
			.event_type = render_event_type::destroy_mesh,
		});

		_node_index = _skin_index = _sid = 0;
	}
}
