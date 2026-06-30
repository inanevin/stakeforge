// Copyright (c) 2025 Inan Evin
#pragma once

#include "resource_handle.hpp"
#include "resource_type.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <cstddef>

namespace sfg
{
	class resource_manager_t;
	class istream_t;
	class ostream_t;
	class resource_file_system_t;

	inline constexpr u32 make_resource_wire_magic(char c0, char c1, char c2, char c3)
	{
		const u32 b0 = static_cast<u32>(static_cast<u8>(c0)) << 24;
		const u32 b1 = static_cast<u32>(static_cast<u8>(c1)) << 16;
		const u32 b2 = static_cast<u32>(static_cast<u8>(c2)) << 8;
		const u32 b3 = static_cast<u32>(static_cast<u8>(c3));
		return b0 | b1 | b2 | b3;
	}

	struct resource_header_t
	{
		u32 magic		= 0;
		u32 version		= 0;
		u64 source_tick = 0;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	ostream_t make_resource_stream(const resource_header_t& header, const ostream_t& payload);

	enum class resource_state_e : u8
	{
		failed,
		ready,
		ready_preview,
	};

	struct resource_entry_t
	{
		sid_t			 hash		= 0;
		chunk_handle32_t runtime	= {};
		chunk_handle32_t internals	= {};
		chunk_handle32_t debug_name = {};
		u32				 ref_count	= 0;
		resource_state_e state		= resource_state_e::failed;
		resource_type_e	 type		= resource_type_e::invalid;
	};

	struct resource_context_t
	{
		resource_manager_t& resource_manager;
	};

	struct resource_type_desc_t
	{
		using load_fn_t	  = bool (*)(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs);
		using unload_fn_t = void (*)(resource_entry_t& entry, resource_context_t& ctx);

		resource_type_e type				= resource_type_e::invalid;
		u32				runtime_size		= 0;
		u32				runtime_alignment	= 0;
		u32				internals_size		= 0;
		u32				internals_alignment = 0;
		u32				wire_magic			= 0;
		u32				wire_version		= 0;
		bool			use_async_load		= false;

		load_fn_t	load	   = nullptr;
		load_fn_t	async_load = nullptr;
		unload_fn_t unload	   = nullptr;
	};

	extern const resource_type_desc_t* const g_resource_type_descs[RESOURCE_TYPE_MAX];

	inline const resource_type_desc_t* find_resource_type_desc(resource_type_e type)
	{
		const u8 t = static_cast<u8>(type);
		if (t >= RESOURCE_TYPE_MAX)
			return nullptr;
		return g_resource_type_descs[t];
	}

	struct resource_type_reflection_t
	{
		resource_type_reflection_t();
	};

	inline resource_type_reflection_t g_reflect_resource_type;
}
