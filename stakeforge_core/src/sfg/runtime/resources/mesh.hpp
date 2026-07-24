// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "mesh_def.hpp"

#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	struct mesh_primitive_runtime_t
	{
		u32 material_index = UINT32_MAX;
		u32 start_index	   = 0;
		u32 start_vertex   = 0;
		u32 index_count	   = 0;
	};

	struct mesh_runtime_t
	{
		chunk_handle32_t vertex_data	  = {};
		chunk_handle32_t index_data		  = {};
		chunk_handle32_t primitives		  = {};
		u32				 vertex_data_size = 0;
		u32				 index_data_size  = 0;
		u32				 primitive_count  = 0;
		u32				 vertex_count	  = 0;
		u32				 index_count	  = 0;
		u32				 vertex_stride	  = 0;
		u32				 index_stride	  = 0;
		u8				 is_skinned		  = 0;
	};

	struct mesh_internals_t
	{
		render_resource_handle_t vertex_buffer	 = {};
		render_resource_handle_t index_buffer	 = {};
		aabb_t					 local_bounds	 = {};
		u32						 primitive_count = 0;
		u32						 vertex_count	 = 0;
		u32						 index_count	 = 0;
		u32						 vertex_stride	 = 0;
		u8						 is_skinned		 = 0;
	};

	class mesh_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('M', 'E', 'S', 'H');
		static constexpr u32 WIRE_VERSION = 5;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static bool runtime_load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t mesh_resource_desc;
}
