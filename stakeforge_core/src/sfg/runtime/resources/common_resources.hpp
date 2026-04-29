// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/span.hpp"
#include "memory/chunk_handle.hpp"

namespace sfg
{
	class resource_manager_t;

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

	inline constexpr u8 resource_type_max = static_cast<u8>(resource_type_e::count);

	enum class resource_state_e : u8
	{
		unloaded,
		load_queued,
		cpu_ready,
		internals_queued,
		ready,
		failed,
	};

	struct resource_entry_t
	{
		u64				 hash		 = 0;
		resource_type_e	 type		 = resource_type_e::invalid;
		u32				 ref_count	 = 0;
		resource_state_e state		 = resource_state_e::unloaded;
		chunk_handle32_t source_path = {};
		chunk_handle32_t cooked_data = {};
		chunk_handle32_t metadata	 = {};
		chunk_handle32_t internals	 = {};
	};

	struct resource_context_t
	{
		resource_manager_t* resource_manager = nullptr;
	};

	struct resource_type_desc_t
	{
		using load_cpu_fn_t			 = bool (*)(resource_entry_t& entry, span_t<const u8> data, resource_context_t& ctx);
		using create_internals_fn_t	 = bool (*)(resource_entry_t& entry, resource_context_t& ctx);
		using destroy_internals_fn_t = void (*)(resource_entry_t& entry, resource_context_t& ctx);
		using unload_cpu_fn_t		 = void (*)(resource_entry_t& entry, resource_context_t& ctx);

		resource_type_e type				= resource_type_e::invalid;
		u32				metadata_size		= 0;
		u32				metadata_alignment	= 0;
		u32				internals_size		= 0;
		u32				internals_alignment = 0;

		load_cpu_fn_t		   load_cpu			 = nullptr;
		create_internals_fn_t  create_internals	 = nullptr;
		destroy_internals_fn_t destroy_internals = nullptr;
		unload_cpu_fn_t		   unload_cpu		 = nullptr;
	};
}
