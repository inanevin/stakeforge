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

#include "descriptions_reflection.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t sampler_filter_values[] = {
			{.name = "anisotropic", .display_name = "Anisotropic", .value = static_cast<i64>(sampler_filter_e::anisotropic)},
			{.name = "nearest", .display_name = "Nearest", .value = static_cast<i64>(sampler_filter_e::nearest)},
			{.name = "linear", .display_name = "Linear", .value = static_cast<i64>(sampler_filter_e::linear)},
		};

		static const reflected_enum_value_desc_t sampler_border_values[] = {
			{.name = "transparent", .display_name = "Transparent", .value = static_cast<i64>(sampler_border_e::transparent)},
			{.name = "white", .display_name = "White", .value = static_cast<i64>(sampler_border_e::white)},
		};

		static const reflected_enum_value_desc_t address_mode_values[] = {
			{.name = "repeat", .display_name = "Repeat", .value = static_cast<i64>(address_mode::repeat)},
			{.name = "border", .display_name = "Border", .value = static_cast<i64>(address_mode::border)},
			{.name = "clamp", .display_name = "Clamp", .value = static_cast<i64>(address_mode::clamp)},
			{.name = "mirrored_repeat", .display_name = "Mirrored Repeat", .value = static_cast<i64>(address_mode::mirrored_repeat)},
			{.name = "mirrored_clamp", .display_name = "Mirrored Clamp", .value = static_cast<i64>(address_mode::mirrored_clamp)},
		};

		static const reflected_enum_value_desc_t compare_values[] = {
			{.name = "never", .display_name = "Never", .value = static_cast<i64>(compare_op::never)},
			{.name = "less", .display_name = "Less", .value = static_cast<i64>(compare_op::less)},
			{.name = "equal", .display_name = "Equal", .value = static_cast<i64>(compare_op::equal)},
			{.name = "lequal", .display_name = "Less Equal", .value = static_cast<i64>(compare_op::lequal)},
			{.name = "greater", .display_name = "Greater", .value = static_cast<i64>(compare_op::greater)},
			{.name = "nequal", .display_name = "Not Equal", .value = static_cast<i64>(compare_op::nequal)},
			{.name = "gequal", .display_name = "Greater Equal", .value = static_cast<i64>(compare_op::gequal)},
			{.name = "always", .display_name = "Always", .value = static_cast<i64>(compare_op::always)},
		};
	}

	sampler_filter_reflection_t::sampler_filter_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = sampler_filter_values, .size = std::size(sampler_filter_values)},
			.name		 = "sampler_filter_e",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(sampler_filter_e),
			.alignment	 = alignof(sampler_filter_e),
		});
	}

	sampler_border_reflection_t::sampler_border_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = sampler_border_values, .size = std::size(sampler_border_values)},
			.name		 = "sampler_border_e",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(sampler_border_e),
			.alignment	 = alignof(sampler_border_e),
		});
	}

	address_mode_reflection_t::address_mode_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = address_mode_values, .size = std::size(address_mode_values)},
			.name		 = "address_mode",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(address_mode),
			.alignment	 = alignof(address_mode),
		});
	}

	sampler_desc_reflection_t::sampler_desc_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "debug_name", .display_name = "Debug Name", .type = reflected_value_type_e::string, .offset = offsetof(sampler_desc_t, debug_name), .size = sizeof(sampler_desc_t::debug_name)},
			{.name = "anisotropy", .display_name = "Anisotropy", .type = reflected_value_type_e::u32, .offset = offsetof(sampler_desc_t, anisotropy), .size = sizeof(u32)},
			{.name = "min_lod", .display_name = "Min LOD", .type = reflected_value_type_e::f32, .offset = offsetof(sampler_desc_t, min_lod), .size = sizeof(f32)},
			{.name = "max_lod", .display_name = "Max LOD", .type = reflected_value_type_e::f32, .offset = offsetof(sampler_desc_t, max_lod), .size = sizeof(f32)},
			{.name = "lod_bias", .display_name = "LOD Bias", .type = reflected_value_type_e::f32, .offset = offsetof(sampler_desc_t, lod_bias), .size = sizeof(f32)},
			{.enum_values	= {.data = address_mode_values, .size = std::size(address_mode_values)},
			 .name			= "address_u",
			 .display_name	= "Address U",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = address_mode_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, address_u),
			 .size			= sizeof(address_mode)},
			{.enum_values	= {.data = address_mode_values, .size = std::size(address_mode_values)},
			 .name			= "address_v",
			 .display_name	= "Address V",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = address_mode_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, address_v),
			 .size			= sizeof(address_mode)},
			{.enum_values	= {.data = address_mode_values, .size = std::size(address_mode_values)},
			 .name			= "address_w",
			 .display_name	= "Address W",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = address_mode_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, address_w),
			 .size			= sizeof(address_mode)},
			{.enum_values	= {.data = sampler_filter_values, .size = std::size(sampler_filter_values)},
			 .name			= "min_filter",
			 .display_name	= "Min Filter",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = sampler_filter_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, min_filter),
			 .size			= sizeof(sampler_filter_e)},
			{.enum_values	= {.data = sampler_filter_values, .size = std::size(sampler_filter_values)},
			 .name			= "mag_filter",
			 .display_name	= "Mag Filter",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = sampler_filter_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, mag_filter),
			 .size			= sizeof(sampler_filter_e)},
			{.enum_values	= {.data = sampler_filter_values, .size = std::size(sampler_filter_values)},
			 .name			= "mip_filter",
			 .display_name	= "Mip Filter",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = sampler_filter_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, mip_filter),
			 .size			= sizeof(sampler_filter_e)},
			{.enum_values	= {.data = sampler_border_values, .size = std::size(sampler_border_values)},
			 .name			= "border",
			 .display_name	= "Border",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = sampler_border_reflection_t::TYPE_ID,
			 .offset		= offsetof(sampler_desc_t, border),
			 .size			= sizeof(sampler_border_e)},
			{.enum_values	= {.data = compare_values, .size = std::size(compare_values)},
			 .name			= "compare",
			 .display_name	= "Compare",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = "compare_op"_hs,
			 .offset		= offsetof(sampler_desc_t, compare),
			 .size			= sizeof(compare_op)},
			{.name = "use_compare", .display_name = "Use Compare", .type = reflected_value_type_e::bool8, .offset = offsetof(sampler_desc_t, use_compare), .size = sizeof(bool)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "sampler_desc_t",
			.type_id   = TYPE_ID,
			.size	   = sizeof(sampler_desc_t),
			.alignment = alignof(sampler_desc_t),
		});
	}

}
