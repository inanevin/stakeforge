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

#include "mesh_raw.hpp"
#include "mesh.hpp"
#include "primitive.hpp"
#include "io/log.hpp"
#include "memory/chunk_allocator.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"

namespace SFG
{

	void mesh_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << sid;
		stream << node_index;
		stream << primitives_static;
		stream << primitives_skinned;
		stream << skin_index;
		stream << local_aabb;
		stream << materials;
		stream << collider_vertices;
		stream << collider_indices;
	}

	void mesh_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> sid;
		stream >> node_index;
		stream >> primitives_static;
		stream >> primitives_skinned;
		stream >> skin_index;
		stream >> local_aabb;
		stream >> materials;
		stream >> collider_vertices;
		stream >> collider_indices;
	}

}
