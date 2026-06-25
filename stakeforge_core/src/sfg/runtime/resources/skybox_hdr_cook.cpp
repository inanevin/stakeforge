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

#include "skybox_hdr_cook.hpp"
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <sfg/common/packing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/vendor/stb/stb_image.h>

namespace sfg
{
	namespace
	{
		struct hdr_source_t
		{
			float* data	  = nullptr;
			i32	   width  = 0;
			i32	   height = 0;
		};

		struct sample2_t
		{
			f32 x = 0.0f;
			f32 y = 0.0f;
		};

		bool is_valid_size(vec2u16_t size)
		{
			return size.x != 0 && size.y != 0;
		}

		u8 get_subresource_index(u8 face, u8 mip)
		{
			return static_cast<u8>(face * skybox_hdr_loader_t::MAX_MIPS + mip);
		}

		u16 get_mip_size(u16 base_size, u8 mip)
		{
			const u32 size = static_cast<u32>(base_size) >> mip;
			return static_cast<u16>(size == 0 ? 1 : size);
		}

		u8 get_mip_count(vec2u16_t size, u8 requested)
		{
			u8	count = 1;
			u16 w	  = size.x;
			u16 h	  = size.y;
			while ((w > 1 || h > 1) && count < skybox_hdr_loader_t::MAX_MIPS)
			{
				w = w > 1 ? static_cast<u16>(w >> 1) : 1;
				h = h > 1 ? static_cast<u16>(h >> 1) : 1;
				count++;
			}
			if (requested != 0 && requested < count)
				count = requested;
			return count;
		}

		vec3f_t normalize(const vec3f_t& v)
		{
			const f32 mag = math::sqrt(vec3f_t::dot(v, v));
			if (mag <= MATH_EPS)
				return vec3f_t::zero;
			return v / mag;
		}

		vec3f_t face_direction(u8 face, u16 x, u16 y, vec2u16_t size)
		{
			const f32 u = (2.0f * (static_cast<f32>(x) + 0.5f) / static_cast<f32>(size.x)) - 1.0f;
			const f32 v = (2.0f * (static_cast<f32>(y) + 0.5f) / static_cast<f32>(size.y)) - 1.0f;
			switch (face)
			{
			case 0:
				return normalize({1.0f, -v, -u});
			case 1:
				return normalize({-1.0f, -v, u});
			case 2:
				return normalize({u, 1.0f, v});
			case 3:
				return normalize({u, -1.0f, -v});
			case 4:
				return normalize({u, -v, 1.0f});
			default:
				return normalize({-u, -v, -1.0f});
			}
		}

		vec3f_t get_source_pixel(const hdr_source_t& source, i32 x, i32 y)
		{
			if (x < 0)
				x = (x % source.width) + source.width;
			if (x >= source.width)
				x = x % source.width;
			y				   = static_cast<i32>(math::clamp(static_cast<f32>(y), 0.0f, static_cast<f32>(source.height - 1)));
			const size_t pixel = (static_cast<size_t>(y) * static_cast<size_t>(source.width) + static_cast<size_t>(x)) * 3;
			return {source.data[pixel + 0], source.data[pixel + 1], source.data[pixel + 2]};
		}

		vec3f_t sample_equirect(const hdr_source_t& source, const vec3f_t& dir)
		{
			const f32	  u	  = static_cast<f32>(std::atan2(dir.z, dir.x)) * MATH_R_TWO_PI + 0.5f;
			const f32	  v	  = math::acos(math::clamp(dir.y, -1.0f, 1.0f)) * MATH_R_PI;
			const f32	  x	  = u * static_cast<f32>(source.width) - 0.5f;
			const f32	  y	  = v * static_cast<f32>(source.height) - 0.5f;
			const i32	  x0  = static_cast<i32>(math::floor(x));
			const i32	  y0  = static_cast<i32>(math::floor(y));
			const i32	  x1  = x0 + 1;
			const i32	  y1  = y0 + 1;
			const f32	  tx  = x - static_cast<f32>(x0);
			const f32	  ty  = y - static_cast<f32>(y0);
			const vec3f_t c00 = get_source_pixel(source, x0, y0);
			const vec3f_t c10 = get_source_pixel(source, x1, y0);
			const vec3f_t c01 = get_source_pixel(source, x0, y1);
			const vec3f_t c11 = get_source_pixel(source, x1, y1);
			const vec3f_t cx0 = c00 * (1.0f - tx) + c10 * tx;
			const vec3f_t cx1 = c01 * (1.0f - tx) + c11 * tx;
			return cx0 * (1.0f - ty) + cx1 * ty;
		}

