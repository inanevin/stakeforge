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

#include "texture_payload_type.hpp"
#include <cstddef>

#include <sfg/reflection/reflection_registry.hpp>

#include <iterator>

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t texture_payload_type_values[] = {
			{
				.name		  = "uncompressed",
				.display_name = "Uncompressed",
				.value		  = static_cast<i64>(texture_payload_type_e::uncompressed),
			},
			{
				.name		  = "png",
				.display_name = "PNG",
				.value		  = static_cast<i64>(texture_payload_type_e::png),
			},
			{
				.name		  = "ktx2_uastc",
				.display_name = "KTX2 UASTC",
				.value		  = static_cast<i64>(texture_payload_type_e::ktx2_uastc),
			},
		};

		static const reflected_enum_value_desc_t texture_ktx2_compression_values[] = {
			{
				.name		  = "fastest",
				.display_name = "Fastest",
				.value		  = static_cast<i64>(texture_ktx2_compression_e::fastest),
			},
			{
				.name		  = "faster",
				.display_name = "Faster",
				.value		  = static_cast<i64>(texture_ktx2_compression_e::faster),
			},
			{
				.name		  = "default_quality",
				.display_name = "Default Quality",
				.value		  = static_cast<i64>(texture_ktx2_compression_e::default_quality),
			},
			{
				.name		  = "high_quality",
				.display_name = "High Quality",
				.value		  = static_cast<i64>(texture_ktx2_compression_e::high_quality),
			},
		};
	}

	texture_payload_type_reflection_t::texture_payload_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<texture_payload_type_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = texture_payload_type_values, .size = std::size(texture_payload_type_values)},
			.name		 = "texture_payload_type_e",
			.type_id	 = type_id_t<texture_payload_type_e>::value,
			.size		 = sizeof(texture_payload_type_e),
			.alignment	 = alignof(texture_payload_type_e),
		});
	}

	texture_ktx2_compression_reflection_t::texture_ktx2_compression_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<texture_ktx2_compression_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = texture_ktx2_compression_values, .size = std::size(texture_ktx2_compression_values)},
			.name		 = "texture_ktx2_compression_e",
			.type_id	 = type_id_t<texture_ktx2_compression_e>::value,
			.size		 = sizeof(texture_ktx2_compression_e),
			.alignment	 = alignof(texture_ktx2_compression_e),
		});
	}
}
