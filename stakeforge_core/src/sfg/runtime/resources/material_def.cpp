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

#include "material_def.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>

namespace sfg
{
	void material_parameter_t::serialize(ostream_t& stream) const
	{
		stream << type;
		for (u8 i = 0; i < 4; ++i)
			stream << values[i];
	}

	void material_parameter_t::deserialize(istream_t& stream)
	{
		stream >> type;
		for (u8 i = 0; i < 4; ++i)
			stream >> values[i];
	}

	void material_def_t::serialize(ostream_t& stream) const
	{
		SFG_ASSERT(textures.empty() || sampler != NULL_SID);
		stream << static_cast<u32>(pass_flags);
		stream << shader;
		stream << sampler;

		stream << static_cast<u32>(textures.size());
		for (const sid_t texture : textures)
			stream << texture;

		stream << static_cast<u32>(parameters.size());
		for (const material_parameter_t& parameter : parameters)
			parameter.serialize(stream);

		stream << double_sided;
		stream << use_alpha_cutoff;
	}

	void material_def_t::deserialize(istream_t& stream)
	{
		u32 flags = 0;
		stream >> flags;
		pass_flags = static_cast<world_pass_flags_e>(flags);
		stream >> shader;
		stream >> sampler;

		u32 texture_count = 0;
		stream >> texture_count;
		textures.resize(texture_count);
		for (sid_t& texture : textures)
			stream >> texture;

		u32 parameter_count = 0;
		stream >> parameter_count;
		parameters.resize(parameter_count);
		for (material_parameter_t& parameter : parameters)
			parameter.deserialize(stream);

		stream >> double_sided;
		stream >> use_alpha_cutoff;
		SFG_ASSERT(textures.empty() || sampler != NULL_SID);
	}

}
