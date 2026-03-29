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
#include "resources/common_resources.hpp"
#include "memory/pool_handle.hpp"

namespace SFG
{
	class world;

	class comp_particle_emitter
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// comp
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// comp
		// -----------------------------------------------------------------------------

		void set_values(world& w, resource_handle particle_resource, resource_handle material);
		void restart(world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline resource_handle get_material() const
		{
			return _material;
		}

		inline resource_handle get_particle_resource() const
		{
			return _particle_resource;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header			= {};
		resource_handle	 _material			= {};
		resource_handle	 _particle_resource = {};
	};

	REFLECT_TYPE(comp_particle_emitter);
}
