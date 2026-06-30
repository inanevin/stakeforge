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

#include <sfg/reflection/reflection_registry_v2.hpp>

namespace sfg
{
	texture_payload_type_reflection_t::texture_payload_type_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "texture_payload_type_e",
			.fields =
				{
					{.name = "ktx2_uastc", .display_name = "KTX2 UASTC"},
					{.name = "uncompressed", .display_name = "Uncompressed"},
					{.name = "png", .display_name = "PNG"},
				},
			.type_id   = type_id_t<texture_payload_type_e>::value,
			.size	   = sizeof(texture_payload_type_e),
			.alignment = alignof(texture_payload_type_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

	texture_ktx2_compression_reflection_t::texture_ktx2_compression_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "texture_ktx2_compression_e",
			.fields =
				{
					{.name = "fastest", .display_name = "Fastest"},
					{.name = "faster", .display_name = "Faster"},
					{.name = "default_quality", .display_name = "Default Quality"},
					{.name = "high_quality", .display_name = "High Quality"},
				},
			.type_id   = type_id_t<texture_ktx2_compression_e>::value,
			.size	   = sizeof(texture_ktx2_compression_e),
			.alignment = alignof(texture_ktx2_compression_e),
			.flags	   = reflected_type_flag_enum,
		});
	}
}
