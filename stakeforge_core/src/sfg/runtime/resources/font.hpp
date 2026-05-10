// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/memory/chunk_handle.hpp>

namespace sfg
{
	class font_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = 0x53464E54;
		static constexpr u32 WIRE_VERSION = 5;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct font_runtime_t
	{
		void*			 face		= nullptr;
		const u8*		 ttf_data	= nullptr;
		chunk_handle32_t ttf_chunk	= {};
		chunk_handle32_t face_chunk = {};
		u32				 ttf_size	= 0;
		i32				 ascent		= 0;
		i32				 descent	= 0;
		i32				 line_gap	= 0;
		u64				 face_id	= 0;
	};

	struct font_internals_t
	{
		u32 reserved = 0;
	};

	extern const resource_type_desc_t font_resource_desc;
}
