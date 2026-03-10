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

#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "math/vector4.hpp"

namespace SFG
{
	class world;

	class comp_skybox
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);
		void set_values(world& w, const vector4& start_color, const vector4& mid_color, const vector4& end_color, const vector4& fog_color, f32 fog_start, f32 fog_end);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header	  = {};
		vector4			 _start_color = vector4(0.2f, 0.1f, 0.2f, 1.0f);
		vector4			 _mid_color	  = vector4(0.1f, 0.1f, 0.2f, 1.0f);
		vector4			 _end_color	  = vector4(0.2f, 0.1f, 0.1f, 1.0f);
		vector4			 _fog_color	  = vector4(0.0f, 0.0f, 0.0f, 0.0f);
		f32				 _fog_start	  = 0.0f;
		f32				 _fog_end	  = 0.0f;
	};

	REFLECT_TYPE(comp_skybox);
}
