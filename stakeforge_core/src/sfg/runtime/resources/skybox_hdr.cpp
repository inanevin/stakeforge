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

#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
#define SFG_SKYBOX_HDR_TEX_RADIANCE	  0
#define SFG_SKYBOX_HDR_TEX_IRRADIANCE 1
#define SFG_SKYBOX_HDR_TEX_PREFILTER  2
#define SFG_SKYBOX_HDR_TEX_BRDF_LUT	  3
#define SFG_SKYBOX_HDR_STAGING_BASE	  16

	namespace
	{
		u8 get_subresource_index(u8 face, u8 mip)
		{
			return static_cast<u8>(face * skybox_hdr_loader_t::MAX_MIPS + mip);
		}

		bool load_texture_block(istream_t& stream, skybox_hdr_texture_block_t& block)
		{
			stream >> block.format;
			stream >> block.size;
			stream >> block.face_count;
			stream >> block.mip_count;

			if (block.format == format_e::undefined || block.size.x == 0 || block.size.y == 0)
				return false;
			if (block.face_count == 0 || block.face_count > skybox_hdr_loader_t::MAX_FACES)
				return false;
			if (block.mip_count == 0 || block.mip_count > skybox_hdr_loader_t::MAX_MIPS)
				return false;

			for (u8 face = 0; face < block.face_count; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					texture_buffer_t& buffer = block.buffers[get_subresource_index(face, mip)];
					stream >> buffer.size;
					stream >> buffer.row_pitch;
					stream >> buffer.data_size;
					buffer.bpp = format_get_bpp(block.format);
					if (buffer.size.x == 0 || buffer.size.y == 0 || buffer.row_pitch == 0 || buffer.data_size == 0)
						return false;
					buffer.pixels = stream.get_data_current();
					stream.skip_by(buffer.data_size);
				}
			}

			return true;
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

		u32 make_staging_user_data(u8 texture_index, u8 face)
		{
			return SFG_SKYBOX_HDR_STAGING_BASE + static_cast<u32>(texture_index) * skybox_hdr_loader_t::MAX_FACES + face;
		}

		void assign_staging(skybox_hdr_internals_t& internals, u8 texture_index, u8 face, gfx_resource_handle staging)
		{
			switch (texture_index)
			{
			case SFG_SKYBOX_HDR_TEX_RADIANCE:
				internals.radiance_staging[face] = staging;
				break;
			case SFG_SKYBOX_HDR_TEX_IRRADIANCE:
				internals.irradiance_staging[face] = staging;
				break;
			case SFG_SKYBOX_HDR_TEX_PREFILTER:
				internals.prefilter_staging[face] = staging;
				break;
			case SFG_SKYBOX_HDR_TEX_BRDF_LUT:
				internals.brdf_lut_staging = staging;
				break;
			default:
				SFG_ASSERT(false);
				break;
			}
		}

		void enqueue_block_upload(const skybox_hdr_texture_block_t& block, gfx_texture_handle texture, const gfx_resource_handle* staging)
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

				const texture_upload_desc_t upload = {
					.texture		   = texture,
					.staging		   = staging[face],
					.mips			   = {.data = upload_mips, .size = block.mip_count},
					.target_states	   = resource_state_ps_resource,
					.destination_slice = face,
					.ownership		   = texture_data_ownership_e::c_free,
				};
				render_resources_t::get().enqueue_texture_upload(upload);
			}
		}

		void enqueue_destroy_internals(const skybox_hdr_internals_t& internals)
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

	bool skybox_hdr_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	  mem	  = ctx.resource_manager.get_memory();
		skybox_hdr_runtime_t* runtime = mem.get<skybox_hdr_runtime_t>(entry.runtime);
		*runtime					  = {};

		istream_t stream;
		stream.open(entry.after_header_data.data, entry.after_header_data.size);
		stream >> runtime->radiance_size;
		stream >> runtime->irradiance_size;
		stream >> runtime->prefilter_size;
		stream >> runtime->brdf_lut_size;
		stream >> runtime->intensity;
		stream >> runtime->rotation;
		stream >> runtime->prefilter_mips;

		return load_texture_block(stream, runtime->radiance) && load_texture_block(stream, runtime->irradiance) && load_texture_block(stream, runtime->prefilter) && load_texture_block(stream, runtime->brdf_lut);
	}

	create_internals_result_e skybox_hdr_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&			mem		  = ctx.resource_manager.get_memory();
		const skybox_hdr_runtime_t* runtime	  = mem.get<skybox_hdr_runtime_t>(entry.runtime);
		skybox_hdr_internals_t*		internals = mem.get<skybox_hdr_internals_t>(entry.internals);
		*internals							  = {};

		render_resources_t& render_resources = render_resources_t::get();
		render_resources.enqueue_create_texture(entry.hash, make_texture_desc(runtime->radiance, "skybox_hdr_radiance"), resource_type_e::hdr_skybox, SFG_SKYBOX_HDR_TEX_RADIANCE);
		render_resources.enqueue_create_texture(entry.hash, make_texture_desc(runtime->irradiance, "skybox_hdr_irradiance"), resource_type_e::hdr_skybox, SFG_SKYBOX_HDR_TEX_IRRADIANCE);
		render_resources.enqueue_create_texture(entry.hash, make_texture_desc(runtime->prefilter, "skybox_hdr_prefilter"), resource_type_e::hdr_skybox, SFG_SKYBOX_HDR_TEX_PREFILTER);
		render_resources.enqueue_create_texture(entry.hash, make_texture_desc(runtime->brdf_lut, "skybox_hdr_brdf_lut"), resource_type_e::hdr_skybox, SFG_SKYBOX_HDR_TEX_BRDF_LUT);

		const resource_desc_t radiance_staging_desc	  = make_staging_desc(runtime->radiance);
		const resource_desc_t irradiance_staging_desc = make_staging_desc(runtime->irradiance);
		const resource_desc_t prefilter_staging_desc  = make_staging_desc(runtime->prefilter);
		const resource_desc_t brdf_lut_staging_desc	  = make_staging_desc(runtime->brdf_lut);

		for (u8 face = 0; face < runtime->radiance.face_count; ++face)
			render_resources.enqueue_create_resource(entry.hash, resource_type_e::hdr_skybox, radiance_staging_desc, make_staging_user_data(SFG_SKYBOX_HDR_TEX_RADIANCE, face));
		for (u8 face = 0; face < runtime->irradiance.face_count; ++face)
			render_resources.enqueue_create_resource(entry.hash, resource_type_e::hdr_skybox, irradiance_staging_desc, make_staging_user_data(SFG_SKYBOX_HDR_TEX_IRRADIANCE, face));
		for (u8 face = 0; face < runtime->prefilter.face_count; ++face)
			render_resources.enqueue_create_resource(entry.hash, resource_type_e::hdr_skybox, prefilter_staging_desc, make_staging_user_data(SFG_SKYBOX_HDR_TEX_PREFILTER, face));
		render_resources.enqueue_create_resource(entry.hash, resource_type_e::hdr_skybox, brdf_lut_staging_desc, make_staging_user_data(SFG_SKYBOX_HDR_TEX_BRDF_LUT, 0));

		internals->pending_count = static_cast<u8>(4 + runtime->radiance.face_count + runtime->irradiance.face_count + runtime->prefilter.face_count + runtime->brdf_lut.face_count);
		return create_internals_result_e::queued;
	}

	resource_ready_result_e skybox_hdr_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t&		mem		  = ctx.resource_manager.get_memory();
		skybox_hdr_runtime_t*	runtime	  = mem.get<skybox_hdr_runtime_t>(entry.runtime);
		skybox_hdr_internals_t* internals = mem.get<skybox_hdr_internals_t>(entry.internals);

		SFG_ASSERT(internals->pending_count > 0);

		if (completion.state == resource_state_e::failed)
		{
			internals->had_failure = 1;
		}
		else if (completion.kind == render_resource_kind_e::texture)
		{
			switch (completion.user_data)
			{
			case SFG_SKYBOX_HDR_TEX_RADIANCE:
				internals->radiance_texture = completion.texture;
				internals->radiance_index	= completion.gpu_index;
				break;
			case SFG_SKYBOX_HDR_TEX_IRRADIANCE:
				internals->irradiance_texture = completion.texture;
				internals->irradiance_index	  = completion.gpu_index;
				break;
			case SFG_SKYBOX_HDR_TEX_PREFILTER:
				internals->prefilter_texture = completion.texture;
				internals->prefilter_index	 = completion.gpu_index;
				break;
			case SFG_SKYBOX_HDR_TEX_BRDF_LUT:
				internals->brdf_lut_texture = completion.texture;
				internals->brdf_lut_index	= completion.gpu_index;
				break;
			default:
				SFG_ASSERT(false);
				break;
			}
		}
		else
		{
			const u32 staging_data	= completion.user_data - SFG_SKYBOX_HDR_STAGING_BASE;
			const u8  texture_index = static_cast<u8>(staging_data / skybox_hdr_loader_t::MAX_FACES);
			const u8  face			= static_cast<u8>(staging_data % skybox_hdr_loader_t::MAX_FACES);
			assign_staging(*internals, texture_index, face, completion.resource);
		}

		internals->pending_count--;
		if (internals->pending_count != 0)
			return resource_ready_result_e::pending;

		if (internals->had_failure)
		{
			enqueue_destroy_internals(*internals);
			*internals = {};
			return resource_ready_result_e::failed;
		}

		enqueue_block_upload(runtime->radiance, internals->radiance_texture, internals->radiance_staging);
		enqueue_block_upload(runtime->irradiance, internals->irradiance_texture, internals->irradiance_staging);
		enqueue_block_upload(runtime->prefilter, internals->prefilter_texture, internals->prefilter_staging);
		enqueue_block_upload(runtime->brdf_lut, internals->brdf_lut_texture, &internals->brdf_lut_staging);
		return resource_ready_result_e::ready;
	}

	void skybox_hdr_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		mem		  = ctx.resource_manager.get_memory();
		skybox_hdr_internals_t* internals = mem.get<skybox_hdr_internals_t>(entry.internals);
		enqueue_destroy_internals(*internals);
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
		.create_internals	 = skybox_hdr_loader_t::create_internals,
		.resource_ready		 = skybox_hdr_loader_t::resource_ready,
		.destroy_internals	 = skybox_hdr_loader_t::destroy_internals,
	};

#undef SFG_SKYBOX_HDR_TEX_RADIANCE
#undef SFG_SKYBOX_HDR_TEX_IRRADIANCE
#undef SFG_SKYBOX_HDR_TEX_PREFILTER
#undef SFG_SKYBOX_HDR_TEX_BRDF_LUT
#undef SFG_SKYBOX_HDR_STAGING_BASE
}
