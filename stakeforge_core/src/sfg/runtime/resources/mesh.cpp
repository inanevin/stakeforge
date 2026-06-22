// Copyright (c) 2025 Inan Evin

#include "mesh.hpp"

#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/render_resources.hpp>

#include <limits>

namespace sfg
{
	bool mesh_loader_t::load(resource_entry_t&, resource_context_t&, ostream_t&)
	{
		return false;
	}

#define MESH_RESOURCE_USER_DATA_VERTEX_BUFFER 1
#define MESH_RESOURCE_USER_DATA_INDEX_BUFFER  2

	namespace
	{
		void free_mesh_runtime_cpu_data(mesh_runtime_t& runtime)
		{
			if (runtime.vertex_data != nullptr)
				SFG_FREE(runtime.vertex_data);
			if (runtime.index_data != nullptr)
				SFG_FREE(runtime.index_data);

			runtime.vertex_data		 = nullptr;
			runtime.index_data		 = nullptr;
			runtime.vertex_data_size = 0;
			runtime.index_data_size	 = 0;
			runtime.vertex_count	 = 0;
			runtime.index_count		 = 0;
			runtime.vertex_stride	 = 0;
		}

		void free_mesh_internals_cpu_data(mesh_internals_t& internals)
		{
			if (internals.static_primitives != nullptr)
				SFG_FREE(internals.static_primitives);
			if (internals.skinned_primitives != nullptr)
				SFG_FREE(internals.skinned_primitives);

			internals.static_primitives	 = nullptr;
			internals.skinned_primitives = nullptr;
			internals.primitive_count	 = 0;
		}

		void free_mesh_cpu_data(mesh_runtime_t& runtime, mesh_internals_t& internals)
		{
			free_mesh_runtime_cpu_data(runtime);
			free_mesh_internals_cpu_data(internals);
		}

		template <typename def_primitive_t, typename runtime_primitive_t, typename vertex_t>
		bool build_mesh_data(const vector_t<def_primitive_t>& def_primitives, const vector_t<resource_handle_t>& materials, runtime_primitive_t*& out_primitives, mesh_runtime_t& runtime, mesh_internals_t& internals)
		{
			size_t vertex_count = 0;
			size_t index_count	= 0;
			for (const def_primitive_t& primitive : def_primitives)
			{
				if (primitive.vertices.empty() || primitive.indices.empty())
					return false;
				if (primitive.material_index != UINT32_MAX && primitive.material_index >= materials.size())
					return false;

				vertex_count += primitive.vertices.size();
				index_count += primitive.indices.size();
			}

			if (def_primitives.size() > std::numeric_limits<u32>::max() || vertex_count == 0 || index_count == 0 || vertex_count > std::numeric_limits<u32>::max() || index_count > std::numeric_limits<u32>::max())
				return false;

			const size_t vertex_data_size = vertex_count * sizeof(vertex_t);
			const size_t index_data_size  = index_count * sizeof(primitive_index);
			if (vertex_data_size > std::numeric_limits<u32>::max() || index_data_size > std::numeric_limits<u32>::max())
				return false;

			const u32 primitive_count = static_cast<u32>(def_primitives.size());
			out_primitives			  = static_cast<runtime_primitive_t*>(SFG_MALLOC(sizeof(runtime_primitive_t) * primitive_count));
			runtime.vertex_data		  = SFG_MALLOC(vertex_data_size);
			runtime.index_data		  = static_cast<primitive_index*>(SFG_MALLOC(index_data_size));
			if (out_primitives == nullptr || runtime.vertex_data == nullptr || runtime.index_data == nullptr)
				return false;

			u32 vertex_start = 0;
			u32 index_start	 = 0;
			u8* vertex_dst	 = static_cast<u8*>(runtime.vertex_data);
			u8* index_dst	 = reinterpret_cast<u8*>(runtime.index_data);
			for (u32 i = 0; i < primitive_count; ++i)
			{
				const def_primitive_t& primitive = def_primitives[i];
				runtime_primitive_t&   out		 = out_primitives[i];

				out.material	 = primitive.material_index != UINT32_MAX ? materials[primitive.material_index] : NULL_RESOURCE_HANDLE;
				out.vertex_start = vertex_start;
				out.index_start	 = index_start;
				out.vertex_count = static_cast<u32>(primitive.vertices.size());
				out.index_count	 = static_cast<u32>(primitive.indices.size());

				const size_t vertex_bytes = primitive.vertices.size() * sizeof(vertex_t);
				const size_t index_bytes  = primitive.indices.size() * sizeof(primitive_index);
				SFG_MEMCPY(vertex_dst, primitive.vertices.data(), vertex_bytes);
				SFG_MEMCPY(index_dst, primitive.indices.data(), index_bytes);

				vertex_dst += vertex_bytes;
				index_dst += index_bytes;
				vertex_start += out.vertex_count;
				index_start += out.index_count;
			}

			runtime.vertex_data_size = static_cast<u32>(vertex_data_size);
			runtime.index_data_size	 = static_cast<u32>(index_data_size);
			runtime.vertex_count	 = static_cast<u32>(vertex_count);
			runtime.index_count		 = static_cast<u32>(index_count);
			runtime.vertex_stride	 = sizeof(vertex_t);

			internals.primitive_count = primitive_count;
			internals.vertex_count	  = runtime.vertex_count;
			internals.index_count	  = runtime.index_count;
			internals.vertex_stride	  = runtime.vertex_stride;
			return true;
		}
	}

