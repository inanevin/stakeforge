// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct physical_material_runtime_t
	{
		f32 restitution		= 0.0f;
		f32 friction		= 0.2f;
		f32 angular_damping = 0.05f;
		f32 linear_damping	= 0.05f;
	};

	struct physical_material_internals_t
	{
		u32 reserved = 0;
	};

	class physical_material_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = 0x5346504D;
		static constexpr u32 WIRE_VERSION = 1;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t physical_material_resource_desc;
}
