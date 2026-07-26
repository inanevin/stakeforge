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

#include "render_resources_util.hpp"
#include "render_resources.hpp"
#include <sfg/common/packing.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
#define RENDER_RESOURCES_BRDF_LUT_SIZE		   128
#define RENDER_RESOURCES_BRDF_LUT_SAMPLE_COUNT 256

	render_resource_handle_t render_resources_util_t::create_uploaded_texture(render_resources_t& resources, format_e format, vec2u16_t size, const u8* pixels, const char* name, render_resource_handle_t& out_staging)
	{
		const u32 row_pitch = format_get_row_pitch(format, size.x);
		const u32 row_count = format_get_row_count(format, size.y);
		const u32 data_size = row_pitch * row_count;
		u8*		  data		= static_cast<u8*>(SFG_MALLOC(data_size));

		SFG_MEMCPY(data, pixels, data_size);

		texture_desc_t texture_desc = {};
		texture_desc.texture_format = format;
		texture_desc.size			= size;
		texture_desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		texture_desc.mip_levels		= 1;
		texture_desc.array_length	= 1;
		texture_desc.samples		= 1;
		texture_desc.set_name(name);

		resource_desc_t staging_desc = {};
		staging_desc.size			 = gfx_backend::align_texture_size(gfx_backend::align_texture_size_pitch(row_pitch) * row_count);
		staging_desc.flags			 = resource_flags::rf_cpu_visible;
		staging_desc.set_name("default_texture_upload_staging");

		const texture_buffer_t mip = {
			.pixels	   = data,
			.data_size = data_size,
			.row_pitch = row_pitch,
			.size	   = size,
			.bpp	   = format_get_bpp(format),
		};

		const render_resource_handle_t texture = resources.enqueue_create_texture(texture_desc);
		out_staging							   = resources.enqueue_create_resource(staging_desc);
		resources.enqueue_texture_upload({
			.mips			   = {.data = &mip, .size = 1},
			.texture		   = texture,
			.staging		   = out_staging,
			.target_states	   = resource_state_ps_resource,
			.destination_slice = 0,
			.ownership		   = texture_data_ownership_e::c_free,
		});

		return texture;
	}

	render_resource_handle_t render_resources_util_t::create_brdf_lut(render_resources_t& resources, render_resource_handle_t& out_staging)
	{
		const vec2u16_t size		= {RENDER_RESOURCES_BRDF_LUT_SIZE, RENDER_RESOURCES_BRDF_LUT_SIZE};
		const u32		pixel_count = static_cast<u32>(size.x) * size.y;
		u16*			pixels		= static_cast<u16*>(SFG_MALLOC(pixel_count * sizeof(u16) * 2));

		for (u32 y = 0; y < size.y; ++y)
		{
			const f32 roughness = (static_cast<f32>(y) + 0.5f) / size.y;
			const f32 alpha		= roughness * roughness;
			const f32 alpha_sq	= alpha * alpha;

			for (u32 x = 0; x < size.x; ++x)
			{
				const f32 ndotv = (static_cast<f32>(x) + 0.5f) / size.x;
				const f32 vx	= std::sqrt(math::max(1.0f - ndotv * ndotv, 0.0f));
				f32		  scale = 0.0f;
				f32		  bias	= 0.0f;

				for (u32 sample_index = 0; sample_index < RENDER_RESOURCES_BRDF_LUT_SAMPLE_COUNT; ++sample_index)
				{
					u32 bits = sample_index;
					bits	 = (bits << 16) | (bits >> 16);
					bits	 = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
					bits	 = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
					bits	 = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
					bits	 = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);

					const f32 xi_x		= static_cast<f32>(sample_index) / RENDER_RESOURCES_BRDF_LUT_SAMPLE_COUNT;
					const f32 xi_y		= static_cast<f32>(bits) * 2.3283064365386963e-10f;
					const f32 phi		= MATH_TWO_PI * xi_x;
					const f32 cos_theta = std::sqrt((1.0f - xi_y) / (1.0f + (alpha_sq - 1.0f) * xi_y));
					const f32 sin_theta = std::sqrt(math::max(1.0f - cos_theta * cos_theta, 0.0f));
					const f32 hx		= std::cos(phi) * sin_theta;
					const f32 hz		= cos_theta;
					const f32 vdot_h	= math::max(vx * hx + ndotv * hz, 0.0f);
					const f32 lz		= 2.0f * vdot_h * hz - ndotv;
					const f32 ndotl		= math::max(lz, 0.0f);
					const f32 ndoth		= math::max(hz, 0.0f);

					if (ndotl <= 0.0f)
						continue;

					const f32 k			 = alpha * 0.5f;
					const f32 geometry_v = ndotv / (ndotv * (1.0f - k) + k);
					const f32 geometry_l = ndotl / (ndotl * (1.0f - k) + k);
					const f32 geometry	 = geometry_v * geometry_l;
					const f32 visibility = geometry * vdot_h / math::max(ndoth * ndotv, 0.0001f);
					const f32 fresnel	 = std::pow(1.0f - vdot_h, 5.0f);

					scale += (1.0f - fresnel) * visibility;
					bias += fresnel * visibility;
				}

				const u32 pixel_index	= (y * size.x + x) * 2;
				pixels[pixel_index]		= packing_t::float_to_half(scale / RENDER_RESOURCES_BRDF_LUT_SAMPLE_COUNT);
				pixels[pixel_index + 1] = packing_t::float_to_half(bias / RENDER_RESOURCES_BRDF_LUT_SAMPLE_COUNT);
			}
		}

		const render_resource_handle_t texture = create_uploaded_texture(resources, format_e::r16g16_sfloat, size, reinterpret_cast<const u8*>(pixels), "brdf_lut", out_staging);

		SFG_FREE(pixels);

		return texture;
	}
}
