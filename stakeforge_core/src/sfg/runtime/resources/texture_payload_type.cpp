/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "texture_payload_type.hpp"

#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	texture_payload_type_reflection_t::texture_payload_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

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
		reflection_registry_t& registry = reflection_registry_t::get();

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
