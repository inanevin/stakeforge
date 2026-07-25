// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "material_limits.hpp"
#include "shader_data_definition.hpp"
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/data/bitmask.hpp>
namespace sfg
{
	class material_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('M', 'A', 'T', 'L');
		static constexpr u32 WIRE_VERSION = 8;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct material_runtime_parameter_t
	{
		union {
			f32 values[4] = {};
			u32 values_u32[4];
		};
		u16					data_offset = 0;
		shader_param_type_e type		= shader_param_type_e::invalid;
		shader_param_hint_e hint		= shader_param_hint_e::none;
	};

	struct material_runtime_t
	{
		material_runtime_parameter_t parameters[SFG_MATERIAL_MAX_PARAMS]			= {};
		sid_t						 parameter_name_hashes[SFG_MATERIAL_MAX_PARAMS] = {};
		sid_t						 texture_name_hashes[SFG_MATERIAL_MAX_TEXTURES] = {};
		sid_t						 sampler_name_hashes[SFG_MATERIAL_MAX_TEXTURES] = {};
		sid_t						 texture_guids[SFG_MATERIAL_MAX_TEXTURES]		= {};
		sid_t						 sampler_guids[SFG_MATERIAL_MAX_TEXTURES]		= {};
		sid_t						 shader_guid									= NULL_SID;
		shader_texture_type_e		 texture_types[SFG_MATERIAL_MAX_TEXTURES]		= {};
		u32							 parameter_data_size							= 0;
		u32							 parameter_count								= 0;
		u32							 texture_count									= 0;
		u32							 sampler_count									= 0;
		bitmask_t<u32>				 pass_flags										= 0;
		u8							 double_sided									= 0;
		u8							 use_alpha_cutoff								= 0;
		u8							 parameters_dirty								= 0;
	};

	struct material_internals_t
	{
		render_resource_handle_t parameter_buffers[BACK_BUFFER_COUNT] = {};
	};

	void pack_material_parameters(const material_runtime_t& material, u8* data);

	extern const resource_type_desc_t material_resource_desc;
}
