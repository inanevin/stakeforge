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

#include "descriptions.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/math/math.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	namespace
	{
		void set_desc_name(char* dst, size_t capacity, const char* src)
		{
			SFG_ASSERT(src != nullptr);
			if (src == nullptr)
				return;
			const size_t len = std::strlen(src);
			SFG_ASSERT(len < capacity);
			if (len >= capacity)
				return;
			SFG_MEMCPY(dst, src, len + 1);
		}
	}

	void resource_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	void texture_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	bool sampler_desc_t::operator==(const sampler_desc_t& other) const
	{
		return other.anisotropy == anisotropy && other.address_u == address_u && other.address_v == address_v && other.address_w == address_w && other.min_filter == min_filter && other.mag_filter == mag_filter && other.mip_filter == mip_filter &&
			   other.border == border && other.compare == compare && other.use_compare == use_compare && math::almost_equal(min_lod, other.min_lod) && math::almost_equal(max_lod, other.max_lod) && math::almost_equal(lod_bias, other.lod_bias);
	}

	void sampler_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	void sampler_desc_t::serialize(ostream_t& stream) const
	{
		stream << anisotropy;
		stream << lod_bias;
		stream << min_lod;
		stream << max_lod;
		stream << address_u;
		stream << address_v;
		stream << address_w;
		stream << min_filter;
		stream << mag_filter;
		stream << mip_filter;
		stream << border;
		stream << compare;
		stream << use_compare;
	}

	void sampler_desc_t::deserialize(istream_t& stream)
	{
		u8 addr_u	  = 0;
		u8 addr_v	  = 0;
		u8 addr_w	  = 0;
		u8 min		  = 0;
		u8 mag		  = 0;
		u8 mip		  = 0;
		u8 border_val = 0;
		u8 compare_op = 0;
		stream >> anisotropy;
		stream >> lod_bias;
		stream >> min_lod;
		stream >> max_lod;
		stream >> addr_u;
		stream >> addr_v;
		stream >> addr_w;
		stream >> min;
		stream >> mag;
		stream >> mip;
		stream >> border_val;
		stream >> compare_op;
		stream >> use_compare;
		address_u  = static_cast<address_mode>(addr_u);
		address_v  = static_cast<address_mode>(addr_v);
		address_w  = static_cast<address_mode>(addr_w);
		min_filter = static_cast<sampler_filter_e>(min);
		mag_filter = static_cast<sampler_filter_e>(mag);
		mip_filter = static_cast<sampler_filter_e>(mip);
		border	   = static_cast<sampler_border_e>(border_val);
		compare	   = static_cast<sfg::compare_op>(compare_op);
	}

	sampler_filter_reflection_t::sampler_filter_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "sampler_filter_e",
			.display_name = "Sampler Filter",
			.fields =
				{
					{.name = "anisotropic", .display_name = "Anisotropic"},
					{.name = "nearest", .display_name = "Nearest"},
					{.name = "linear", .display_name = "Linear"},
				},
			.type_id   = type_id_t<sampler_filter_e>::value,
			.size	   = sizeof(sampler_filter_e),
			.alignment = alignof(sampler_filter_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

	sampler_border_reflection_t::sampler_border_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "sampler_border_e",
			.display_name = "Sampler Border",
			.fields =
				{
					{.name = "transparent", .display_name = "Transparent"},
					{.name = "white", .display_name = "White"},
				},
			.type_id   = type_id_t<sampler_border_e>::value,
			.size	   = sizeof(sampler_border_e),
			.alignment = alignof(sampler_border_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

	address_mode_reflection_t::address_mode_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "address_mode",
			.display_name = "Address Mode",
			.fields =
				{
					{.name = "repeat", .display_name = "Repeat"},
					{.name = "border", .display_name = "Border"},
					{.name = "clamp", .display_name = "Clamp"},
					{.name = "mirrored_repeat", .display_name = "Mirrored Repeat"},
					{.name = "mirrored_clamp", .display_name = "Mirrored Clamp"},
				},
			.type_id   = type_id_t<address_mode>::value,
			.size	   = sizeof(address_mode),
			.alignment = alignof(address_mode),
			.flags	   = reflected_type_flag_enum,
		});
	}

	sampler_desc_reflection_t::sampler_desc_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "sampler_desc_t",
			.display_name = "Sampler",
			.fields =
				{
					{
						.name		  = "debug_name",
						.display_name = "Debug Name",
						.offset		  = offsetof(sampler_desc_t, debug_name),
						.size		  = sizeof(sampler_desc_t::debug_name),
						.flags		  = reflected_field_flags_e::reflected_field_flag_no_ui,
						.type		  = reflected_value_type_e::char_array,
					},
					{
						.name			   = "anisotropy",
						.display_name	   = "Anisotropy",
						.offset			   = offsetof(sampler_desc_t, anisotropy),
						.size			   = sizeof(u32),
						.flags			   = reflected_field_flags_e::reflected_field_flag_clamped,
						.min_clamp		   = 0,
						.max_clamp		   = 8,
						.clamp_granularity = 1,
						.type			   = reflected_value_type_e::u32,
					},
					{
						.name			   = "min_lod",
						.display_name	   = "Min LOD",
						.offset			   = offsetof(sampler_desc_t, min_lod),
						.size			   = sizeof(f32),
						.flags			   = reflected_field_flags_e::reflected_field_flag_clamped,
						.min_clamp		   = 0,
						.max_clamp		   = 16,
						.clamp_granularity = 1,
						.type			   = reflected_value_type_e::f32,
					},
					{
						.name			   = "max_lod",
						.display_name	   = "Max LOD",
						.offset			   = offsetof(sampler_desc_t, max_lod),
						.size			   = sizeof(f32),
						.flags			   = reflected_field_flags_e::reflected_field_flag_clamped,
						.min_clamp		   = 0,
						.max_clamp		   = 16,
						.clamp_granularity = 1,
						.type			   = reflected_value_type_e::f32,
					},
					{
						.name			   = "lod_bias",
						.display_name	   = "LOD Bias",
						.offset			   = offsetof(sampler_desc_t, lod_bias),
						.size			   = sizeof(f32),
						.flags			   = reflected_field_flags_e::reflected_field_flag_clamped,
						.min_clamp		   = 0,
						.max_clamp		   = 16,
						.clamp_granularity = 1,
						.type			   = reflected_value_type_e::f32,
					},
					{
						.name		  = "address_u",
						.display_name = "Address U",
						.sub_type_id  = type_id_t<address_mode>::value,
						.offset		  = offsetof(sampler_desc_t, address_u),
						.size		  = sizeof(address_mode),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "address_v",
						.display_name = "Address V",
						.sub_type_id  = type_id_t<address_mode>::value,
						.offset		  = offsetof(sampler_desc_t, address_v),
						.size		  = sizeof(address_mode),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "address_w",
						.display_name = "Address W",
						.sub_type_id  = type_id_t<address_mode>::value,
						.offset		  = offsetof(sampler_desc_t, address_w),
						.size		  = sizeof(address_mode),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "min_filter",
						.display_name = "Min Filter",
						.sub_type_id  = type_id_t<sampler_filter_e>::value,
						.offset		  = offsetof(sampler_desc_t, min_filter),
						.size		  = sizeof(sampler_filter_e),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "mag_filter",
						.display_name = "Mag Filter",
						.sub_type_id  = type_id_t<sampler_filter_e>::value,
						.offset		  = offsetof(sampler_desc_t, mag_filter),
						.size		  = sizeof(sampler_filter_e),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "mip_filter",
						.display_name = "Mip Filter",
						.sub_type_id  = type_id_t<sampler_filter_e>::value,
						.offset		  = offsetof(sampler_desc_t, mip_filter),
						.size		  = sizeof(sampler_filter_e),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "border",
						.display_name = "Border",
						.sub_type_id  = type_id_t<sampler_border_e>::value,
						.offset		  = offsetof(sampler_desc_t, border),
						.size		  = sizeof(sampler_border_e),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "compare",
						.display_name = "Compare",
						.sub_type_id  = type_id_t<compare_op>::value,
						.offset		  = offsetof(sampler_desc_t, compare),
						.size		  = sizeof(compare_op),
						.type		  = reflected_value_type_e::u8,
					},
					{
						.name		  = "use_compare",
						.display_name = "Use Compare",
						.offset		  = offsetof(sampler_desc_t, use_compare),
						.size		  = sizeof(bool),
						.type		  = reflected_value_type_e::boolean,
					},
				},
			.type_id   = type_id_t<sampler_desc_t>::value,
			.size	   = sizeof(sampler_desc_t),
			.alignment = alignof(sampler_desc_t),
		});
	}
}
