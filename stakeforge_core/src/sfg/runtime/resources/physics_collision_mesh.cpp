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

#include "physics_collision_mesh.hpp"

#include "physics_collision_mesh_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace sfg
{
	namespace
	{
		bool load_collision_mesh_def(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream)
		{
			physics_collision_mesh_def_t def = {};

			if (!reflection_registry_t::get().type_from_stream(type_id_t<physics_collision_mesh_def_t>::value, &def, nullptr, stream))
			{
				SFG_ERR("failed to deserialize physics collision mesh definition: {0}", entry.hash);
				return false;
			}

			chunk_allocator_t&				  memory  = ctx.resource_manager.get_memory();
			physics_collision_mesh_runtime_t* runtime = memory.get<physics_collision_mesh_runtime_t>(entry.runtime);
			*runtime								  = {};
			runtime->vertices						  = memory.allocate<vec3f_t>(def.vertices.size());
			runtime->indices						  = memory.allocate<primitive_index>(def.indices.size());
			runtime->vertex_count					  = static_cast<u32>(def.vertices.size());
			runtime->index_count					  = static_cast<u32>(def.indices.size());
			SFG_MEMCPY(memory.get<vec3f_t>(runtime->vertices), def.vertices.data(), def.vertices.size() * sizeof(vec3f_t));
			SFG_MEMCPY(memory.get<primitive_index>(runtime->indices), def.indices.data(), def.indices.size() * sizeof(primitive_index));

			JPH::VertexList vertices;
			vertices.reserve(runtime->vertex_count);
			for (const vec3f_t& vertex : def.vertices)
				vertices.emplace_back(vertex.x, vertex.y, vertex.z);

			JPH::IndexedTriangleList triangles;
			triangles.reserve(runtime->index_count / 3);
			for (u32 i = 0; i < runtime->index_count; i += 3)
				triangles.emplace_back(def.indices[i], def.indices[i + 1], def.indices[i + 2]);

			JPH::MeshShapeSettings			settings(vertices, triangles);
			JPH::ShapeSettings::ShapeResult result = settings.Create();
			if (result.HasError())
			{
				SFG_ERR("failed to create physics collision mesh shape: {0}", result.GetError().c_str());
				memory.free(runtime->vertices);
				memory.free(runtime->indices);
				*runtime = {};
				return false;
			}

			JPH::Shape* shape = result.Get().GetPtr();
			shape->AddRef();
			runtime->mesh_shape							  = memory.allocate<JPH::Shape*>(1);
			*memory.get<JPH::Shape*>(runtime->mesh_shape) = shape;
			return true;
		}
	}

	bool physics_collision_mesh_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
		{
			SFG_ERR("failed to read physics collision mesh resource: {0}", entry.hash);
			return false;
		}

		istream_t stream(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);
		if (payload.empty())
		{
			SFG_ERR("failed to decompress physics collision mesh payload: {0}", entry.hash);
			return false;
		}

		return load_collision_mesh_def(entry, ctx, payload);
	}

	bool physics_collision_mesh_loader_t::runtime_load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream)
	{
		return load_collision_mesh_def(entry, ctx, stream);
	}

	void physics_collision_mesh_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&				  memory  = ctx.resource_manager.get_memory();
		physics_collision_mesh_runtime_t* runtime = memory.get<physics_collision_mesh_runtime_t>(entry.runtime);

		(*memory.get<JPH::Shape*>(runtime->mesh_shape))->Release();
		memory.free(runtime->mesh_shape);
		memory.free(runtime->vertices);
		memory.free(runtime->indices);
		*runtime = {};
	}

	const resource_type_desc_t physics_collision_mesh_resource_desc = {
		.type				 = resource_type_e::physics_collision_mesh,
		.runtime_size		 = sizeof(physics_collision_mesh_runtime_t),
		.runtime_alignment	 = alignof(physics_collision_mesh_runtime_t),
		.internals_size		 = sizeof(u32),
		.internals_alignment = sizeof(u32),
		.wire_magic			 = physics_collision_mesh_loader_t::WIRE_MAGIC,
		.wire_version		 = physics_collision_mesh_loader_t::WIRE_VERSION,
		.load				 = physics_collision_mesh_loader_t::load,
		.runtime_load		 = physics_collision_mesh_loader_t::runtime_load,
		.unload				 = physics_collision_mesh_loader_t::unload,
	};
}