	bool mesh_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		mesh_runtime_t*	   runtime	 = mem.get<mesh_runtime_t>(entry.runtime);
		mesh_internals_t*  internals = mem.get<mesh_internals_t>(entry.internals);
		*runtime					 = {};
		*internals					 = {};

		istream_t stream;
		stream.open(entry.load_data.data, entry.load_data.size);

		mesh_def_t mesh = {};
		if (!reflection_registry_t::get().deserialize_from_stream(type_id_t<mesh_def_t>::value, &mesh, stream))
			return false;

		internals->local_bounds = mesh.local_bounds;
		internals->local_bounds.update_half_extents();

		const bool has_static  = !mesh.static_primitives.empty();
		const bool has_skinned = !mesh.skinned_primitives.empty();
		SFG_ASSERT(!has_static || !has_skinned);
		if (has_static && has_skinned)
			return false;

		bool result = true;
		if (has_static)
		{
			result = build_mesh_data<primitive_static_def_t, mesh_static_primitive_t, vertex_static_t>(mesh.static_primitives, mesh.materials, internals->static_primitives, *runtime, *internals);
		}
		else if (has_skinned)
		{
			result				  = build_mesh_data<primitive_skinned_def_t, mesh_skinned_primitive_t, vertex_skinned_t>(mesh.skinned_primitives, mesh.materials, internals->skinned_primitives, *runtime, *internals);
			runtime->is_skinned	  = 1;
			internals->is_skinned = 1;
		}

		if (!result)
		{
			free_mesh_cpu_data(*runtime, *internals);
			return false;
		}

		return true;
	}

	create_internals_result_e mesh_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		mesh_runtime_t*	   runtime	 = mem.get<mesh_runtime_t>(entry.runtime);
		mesh_internals_t*  internals = mem.get<mesh_internals_t>(entry.internals);

		if (runtime->vertex_data_size != 0)
		{
			resource_desc_t desc = {};
			desc.size			 = runtime->vertex_data_size;
			desc.structure_size	 = runtime->vertex_stride;
			desc.structure_count = runtime->vertex_count;
			desc.flags			 = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
			desc.set_name(mem.get_text(entry.debug_name));
			render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, desc, MESH_RESOURCE_USER_DATA_VERTEX_BUFFER);
			internals->pending_count++;
		}

		if (runtime->index_data_size != 0)
		{
			resource_desc_t desc = {};
			desc.size			 = runtime->index_data_size;
			desc.structure_size	 = sizeof(primitive_index);
			desc.structure_count = runtime->index_count;
			desc.flags			 = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
			desc.set_name(mem.get_text(entry.debug_name));
			render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, desc, MESH_RESOURCE_USER_DATA_INDEX_BUFFER);
			internals->pending_count++;
		}

		if (internals->pending_count == 0)
		{
			free_mesh_runtime_cpu_data(*runtime);
			return create_internals_result_e::ready;
		}

		return create_internals_result_e::queued;
	}

	resource_ready_result_e mesh_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		mesh_runtime_t*	   runtime	 = mem.get<mesh_runtime_t>(entry.runtime);
		mesh_internals_t*  internals = mem.get<mesh_internals_t>(entry.internals);

		SFG_ASSERT(completion.kind == render_resource_kind_e::resource);
		SFG_ASSERT(internals->pending_count > 0);

		if (completion.state == resource_state_e::failed)
		{
			internals->had_failure = 1;
		}
		else if (completion.user_data == MESH_RESOURCE_USER_DATA_VERTEX_BUFFER)
		{
			internals->vertex_buffer	   = completion.resource;
			internals->vertex_buffer_index = completion.gpu_index;
			render_resources_t::get().enqueue_data_upload(internals->vertex_buffer, runtime->vertex_data, runtime->vertex_data_size);
		}
		else
		{
			SFG_ASSERT(completion.user_data == MESH_RESOURCE_USER_DATA_INDEX_BUFFER);
			internals->index_buffer		  = completion.resource;
			internals->index_buffer_index = completion.gpu_index;
			render_resources_t::get().enqueue_data_upload(internals->index_buffer, runtime->index_data, runtime->index_data_size);
		}

		internals->pending_count--;
		if (internals->pending_count != 0)
			return resource_ready_result_e::pending;

		free_mesh_runtime_cpu_data(*runtime);
		if (internals->had_failure)
		{
			render_resources_t::get().enqueue_destroy_resource(internals->vertex_buffer);
			render_resources_t::get().enqueue_destroy_resource(internals->index_buffer);
			free_mesh_internals_cpu_data(*internals);
			*internals = {};
			return resource_ready_result_e::failed;
		}

		return resource_ready_result_e::ready;
	}

	void mesh_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		mesh_runtime_t*	   runtime	 = mem.get<mesh_runtime_t>(entry.runtime);
		mesh_internals_t*  internals = mem.get<mesh_internals_t>(entry.internals);

		free_mesh_cpu_data(*runtime, *internals);
		render_resources_t::get().enqueue_destroy_resource(internals->vertex_buffer);
		render_resources_t::get().enqueue_destroy_resource(internals->index_buffer);
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
		.initial_load_offset = 0,
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.async_load			 = false,
		.load				 = mesh_loader_t::load,
		.load_v2			 = mesh_loader_t::load,
		.create_internals	 = mesh_loader_t::create_internals,
		.resource_ready		 = mesh_loader_t::resource_ready,
		.destroy_internals	 = mesh_loader_t::destroy_internals,
	};
}
