// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "shader_types.hpp"
#include <sfg/data/bitmask.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	class istream_t;

	class shader_loader_t
	{
	public:
		static constexpr u8	 MAX_COMPILE_VARIANTS  = 16;
		static constexpr u8	 MAX_PSO_VARIANTS	   = 32;
		static constexpr u8	 MAX_STAGE_PER_VARIANT = 4;
		static constexpr u32 WIRE_MAGIC			   = make_resource_wire_magic('S', 'H', 'D', 'R');
		static constexpr u32 WIRE_VERSION		   = 7;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct shader_runtime_stage_entry_t
	{
		span_t<u8> data	 = {};
		u8		   stage = 0;
	};

	struct shader_runtime_compile_variant_t
	{
		u8							 stage_count									= 0;
		shader_runtime_stage_entry_t stages[shader_loader_t::MAX_STAGE_PER_VARIANT] = {};
	};

	struct shader_runtime_pso_variant_t
	{
		span_t<u8> desc_stream			 = {};
		u32		   variant_flags		 = 0;
		u8		   compile_variant_index = 0;
	};

	struct shader_runtime_t
	{
		shader_runtime_pso_variant_t	 pso_variants[shader_loader_t::MAX_PSO_VARIANTS]		 = {};
		shader_runtime_compile_variant_t compile_variants[shader_loader_t::MAX_COMPILE_VARIANTS] = {};
		shader_type_e					 type													 = shader_type_e::invalid;
		u8								 compile_variant_count									 = 0;
		u8								 pso_variant_count										 = 0;
	};

	struct shader_internals_t
	{
		render_resource_handle_t psos[shader_loader_t::MAX_PSO_VARIANTS]	  = {};
		bitmask_t<u32>			 pso_flags[shader_loader_t::MAX_PSO_VARIANTS] = {};
		u8						 pso_count									  = 0;

		render_resource_handle_t find_pso(bitmask_t<u32> flags) const;
	};

	extern const resource_type_desc_t shader_resource_desc;
}
