// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "mesh_def.hpp"

#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	struct mesh_static_primitive_t
	{
		resource_handle_t material	   = NULL_RESOURCE_HANDLE;
		u32				  vertex_start = 0;
		u32				  index_start  = 0;
		u32				  vertex_count = 0;
		u32				  index_count  = 0;
	};

	struct mesh_skinned_primitive_t
	{
		resource_handle_t material	   = NULL_RESOURCE_HANDLE;
		u32				  vertex_start = 0;
		u32				  index_start  = 0;
		u32				  vertex_count = 0;
		u32				  index_count  = 0;
	};

	struct mesh_runtime_t
	{
		void*			 vertex_data	  = nullptr;
		primitive_index* index_data		  = nullptr;
		u32				 vertex_data_size = 0;
		u32				 index_data_size  = 0;
		u32				 vertex_count	  = 0;
		u32				 index_count	  = 0;
		u32				 vertex_stride	  = 0;
		u8				 is_skinned		  = 0;
	};

	struct mesh_internals_t
	{
		mesh_static_primitive_t*  static_primitives	 = nullptr;
		mesh_skinned_primitive_t* skinned_primitives = nullptr;
		render_resource_handle_t  vertex_buffer		 = {};
		render_resource_handle_t  index_buffer		 = {};
		aabb_t					  local_bounds		 = {};
		u32						  primitive_count	 = 0;
		u32						  vertex_count		 = 0;
		u32						  index_count		 = 0;
		u32						  vertex_stride		 = 0;
		u8						  is_skinned		 = 0;
	};

	class mesh_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('M', 'E', 'S', 'H');
		static constexpr u32 WIRE_VERSION = 3;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t mesh_resource_desc;
}
