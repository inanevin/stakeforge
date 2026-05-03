// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct texture_mip_t
	{
		u32 offset = 0;
		u32 size   = 0;
		u32 width  = 0;
		u32 height = 0;
	};

	struct texture_data_t
	{
		chunk_handle32_t pixels		 = {};
		u32				 pixels_size = 0;
		u32				 width		 = 0;
		u32				 height		 = 0;
		u8				 channels	 = 0;
		u8				 mip_count	 = 0;
		u8				 is_linear	 = 0;
		texture_mip_t	 mips[16]	 = {};
	};

	struct texture_internals_t
	{
		u32 reserved = 0;
	};

	struct texture_config_t
	{
		bool generate_mipmaps = false;
		bool is_linear		  = false;
	};

	extern bool texture_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool texture_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void texture_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void texture_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void texture_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t texture_resource_desc;
	
	inline constexpr u32 texture_wire_magic	  = 0x53465458;
	inline constexpr u32 texture_wire_version = 1;
	inline constexpr u8	 texture_max_mips	  = 16;
}
