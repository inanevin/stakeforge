// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "material_limits.hpp"
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/data/bitmask.hpp>
namespace sfg
{
	enum class material_parameter_type_e : u8;

	class material_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('M', 'A', 'T', 'L');
		static constexpr u32 WIRE_VERSION = 3;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct material_runtime_parameter_t
	{
		f32						  values[4] = {};
		material_parameter_type_e type		= {};
	};

	struct material_runtime_t
	{
		material_runtime_parameter_t parameters[SFG_MATERIAL_MAX_PARAMS]	  = {};
		sid_t						 texture_guids[SFG_MATERIAL_MAX_TEXTURES] = {};
		sid_t						 shader_guid							  = NULL_SID;
		sid_t						 sampler_guid							  = NULL_SID;
		u32							 parameter_data_size					  = 0;
		u32							 parameter_count						  = 0;
		u32							 texture_count							  = 0;
		bitmask_t<u32>				 pass_flags								  = 0;
		u8							 double_sided							  = 0;
		u8							 use_alpha_cutoff						  = 0;
	};

	struct material_internals_t
	{
		render_resource_handle_t parameter_buffer = {};
	};

	extern const resource_type_desc_t material_resource_desc;
}
