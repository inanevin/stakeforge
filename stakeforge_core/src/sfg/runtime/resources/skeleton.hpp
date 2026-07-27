// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "skeleton_def.hpp"

namespace sfg
{
	class skeleton_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('S', 'K', 'E', 'L');
		static constexpr u32 WIRE_VERSION = 7;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct skeleton_joint_runtime_t
	{
		mat4x3_t		 local		  = mat4x3_t::identity;
		mat4x3_t		 inverse_bind = mat4x3_t::identity;
		sid_t			 name_hash	  = NULL_SID;
		chunk_handle32_t name		  = {};
		u32				 parent_index = SKELETON_JOINT_NO_PARENT;
	};

	struct skeleton_slot_runtime_t
	{
		sid_t			 slot_name_hash	  = NULL_SID;
		chunk_handle32_t debug_name		  = {};
		quat_t			 local_rotation	  = quat_t::identity;
		vec3f_t			 local_position	  = vec3f_t::zero;
		u32				 slot_joint_index = SKELETON_JOINT_NO_PARENT;
	};

	struct skeleton_runtime_t
	{
		chunk_handle32_t joints			  = {};
		chunk_handle32_t evaluation_order = {};
		chunk_handle32_t slots			  = {};
		u32				 root_joint_index = SKELETON_JOINT_NO_PARENT;
		u32				 joint_count	  = 0;
		u32				 slot_count		  = 0;
	};

	struct skeleton_internals_t
	{
		u32 reserved = 0;
	};

	extern const resource_type_desc_t skeleton_resource_desc;
}
