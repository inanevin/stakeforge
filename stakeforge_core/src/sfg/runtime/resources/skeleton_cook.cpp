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

#include "skeleton_cook.hpp"

#include "skeleton.hpp"
#include "skeleton_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool skeleton_cooker::cook_from_def(const skeleton_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t skeleton_stream;
		if (!reflection_registry_t::get().type_to_stream(type_id_t<skeleton_def_t>::value, const_cast<skeleton_def_t*>(&def), nullptr, skeleton_stream))
		{
			SFG_ERR("failed to serialize skeleton definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::skeleton,
			.magic		 = skeleton_loader_t::WIRE_MAGIC,
			.version	 = skeleton_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(skeleton_stream.get_raw(), skeleton_stream.get_size()),
		};

		stream.write_raw(skeleton_stream.get_raw(), skeleton_stream.get_size());
		return true;
	}
}