		u32 radical_inverse_vdc(u32 bits)
		{
			bits = (bits << 16u) | (bits >> 16u);
			bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
			bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
			bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
			bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
			return bits;
		}

		sample2_t hammersley(u32 i, u32 count)
		{
			return {static_cast<f32>(i) / static_cast<f32>(count), static_cast<f32>(radical_inverse_vdc(i)) * 2.3283064365386963e-10f};
		}

		void make_basis(const vec3f_t& n, vec3f_t& tangent, vec3f_t& bitangent)
		{
			const vec3f_t up = math::abs(n.y) < 0.999f ? vec3f_t{0.0f, 1.0f, 0.0f} : vec3f_t{1.0f, 0.0f, 0.0f};
			tangent			 = normalize(vec3f_t::cross(up, n));
			bitangent		 = vec3f_t::cross(n, tangent);
		}

		vec3f_t sample_cosine_hemisphere(const vec3f_t& n, sample2_t xi)
		{
			vec3f_t tangent;
			vec3f_t bitangent;
			make_basis(n, tangent, bitangent);
			const f32 phi		= MATH_TWO_PI * xi.x;
			const f32 cos_theta = math::sqrt(1.0f - xi.y);
			const f32 sin_theta = math::sqrt(xi.y);
			return normalize(tangent * (math::cos(phi) * sin_theta) + bitangent * (math::sin(phi) * sin_theta) + n * cos_theta);
		}

		vec3f_t importance_sample_ggx(sample2_t xi, const vec3f_t& n, f32 roughness)
		{
			const f32 a			= roughness * roughness;
			const f32 a2		= a * a;
			const f32 phi		= MATH_TWO_PI * xi.x;
			const f32 denom		= 1.0f + (a2 - 1.0f) * xi.y;
			const f32 cos_theta = math::sqrt((1.0f - xi.y) / denom);
			const f32 sin_theta = math::sqrt(math::max(0.0f, 1.0f - cos_theta * cos_theta));
			vec3f_t	  tangent;
			vec3f_t	  bitangent;
			make_basis(n, tangent, bitangent);
			return normalize(tangent * (math::cos(phi) * sin_theta) + bitangent * (math::sin(phi) * sin_theta) + n * cos_theta);
		}

		vec3f_t integrate_irradiance(const hdr_source_t& source, const vec3f_t& n, u32 sample_count)
		{
			vec3f_t irradiance = vec3f_t::zero;
			for (u32 i = 0; i < sample_count; ++i)
				irradiance += sample_equirect(source, sample_cosine_hemisphere(n, hammersley(i, sample_count)));
			return irradiance * (MATH_PI / static_cast<f32>(sample_count));
		}

		vec3f_t integrate_prefilter(const hdr_source_t& source, const vec3f_t& r, f32 roughness, u32 sample_count)
		{
			if (roughness <= MATH_EPS)
				return sample_equirect(source, r);

			vec3f_t		  color		   = vec3f_t::zero;
			f32			  total_weight = 0.0f;
			const vec3f_t v			   = r;
			for (u32 i = 0; i < sample_count; ++i)
			{
				const vec3f_t h		= importance_sample_ggx(hammersley(i, sample_count), r, roughness);
				const vec3f_t l		= normalize(h * (2.0f * vec3f_t::dot(v, h)) - v);
				const f32	  ndotl = math::max(vec3f_t::dot(r, l), 0.0f);
				if (ndotl > 0.0f)
				{
					color += sample_equirect(source, l) * ndotl;
					total_weight += ndotl;
				}
			}
			if (total_weight <= MATH_EPS)
				return vec3f_t::zero;
			return color / total_weight;
		}

		f32 geometry_schlick_ggx_ibl(f32 ndotv, f32 roughness)
		{
			const f32 a = roughness;
			const f32 k = (a * a) * 0.5f;
			return ndotv / (ndotv * (1.0f - k) + k);
		}

