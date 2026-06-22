// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "material_limits.hpp"
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/data/bitmask.hpp>
namespace sfg
{
	class istream_t;
	enum class material_parameter_type_e : u8;

	class material_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('M', 'A', 'T', 'L');
		static constexpr u32 WIRE_VERSION = 3;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static bool						 load(resource_entry_t& entry, resource_context_t& ctx, ostream_t& stream);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static resource_ready_result_e	 resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct material_runtime_parameter_t
	{
		f32						  values[4] = {};
		material_parameter_type_e type		= {};
	};

	struct material_runtime_t
	{
		material_runtime_parameter_t parameters[MATERIAL_MAX_PARAMETERS]	= {};
		sid_t						 texture_guids[MATERIAL_MAX_TEXTURES]	= {};
		gpu_index_t					 texture_indices[MATERIAL_MAX_TEXTURES] = {};
		sid_t						 shader_guid							= NULL_SID;
		sid_t						 sampler_guid							= NULL_SID;
		u32							 parameter_data_size					= 0;
		u32							 parameter_count						= 0;
		u32							 texture_count							= 0;
		bitmask_t<u32>				 pass_flags								= 0;
		u8							 double_sided							= 0;
		u8							 use_alpha_cutoff						= 0;
	};

	struct material_internals_t
	{
		gfx_resource_handle parameter_buffer = {};
		gfx_resource_handle texture_buffer	 = {};
		gpu_index_t			parameter_index	 = NULL_GPU_INDEX;
		gpu_index_t			texture_index	 = NULL_GPU_INDEX;
		gpu_index_t			sampler_index	 = NULL_GPU_INDEX;
		u8					pending_count	 = 0;
		u8					had_failure		 = 0;
	};

	extern const resource_type_desc_t material_resource_desc;
}
