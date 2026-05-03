// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "shader_types.hpp"

namespace sfg
{
	class istream_t;

	inline constexpr u8 shader_max_compile_variants	  = 16;
	inline constexpr u8 shader_max_pso_variants		  = 32;
	inline constexpr u8 shader_max_stages_per_variant = 4;

	struct shader_stage_entry_t
	{
		u32 offset = 0;
		u32 size   = 0;
		u8	stage  = 0;
	};

	struct shader_compile_variant_t
	{
		u8					 stage_count						   = 0;
		shader_stage_entry_t stages[shader_max_stages_per_variant] = {};
	};

	struct shader_pso_variant_t
	{
		u32 variant_flags		  = 0;
		u8	compile_variant_index = 0;
	};

	struct shader_data_t
	{
		chunk_handle32_t		 blobs										   = {};
		u32						 blobs_size									   = 0;
		shader_type_e			 type										   = shader_type_e::invalid;
		u8						 compile_variant_count						   = 0;
		u8						 pso_variant_count							   = 0;
		shader_compile_variant_t compile_variants[shader_max_compile_variants] = {};
		shader_pso_variant_t	 pso_variants[shader_max_pso_variants]		   = {};
	};

	struct shader_internals_t
	{
		u32 reserved = 0;
	};

	struct shader_config_t
	{
		shader_type_e type = shader_type_e::invalid;
	};

	extern bool shader_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool shader_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void shader_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void shader_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void shader_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t shader_resource_desc;

	inline constexpr u32 shader_wire_magic	 = 0x52444853;
	inline constexpr u32 shader_wire_version = 2;
}
