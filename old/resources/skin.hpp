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

#pragma once

#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"
#include "reflection/type_reflection.hpp"

namespace SFG
{
	struct skin_raw;
	class world;

	class skin
	{
	public:
		static void reflect();
		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const skin_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline chunk_handle32 get_joints() const
		{
			return _joints;
		}

		inline u16 get_joints_count() const
		{
			return _joints_count;
		}

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _joints;
		u16			   _joints_count = 0;
		i16			   _root		 = -1;
	};

	REFLECT_TYPE(skin);

}