		sample2_t integrate_brdf(f32 ndotv, f32 roughness, u32 sample_count)
		{
			const vec3f_t v = {math::sqrt(math::max(0.0f, 1.0f - ndotv * ndotv)), 0.0f, ndotv};
			const vec3f_t n = {0.0f, 0.0f, 1.0f};
			f32			  a = 0.0f;
			f32			  b = 0.0f;
			for (u32 i = 0; i < sample_count; ++i)
			{
				const vec3f_t h		= importance_sample_ggx(hammersley(i, sample_count), n, roughness);
				const vec3f_t l		= normalize(h * (2.0f * vec3f_t::dot(v, h)) - v);
				const f32	  ndotl = math::max(l.z, 0.0f);
				const f32	  ndoth = math::max(h.z, 0.0f);
				const f32	  vdoth = math::max(vec3f_t::dot(v, h), 0.0f);
				if (ndotl > 0.0f)
				{
					const f32 g	   = geometry_schlick_ggx_ibl(ndotv, roughness) * geometry_schlick_ggx_ibl(ndotl, roughness);
					const f32 gvis = (g * vdoth) / math::max(ndoth * ndotv, MATH_EPS);
					const f32 fc   = math::pow(1.0f - vdoth, 5.0f);
					a += (1.0f - fc) * gvis;
					b += fc * gvis;
				}
			}
			return {a / static_cast<f32>(sample_count), b / static_cast<f32>(sample_count)};
		}

		texture_buffer_t make_buffer(format_e format, vec2u16_t size)
		{
			texture_buffer_t buffer = {};
			buffer.size				= size;
			buffer.bpp				= format_get_bpp(format);
			buffer.row_pitch		= static_cast<u32>(size.x) * buffer.bpp;
			buffer.data_size		= buffer.row_pitch * static_cast<u32>(size.y);
			buffer.pixels			= static_cast<u8*>(SFG_MALLOC(buffer.data_size));
			return buffer;
		}

		void write_rgba16f(texture_buffer_t& buffer, u16 x, u16 y, const vec3f_t& color)
		{
			u16* pixel = reinterpret_cast<u16*>(buffer.pixels + static_cast<size_t>(y) * buffer.row_pitch + static_cast<size_t>(x) * buffer.bpp);
			pixel[0]   = packing_t::float_to_half(math::max(color.x, 0.0f));
			pixel[1]   = packing_t::float_to_half(math::max(color.y, 0.0f));
			pixel[2]   = packing_t::float_to_half(math::max(color.z, 0.0f));
			pixel[3]   = packing_t::float_to_half(1.0f);
		}

		void write_rg16f(texture_buffer_t& buffer, u16 x, u16 y, sample2_t value)
		{
			u16* pixel = reinterpret_cast<u16*>(buffer.pixels + static_cast<size_t>(y) * buffer.row_pitch + static_cast<size_t>(x) * buffer.bpp);
			pixel[0]   = packing_t::float_to_half(value.x);
			pixel[1]   = packing_t::float_to_half(value.y);
		}

		skybox_hdr_texture_block_t build_radiance(const hdr_source_t& source, vec2u16_t size)
		{
			skybox_hdr_texture_block_t block = {};
			block.format					 = format_e::r16g16b16a16_sfloat;
			block.size						 = size;
			block.face_count				 = skybox_hdr_loader_t::MAX_FACES;
			block.mip_count					 = 1;
			for (u8 face = 0; face < block.face_count; ++face)
			{
				texture_buffer_t& buffer = block.buffers[get_subresource_index(face, 0)];
				buffer					 = make_buffer(block.format, size);
				for (u16 y = 0; y < size.y; ++y)
				{
					for (u16 x = 0; x < size.x; ++x)
						write_rgba16f(buffer, x, y, sample_equirect(source, face_direction(face, x, y, size)));
				}
			}
			return block;
		}

		skybox_hdr_texture_block_t build_irradiance(const hdr_source_t& source, vec2u16_t size, u32 sample_count)
		{
			skybox_hdr_texture_block_t block = {};
			block.format					 = format_e::r16g16b16a16_sfloat;
			block.size						 = size;
			block.face_count				 = skybox_hdr_loader_t::MAX_FACES;
			block.mip_count					 = 1;
			for (u8 face = 0; face < block.face_count; ++face)
			{
				texture_buffer_t& buffer = block.buffers[get_subresource_index(face, 0)];
				buffer					 = make_buffer(block.format, size);
				for (u16 y = 0; y < size.y; ++y)
				{
					for (u16 x = 0; x < size.x; ++x)
						write_rgba16f(buffer, x, y, integrate_irradiance(source, face_direction(face, x, y, size), sample_count));
				}
			}
			return block;
		}

