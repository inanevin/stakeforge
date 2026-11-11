// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/sfg_exports.hpp"

#include "core/runtime/resources/common_resources.hpp"
#include "core/memory/chunk_handle.hpp"

namespace sfg
{
	class SFG_API font_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('F', 'O', 'N', 'T');
		static constexpr u32 WIRE_VERSION = 0;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct font_runtime_t
	{
		void*			 face	   = nullptr;
		const u8*		 ttf_data  = nullptr;
		chunk_handle32_t ttf_chunk = {};
		u32				 ttf_size  = 0;
		i32				 ascent	   = 0;
		i32				 descent   = 0;
		i32				 height	   = 0;
		u64				 face_id   = 0;
	};

	struct font_internals_t
	{
		u32 reserved = 0;
	};

	extern SFG_API const resource_type_desc_t font_resource_desc;
}
