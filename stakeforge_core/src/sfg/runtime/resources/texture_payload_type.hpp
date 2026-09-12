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

#pragma once

#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	enum class texture_payload_type_e : u8
	{
		ktx2_uastc,
		uncompressed,
		png,
	};

	enum class texture_ktx2_compression_e : u8
	{
		fastest,
		faster,
		default_quality,
		high_quality,
	};

	SFG_DEFINE_TYPE_ID(texture_payload_type_e);
	SFG_DEFINE_TYPE_ID(texture_ktx2_compression_e);

	struct texture_payload_type_reflection_t
	{
		texture_payload_type_reflection_t();
	};

	struct texture_ktx2_compression_reflection_t
	{
		texture_ktx2_compression_reflection_t();
	};

	inline texture_payload_type_reflection_t	 g_reflect_texture_payload_type;
	inline texture_ktx2_compression_reflection_t g_reflect_texture_ktx2_compression;
}