		skybox_hdr_texture_block_t build_prefilter(const hdr_source_t& source, vec2u16_t size, u8 mip_count, u32 sample_count)
		{
			skybox_hdr_texture_block_t block = {};
			block.format					 = format_e::r16g16b16a16_sfloat;
			block.size						 = size;
			block.face_count				 = skybox_hdr_loader_t::MAX_FACES;
			block.mip_count					 = get_mip_count(size, mip_count);
			for (u8 face = 0; face < block.face_count; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					const vec2u16_t	  mip_size	= {get_mip_size(size.x, mip), get_mip_size(size.y, mip)};
					const f32		  roughness = block.mip_count == 1 ? 0.0f : static_cast<f32>(mip) / static_cast<f32>(block.mip_count - 1);
					texture_buffer_t& buffer	= block.buffers[get_subresource_index(face, mip)];
					buffer						= make_buffer(block.format, mip_size);
					for (u16 y = 0; y < mip_size.y; ++y)
					{
						for (u16 x = 0; x < mip_size.x; ++x)
							write_rgba16f(buffer, x, y, integrate_prefilter(source, face_direction(face, x, y, mip_size), roughness, sample_count));
					}
				}
			}
			return block;
		}

		skybox_hdr_texture_block_t build_brdf_lut(vec2u16_t size, u32 sample_count)
		{
			skybox_hdr_texture_block_t block = {};
			block.format					 = format_e::r16g16_sfloat;
			block.size						 = size;
			block.face_count				 = 1;
			block.mip_count					 = 1;
			texture_buffer_t& buffer		 = block.buffers[0];
			buffer							 = make_buffer(block.format, size);
			for (u16 y = 0; y < size.y; ++y)
			{
				const f32 roughness = (static_cast<f32>(y) + 0.5f) / static_cast<f32>(size.y);
				for (u16 x = 0; x < size.x; ++x)
				{
					const f32 ndotv = (static_cast<f32>(x) + 0.5f) / static_cast<f32>(size.x);
					write_rg16f(buffer, x, y, integrate_brdf(ndotv, roughness, sample_count));
				}
			}
			return block;
		}

		void write_texture_block(const skybox_hdr_texture_block_t& block, ostream_t& stream)
		{
			stream << block.format;
			stream << block.size;
			stream << block.face_count;
			stream << block.mip_count;
			for (u8 face = 0; face < block.face_count; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					const texture_buffer_t& buffer = block.buffers[get_subresource_index(face, mip)];
					stream << buffer.size;
					stream << buffer.row_pitch;
					stream << buffer.data_size;
					stream.write_raw(buffer.pixels, buffer.data_size);
				}
			}
		}

		void free_texture_block(skybox_hdr_texture_block_t& block)
		{
			for (u8 face = 0; face < block.face_count; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					texture_buffer_t& buffer = block.buffers[get_subresource_index(face, mip)];
					SFG_FREE(buffer.pixels);
					buffer.pixels = nullptr;
				}
			}
		}
	}

	bool skybox_hdr_cooker::cook_from_file(const skybox_hdr_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		if (!is_valid_size(cfg.radiance_size) || !is_valid_size(cfg.irradiance_size) || !is_valid_size(cfg.prefilter_size) || !is_valid_size(cfg.brdf_lut_size) || cfg.irradiance_sample_count == 0 || cfg.prefilter_sample_count == 0 ||
			cfg.brdf_lut_sample_count == 0)
		{
			SFG_ERR("invalid HDR skybox cook config");
			return false;
		}

		int	   width	= 0;
		int	   height	= 0;
		int	   channels = 0;
		float* pixels	= stbi_loadf(full_path, &width, &height, &channels, 3);
		if (pixels == nullptr || width <= 0 || height <= 0 || width > UINT16_MAX || height > UINT16_MAX)
		{
			SFG_ERR("failed to load HDR skybox source {0}", full_path);
			if (pixels != nullptr)
				stbi_image_free(pixels);
			return false;
		}

		const hdr_source_t source = {.data = pixels, .width = width, .height = height};

		skybox_hdr_texture_block_t radiance	  = build_radiance(source, cfg.radiance_size);
		skybox_hdr_texture_block_t irradiance = build_irradiance(source, cfg.irradiance_size, cfg.irradiance_sample_count);
		skybox_hdr_texture_block_t prefilter  = build_prefilter(source, cfg.prefilter_size, cfg.prefilter_mips, cfg.prefilter_sample_count);
		skybox_hdr_texture_block_t brdf_lut	  = build_brdf_lut(cfg.brdf_lut_size, cfg.brdf_lut_sample_count);

		out_header = {
			.magic		 = skybox_hdr_loader_t::WIRE_MAGIC,
			.version	 = skybox_hdr_loader_t::WIRE_VERSION,
			.source_tick = file_system_t::get_last_modified_ticks(full_path),
		};

		ostream_t payload;
		payload << cfg.radiance_size;
		payload << cfg.irradiance_size;
		payload << cfg.prefilter_size;
		payload << cfg.brdf_lut_size;
		payload << cfg.intensity;
		payload << cfg.rotation;
		payload << prefilter.mip_count;
		write_texture_block(radiance, payload);
		write_texture_block(irradiance, payload);
		write_texture_block(prefilter, payload);
		write_texture_block(brdf_lut, payload);

		stream			  = compressor_t::compress(payload);
		const bool result = stream.get_size() != 0;

		free_texture_block(radiance);
		free_texture_block(irradiance);
		free_texture_block(prefilter);
		free_texture_block(brdf_lut);
		stbi_image_free(pixels);
		return result;
	}
}

