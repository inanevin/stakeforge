// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
	namespace
	{
		void clear_mesh_runtime_cpu_data(mesh_runtime_t& runtime)
		{
			runtime.vertex_data		 = {};
			runtime.index_data		 = {};
			runtime.vertex_data_size = 0;
			runtime.index_data_size	 = 0;
			runtime.vertex_count	 = 0;
			runtime.index_count		 = 0;
			runtime.vertex_stride	 = 0;
		}

		void free_mesh_cpu_data(chunk_allocator_t& mem, mesh_runtime_t& runtime)
		{
			mem.free(runtime.vertex_data);
			mem.free(runtime.index_data);

			clear_mesh_runtime_cpu_data(runtime);
		}

		template <typename def_primitive_t, typename vertex_t> void build_mesh_data(const vector_t<def_primitive_t>& def_primitives, const vector_t<resource_handle_t>& materials, chunk_allocator_t& mem, mesh_runtime_t& runtime, mesh_internals_t& internals)
		{
			size_t vertex_count = 0;
			size_t index_count	= 0;
			for (const def_primitive_t& primitive : def_primitives)
			{
				vertex_count += primitive.vertices.size();
				index_count += primitive.indices.size();
			}

			const size_t vertex_data_size = vertex_count * sizeof(vertex_t);
			const size_t index_data_size  = index_count * sizeof(primitive_index);
			const u32	 primitive_count  = static_cast<u32>(def_primitives.size());
			runtime.vertex_data			  = mem.allocate_bytes(vertex_data_size, alignof(vertex_t));
			runtime.index_data			  = mem.allocate_bytes(index_data_size, alignof(primitive_index));
			runtime.primitives			  = mem.allocate_bytes(sizeof(mesh_primitive_runtime_t) * primitive_count, alignof(mesh_primitive_runtime_t));

			u8*						  vertex_dst	= mem.get<u8>(runtime.vertex_data);
			u8*						  index_dst		= mem.get<u8>(runtime.index_data);
			mesh_primitive_runtime_t* primitive_dst = mem.get<mesh_primitive_runtime_t>(runtime.primitives);
			u32						  start_vertex	= 0;
			u32						  start_index	= 0;
			for (u32 i = 0; i < primitive_count; ++i)
			{
				const def_primitive_t& primitive = def_primitives[i];
				SFG_ASSERT(primitive.material_index < materials.size());

				const size_t vertex_bytes = primitive.vertices.size() * sizeof(vertex_t);
				const size_t index_bytes  = primitive.indices.size() * sizeof(primitive_index);
				primitive_dst[i]		  = {
							 .material		 = materials[primitive.material_index],
							 .material_index = primitive.material_index,
							 .start_index	 = start_index,
							 .start_vertex	 = start_vertex,
							 .index_count	 = static_cast<u32>(primitive.indices.size()),
				 };
				SFG_MEMCPY(vertex_dst, primitive.vertices.data(), vertex_bytes);
				SFG_MEMCPY(index_dst, primitive.indices.data(), index_bytes);

				vertex_dst += vertex_bytes;
				index_dst += index_bytes;
				start_vertex += static_cast<u32>(primitive.vertices.size());
				start_index += static_cast<u32>(primitive.indices.size());
			}

			runtime.vertex_data_size = static_cast<u32>(vertex_data_size);
			runtime.index_data_size	 = static_cast<u32>(index_data_size);
			runtime.primitive_count	 = primitive_count;
			runtime.vertex_count	 = static_cast<u32>(vertex_count);
			runtime.index_count		 = static_cast<u32>(index_count);
			runtime.vertex_stride	 = sizeof(vertex_t);
			runtime.index_stride	 = sizeof(primitive_index);

			internals.primitive_count = primitive_count;
			internals.vertex_count	  = runtime.vertex_count;
			internals.index_count	  = runtime.index_count;
			internals.vertex_stride	  = runtime.vertex_stride;
		}
	}

	bool mesh_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
		{
			SFG_ERR("failed to read mesh resource: {0}", entry.hash);
			return false;
		}

		istream_t stream;
		stream.open(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);
		if (payload.empty())
		{
			SFG_ERR("failed to decompress mesh payload: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		mesh_runtime_t*	   runtime	 = mem.get<mesh_runtime_t>(entry.runtime);
		mesh_internals_t*  internals = mem.get<mesh_internals_t>(entry.internals);
		*runtime					 = {};
		*internals					 = {};

		mesh_def_t mesh = {};
		if (!reflection_registry_t::get().type_from_stream(type_id_t<mesh_def_t>::value, &mesh, nullptr, payload))
		{
			SFG_ERR("failed to deserialize mesh definition: {0}", entry.hash);
			return false;
		}

		internals->local_bounds = mesh.local_bounds;
		internals->local_bounds.update_half_extents();

		const bool has_static  = !mesh.static_primitives.empty();
		const bool has_skinned = !mesh.skinned_primitives.empty();
		SFG_ASSERT(has_static || has_skinned);
		SFG_ASSERT(!has_static || !has_skinned);

		if (has_static)
		{
			build_mesh_data<primitive_static_def_t, vertex_static_t>(mesh.static_primitives, mesh.materials, mem, *runtime, *internals);
		}
		else if (has_skinned)
		{
			build_mesh_data<primitive_skinned_def_t, vertex_skinned_t>(mesh.skinned_primitives, mesh.materials, mem, *runtime, *internals);
			runtime->is_skinned	  = 1;
			internals->is_skinned = 1;
		}

		resource_desc_t vertex_desc = {};
		vertex_desc.size			= runtime->vertex_data_size;
		vertex_desc.structure_size	= runtime->vertex_stride;
		vertex_desc.structure_count = runtime->vertex_count;
		vertex_desc.flags			= resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
		vertex_desc.set_name(mem.get_text(entry.debug_name));
		internals->vertex_buffer = render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, vertex_desc);
		render_resources_t::get().enqueue_data_upload({.data = mem.get<u8>(runtime->vertex_data), .resource = internals->vertex_buffer, .data_size = runtime->vertex_data_size});

		resource_desc_t index_desc = {};
		index_desc.size			   = runtime->index_data_size;
		index_desc.structure_size  = sizeof(primitive_index);
		index_desc.structure_count = runtime->index_count;
		index_desc.flags		   = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
		index_desc.set_name(mem.get_text(entry.debug_name));
		internals->index_buffer = render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, index_desc);
		render_resources_t::get().enqueue_data_upload({.data = mem.get<u8>(runtime->index_data), .resource = internals->index_buffer, .data_size = runtime->index_data_size});

		free_mesh_cpu_data(mem, *runtime);
		return true;
	}

	void mesh_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		mesh_runtime_t*	   runtime	 = mem.get<mesh_runtime_t>(entry.runtime);
		mesh_internals_t*  internals = mem.get<mesh_internals_t>(entry.internals);

		clear_mesh_runtime_cpu_data(*runtime);
		render_resources_t::get().enqueue_destroy_resource(internals->vertex_buffer);
		render_resources_t::get().enqueue_destroy_resource(internals->index_buffer);
		mem.free(runtime->primitives);
		*runtime   = {};
		*internals = {};
	}

	const resource_type_desc_t mesh_resource_desc = {
		.type				 = resource_type_e::mesh,
		.runtime_size		 = sizeof(mesh_runtime_t),
		.runtime_alignment	 = alignof(mesh_runtime_t),
		.internals_size		 = sizeof(mesh_internals_t),
		.internals_alignment = alignof(mesh_internals_t),
		.wire_magic			 = mesh_loader_t::WIRE_MAGIC,
		.wire_version		 = mesh_loader_t::WIRE_VERSION,
		.use_async_load		 = false,
		.load				 = mesh_loader_t::load,
		.unload				 = mesh_loader_t::unload,
	};
}
