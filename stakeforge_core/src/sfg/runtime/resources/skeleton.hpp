// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "skeleton_def.hpp"

namespace sfg
{
	class istream_t;

	class skeleton_loader_t
	{
	public:
		static constexpr u32 MAX_JOINTS	  = 96;
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('S', 'K', 'E', 'L');
		static constexpr u32 WIRE_VERSION = 3;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct skeleton_joint_runtime_t
	{
		mat4x3_t inverse_bind = mat4x3_t::identity;
		sid_t	 name_hash	  = NULL_SID;
		u32		 parent_index = SKELETON_JOINT_NO_PARENT;
	};

	struct skeleton_runtime_t
	{
		skeleton_joint_runtime_t joints[skeleton_loader_t::MAX_JOINTS] = {};
		u32						 root_joint_index					   = SKELETON_JOINT_NO_PARENT;
		u32						 joint_count						   = 0;
	};

	struct skeleton_internals_t
	{
		u32 reserved = 0;
	};

	extern const resource_type_desc_t skeleton_resource_desc;
}
