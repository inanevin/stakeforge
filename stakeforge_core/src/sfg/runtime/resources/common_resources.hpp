// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/common/string_id.hpp>

namespace sfg
{
	class resource_manager_t;
	class istream_t;

	enum class resource_type_e : u8
	{
		invalid,
		audio,
		font,
		mesh,
		skeleton,
		animation,
		particle_properties,
		material,
		shader,
		texture,
		texture_sampler,
		physical_material,
		prefab,
		animation_state_machine,
		count,
	};

	struct resource_handle_tag_t
	{
	};
	typedef pool_handle_t<u32, resource_handle_tag_t> resource_handle_t;

	inline constexpr u8 resource_type_max = static_cast<u8>(resource_type_e::count);

	enum class resource_state_e : u8
	{
		queued,
		cpu_ready,
		internals_queued,
		ready,
		failed,
	};

	struct resource_entry_t
	{
		sid_t			 hash	   = 0;
		chunk_handle32_t cpu_data  = {};
		chunk_handle32_t internals = {};
		chunk_handle32_t payload   = {};
		u32				 ref_count = 0;
		resource_state_e state	   = resource_state_e::cpu_ready;
		resource_type_e	 type	   = resource_type_e::invalid;
	};

	struct resource_context_t
	{
		resource_manager_t& resource_manager;
	};

	struct resource_internals_completion_t
	{
		const void* data = nullptr;
		u32			size = 0;
	};

	struct resource_type_desc_t
	{
		using load_fn_t				  = bool (*)(resource_entry_t& entry, istream_t& data, resource_context_t& ctx);
		using create_internals_fn_t	  = bool (*)(resource_entry_t& entry, resource_context_t& ctx);
		using complete_internals_fn_t = bool (*)(resource_entry_t& entry, resource_context_t& ctx, const resource_internals_completion_t& completion);
		using destroy_internals_fn_t  = void (*)(resource_entry_t& entry, resource_context_t& ctx);
		using unload_fn_t			  = void (*)(resource_entry_t& entry, resource_context_t& ctx);
		using unload_cpu_fn_t		  = void (*)(resource_entry_t& entry, resource_context_t& ctx);

		resource_type_e type				= resource_type_e::invalid;
		u32				data_size			= 0;
		u32				data_alignment		= 0;
		u32				internals_size		= 0;
		u32				internals_alignment = 0;

		load_fn_t				load			   = nullptr;
		create_internals_fn_t	create_internals   = nullptr;
		complete_internals_fn_t complete_internals = nullptr;
		destroy_internals_fn_t	destroy_internals  = nullptr;
		unload_fn_t				unload			   = nullptr;
		unload_cpu_fn_t			unload_cpu		   = nullptr;
	};

	extern const resource_type_desc_t* const g_resource_type_descs[resource_type_max];

	inline const resource_type_desc_t* find_resource_type_desc(resource_type_e type)
	{
		const u8 t = static_cast<u8>(type);
		if (t >= resource_type_max)
			return nullptr;
		return g_resource_type_descs[t];
	}

	resource_type_e resolve_resource_type(const string_t& s);
}
