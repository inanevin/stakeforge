// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "mesh_def.hpp"

#include <sfg/gfx/common/gfx_constants.hpp>

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
		mesh_static_primitive_t*  static_primitives	  = nullptr;
		mesh_skinned_primitive_t* skinned_primitives  = nullptr;
		gfx_resource_handle		  vertex_buffer		  = {};
		gfx_resource_handle		  index_buffer		  = {};
		gpu_index_t				  vertex_buffer_index = NULL_GPU_INDEX;
		gpu_index_t				  index_buffer_index  = NULL_GPU_INDEX;
		aabb_t					  local_bounds		  = {};
		u32						  primitive_count	  = 0;
		u32						  vertex_count		  = 0;
		u32						  index_count		  = 0;
		u32						  vertex_stride		  = 0;
		u8						  pending_count		  = 0;
		u8						  had_failure		  = 0;
		u8						  is_skinned		  = 0;
	};

	class mesh_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('M', 'E', 'S', 'H');
		static constexpr u32 WIRE_VERSION = 3;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static bool						 load(resource_entry_t& entry, resource_context_t& ctx, ostream_t& stream);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static resource_ready_result_e	 resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t mesh_resource_desc;
}
