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

#include "skybox_hdr.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
#define SFG_SKYBOX_HDR_TEX_RADIANCE	  0
#define SFG_SKYBOX_HDR_TEX_IRRADIANCE 1
#define SFG_SKYBOX_HDR_TEX_PREFILTER  2
#define SFG_SKYBOX_HDR_TEX_BRDF_LUT	  3

	namespace
	{
		u8 get_subresource_index(u8 face, u8 mip)
		{
			return static_cast<u8>(face * skybox_hdr_loader_t::MAX_MIPS + mip);
		}

		void load_texture_block(istream_t& stream, skybox_hdr_texture_block_t& block)
		{
			stream >> block.format;
			stream >> block.size;
			stream >> block.face_count;
			stream >> block.mip_count;

			for (u8 face = 0; face < block.face_count; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					texture_buffer_t& buffer = block.buffers[get_subresource_index(face, mip)];
					stream >> buffer.size;
					stream >> buffer.row_pitch;
					stream >> buffer.data_size;
					buffer.bpp	  = format_get_bpp(block.format);
					buffer.pixels = stream.get_data_current();
					stream.skip_by(buffer.data_size);
				}
			}
		}

		u32 get_face_staging_size(const skybox_hdr_texture_block_t& block)
		{
			u32 staging_size = 0;
			for (u8 mip = 0; mip < block.mip_count; ++mip)
			{
				const texture_buffer_t& buffer = block.buffers[get_subresource_index(0, mip)];
				staging_size				   = gfx_backend::align_texture_size(staging_size);
				staging_size += gfx_backend::align_texture_size_pitch(buffer.row_pitch) * format_get_row_count(block.format, buffer.size.y);
			}
			return gfx_backend::align_texture_size(staging_size);
		}

		texture_desc_t make_texture_desc(const skybox_hdr_texture_block_t& block, const char* name)
		{
			texture_desc_t desc		  = {};
			desc.texture_format		  = block.format;
			desc.size				  = block.size;
			desc.flags				  = texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
			desc.mip_levels			  = block.mip_count;
			desc.array_length		  = block.face_count;
			desc.samples			  = 1;
			desc.views[0].mip_count	  = 0;
			desc.views[0].level_count = 0;
			desc.views[0].is_cubemap  = block.face_count == skybox_hdr_loader_t::MAX_FACES ? 1 : 0;
			if (desc.views[0].is_cubemap != 0)
				desc.flags.set(texture_flags::tf_cubemap);
			desc.set_name(name);
			return desc;
		}

		resource_desc_t make_staging_desc(const skybox_hdr_texture_block_t& block)
		{
			resource_desc_t desc = {};
			desc.size			 = get_face_staging_size(block);
			desc.flags			 = resource_flags::rf_cpu_visible;
			desc.set_name("skybox_hdr_upload_staging");
			return desc;
		}

		void enqueue_block_upload(const skybox_hdr_texture_block_t& block, render_resource_handle_t texture, const render_resource_handle_t* staging)
		{
			for (u8 face = 0; face < block.face_count; ++face)
			{
				texture_buffer_t upload_mips[skybox_hdr_loader_t::MAX_MIPS] = {};
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					const texture_buffer_t& src = block.buffers[get_subresource_index(face, mip)];
					texture_buffer_t&		dst = upload_mips[mip];
					dst							= src;
					dst.pixels					= static_cast<u8*>(SFG_MALLOC(src.data_size));
					SFG_MEMCPY(dst.pixels, src.pixels, src.data_size);
				}

				const render_texture_upload_desc_t upload = {
					.mips			   = {.data = upload_mips, .size = block.mip_count},
					.texture		   = texture,
					.staging		   = staging[face],
					.target_states	   = resource_state_ps_resource,
					.destination_slice = face,
					.ownership		   = texture_data_ownership_e::c_free,
				};
				render_resources_t::get().enqueue_texture_upload(upload);
			}
		}

		void enqueue_destroy_skybox_resources(const skybox_hdr_internals_t& internals)
		{
			render_resources_t::get().enqueue_destroy_texture(internals.radiance_texture);
			render_resources_t::get().enqueue_destroy_texture(internals.irradiance_texture);
			render_resources_t::get().enqueue_destroy_texture(internals.prefilter_texture);
			render_resources_t::get().enqueue_destroy_texture(internals.brdf_lut_texture);
			for (u8 i = 0; i < skybox_hdr_loader_t::MAX_FACES; ++i)
			{
				render_resources_t::get().enqueue_destroy_resource(internals.radiance_staging[i]);
				render_resources_t::get().enqueue_destroy_resource(internals.irradiance_staging[i]);
				render_resources_t::get().enqueue_destroy_resource(internals.prefilter_staging[i]);
			}
			render_resources_t::get().enqueue_destroy_resource(internals.brdf_lut_staging);
		}
	}

	bool skybox_hdr_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read HDR skybox resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);

		if (payload.empty())
		{
			SFG_ERR("failed to decompress HDR skybox payload: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t&	  mem	  = ctx.resource_manager.get_memory();
		skybox_hdr_runtime_t* runtime = mem.get<skybox_hdr_runtime_t>(entry.runtime);
		*runtime					  = {};

		payload >> runtime->radiance_size;
		payload >> runtime->irradiance_size;
		payload >> runtime->prefilter_size;
		payload >> runtime->brdf_lut_size;
		payload >> runtime->intensity;
		payload >> runtime->rotation;
		payload >> runtime->prefilter_mips;

		load_texture_block(payload, runtime->radiance);
		load_texture_block(payload, runtime->irradiance);
		load_texture_block(payload, runtime->prefilter);
		load_texture_block(payload, runtime->brdf_lut);

		skybox_hdr_internals_t* internals = mem.get<skybox_hdr_internals_t>(entry.internals);
		*internals						  = {};

		render_resources_t& render_resources = render_resources_t::get();
		internals->radiance_texture			 = render_resources.enqueue_create_texture(make_texture_desc(runtime->radiance, "skybox_hdr_radiance"));
		internals->irradiance_texture		 = render_resources.enqueue_create_texture(make_texture_desc(runtime->irradiance, "skybox_hdr_irradiance"));
		internals->prefilter_texture		 = render_resources.enqueue_create_texture(make_texture_desc(runtime->prefilter, "skybox_hdr_prefilter"));
		internals->brdf_lut_texture			 = render_resources.enqueue_create_texture(make_texture_desc(runtime->brdf_lut, "skybox_hdr_brdf_lut"));

		const resource_desc_t radiance_staging_desc	  = make_staging_desc(runtime->radiance);
		const resource_desc_t irradiance_staging_desc = make_staging_desc(runtime->irradiance);
		const resource_desc_t prefilter_staging_desc  = make_staging_desc(runtime->prefilter);
		const resource_desc_t brdf_lut_staging_desc	  = make_staging_desc(runtime->brdf_lut);

		for (u8 face = 0; face < runtime->radiance.face_count; ++face)
			internals->radiance_staging[face] = render_resources.enqueue_create_resource(radiance_staging_desc);
		for (u8 face = 0; face < runtime->irradiance.face_count; ++face)
			internals->irradiance_staging[face] = render_resources.enqueue_create_resource(irradiance_staging_desc);
		for (u8 face = 0; face < runtime->prefilter.face_count; ++face)
			internals->prefilter_staging[face] = render_resources.enqueue_create_resource(prefilter_staging_desc);
		internals->brdf_lut_staging = render_resources.enqueue_create_resource(brdf_lut_staging_desc);

		enqueue_block_upload(runtime->radiance, internals->radiance_texture, internals->radiance_staging);
		enqueue_block_upload(runtime->irradiance, internals->irradiance_texture, internals->irradiance_staging);
		enqueue_block_upload(runtime->prefilter, internals->prefilter_texture, internals->prefilter_staging);
		enqueue_block_upload(runtime->brdf_lut, internals->brdf_lut_texture, &internals->brdf_lut_staging);

		return true;
	}

	void skybox_hdr_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		mem		  = ctx.resource_manager.get_memory();
		skybox_hdr_internals_t* internals = mem.get<skybox_hdr_internals_t>(entry.internals);
		enqueue_destroy_skybox_resources(*internals);
		*internals = {};
	}

	const resource_type_desc_t skybox_hdr_resource_desc = {
		.type				 = resource_type_e::hdr_skybox,
		.runtime_size		 = sizeof(skybox_hdr_runtime_t),
		.runtime_alignment	 = alignof(skybox_hdr_runtime_t),
		.internals_size		 = sizeof(skybox_hdr_internals_t),
		.internals_alignment = alignof(skybox_hdr_internals_t),
		.wire_magic			 = skybox_hdr_loader_t::WIRE_MAGIC,
		.wire_version		 = skybox_hdr_loader_t::WIRE_VERSION,
		.load				 = skybox_hdr_loader_t::load,
		.unload				 = skybox_hdr_loader_t::unload,
	};

#undef SFG_SKYBOX_HDR_TEX_RADIANCE
#undef SFG_SKYBOX_HDR_TEX_IRRADIANCE
#undef SFG_SKYBOX_HDR_TEX_PREFILTER
#undef SFG_SKYBOX_HDR_TEX_BRDF_LUT
}
