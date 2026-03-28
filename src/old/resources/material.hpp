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
#include "data/bitmask.hpp"
#include "data/ostream.hpp"
#include "resources/common_resources.hpp"
#include "reflection/type_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	enum class material_pass_mode;
	struct material_raw;
	class world;

	class material
	{
	public:
		static void reflect();
		~material();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		struct material_creation_params
		{
			const char*			   name;
			material_pass_mode	   pass_mode;
			resource_handle		   handle;
			resource_handle		   shader;
			resource_handle		   sampler;
			const resource_handle* textures;
			u8				   textures_count;
			const void*			   data;
			size_t				   data_size;
		};

		void create_manual(world& w, const material_creation_params& p);
		void create_from_loader(material_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void update_data(world& w, resource_handle material_own_handle, size_t padding, const void* data, size_t data_size);
		void update_sampler(world& w, resource_handle material_own_handle, resource_handle sampler);
		void update_textures(world& w, resource_handle material_own_handle, const resource_handle* textures, u8 start, u8 count);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const bitmask<u32>& get_flags() const
		{
			return _flags;
		}

	private:
		bitmask<u32> _flags = 0;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REFLECT_TYPE(material);

}
