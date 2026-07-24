/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "common_resources.hpp"
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	class skybox_hdr_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC		  = make_resource_wire_magic('H', 'S', 'K', 'Y');
		static constexpr u32 WIRE_VERSION	  = 3;
		static constexpr u8	 MAX_FACES		  = 6;
		static constexpr u8	 MAX_MIPS		  = 16;
		static constexpr u8	 MAX_SUBRESOURCES = MAX_FACES * MAX_MIPS;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct skybox_hdr_texture_block_t
	{
		texture_buffer_t buffers[skybox_hdr_loader_t::MAX_SUBRESOURCES] = {};
		format_e		 format											= format_e::undefined;
		vec2u16_t		 size											= vec2u16_t::zero;
		u8				 face_count										= 0;
		u8				 mip_count										= 0;
	};

	struct skybox_hdr_runtime_t
	{
		skybox_hdr_texture_block_t radiance		   = {};
		skybox_hdr_texture_block_t irradiance	   = {};
		skybox_hdr_texture_block_t prefilter	   = {};
		skybox_hdr_texture_block_t brdf_lut		   = {};
		vec2u16_t				   radiance_size   = vec2u16_t::zero;
		vec2u16_t				   irradiance_size = vec2u16_t::zero;
		vec2u16_t				   prefilter_size  = vec2u16_t::zero;
		vec2u16_t				   brdf_lut_size   = vec2u16_t::zero;
		f32						   intensity	   = 1.0f;
		f32						   rotation		   = 0.0f;
		u8						   prefilter_mips  = 1;
	};

	struct skybox_hdr_internals_t
	{
		render_resource_handle_t radiance_staging[skybox_hdr_loader_t::MAX_FACES]	= {};
		render_resource_handle_t irradiance_staging[skybox_hdr_loader_t::MAX_FACES] = {};
		render_resource_handle_t prefilter_staging[skybox_hdr_loader_t::MAX_FACES]	= {};
		render_resource_handle_t brdf_lut_staging									= {};
		render_resource_handle_t radiance_texture									= {};
		render_resource_handle_t irradiance_texture									= {};
		render_resource_handle_t prefilter_texture									= {};
		render_resource_handle_t brdf_lut_texture									= {};
	};

	extern const resource_type_desc_t skybox_hdr_resource_desc;
}