namespace sfg
{
	skybox_hdr_cook_config_reflection_t::skybox_hdr_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<skybox_hdr_cook_config_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "radiance_size", .display_name = "Radiance Size", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(skybox_hdr_cook_config_t, radiance_size), .size = sizeof(vec2u16_t)},
			{.name = "irradiance_size", .display_name = "Irradiance Size", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(skybox_hdr_cook_config_t, irradiance_size), .size = sizeof(vec2u16_t)},
			{.name = "prefilter_size", .display_name = "Prefilter Size", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(skybox_hdr_cook_config_t, prefilter_size), .size = sizeof(vec2u16_t)},
			{.name = "brdf_lut_size", .display_name = "BRDF LUT Size", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(skybox_hdr_cook_config_t, brdf_lut_size), .size = sizeof(vec2u16_t)},
			{.name = "irradiance_sample_count", .display_name = "Irradiance Samples", .type = reflected_value_type_e::u32, .offset = offsetof(skybox_hdr_cook_config_t, irradiance_sample_count), .size = sizeof(u32)},
			{.name = "prefilter_sample_count", .display_name = "Prefilter Samples", .type = reflected_value_type_e::u32, .offset = offsetof(skybox_hdr_cook_config_t, prefilter_sample_count), .size = sizeof(u32)},
			{.name = "brdf_lut_sample_count", .display_name = "BRDF LUT Samples", .type = reflected_value_type_e::u32, .offset = offsetof(skybox_hdr_cook_config_t, brdf_lut_sample_count), .size = sizeof(u32)},
			{.name = "intensity", .display_name = "Intensity", .type = reflected_value_type_e::f32, .offset = offsetof(skybox_hdr_cook_config_t, intensity), .size = sizeof(f32)},
			{.name = "rotation", .display_name = "Rotation", .type = reflected_value_type_e::f32, .offset = offsetof(skybox_hdr_cook_config_t, rotation), .size = sizeof(f32)},
			{.name = "prefilter_mips", .display_name = "Prefilter Mips", .type = reflected_value_type_e::u8, .offset = offsetof(skybox_hdr_cook_config_t, prefilter_mips), .size = sizeof(u8)},
		};

		registry.register_type({
			.fields		  = {.data = fields, .size = std::size(fields)},
			.name		  = "skybox_hdr_cook_config_t",
			.display_name = "HDR Skybox Cook Config",
			.type_id	  = type_id_t<skybox_hdr_cook_config_t>::value,
			.size		  = sizeof(skybox_hdr_cook_config_t),
			.alignment	  = alignof(skybox_hdr_cook_config_t),
		});
	}
}
