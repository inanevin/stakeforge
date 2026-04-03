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

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "data/bitmask.hpp"

namespace sfg
{
	struct resource_desc_t;

	class buffer_gpu_t
	{
	public:
		void create(const resource_desc_t& desc);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);

		inline gfx_id get_gpu() const
		{
			return _hw;
		}

		inline gpu_index get_index() const
		{
			return _index;
		}

	private:
		u8*		  _mapped = nullptr;
		gpu_index _index  = NULL_GPU_INDEX;

#ifdef SFG_DEBUG
		u32 _total_size = 0;
#endif

		gfx_id _hw = NULL_GFX_ID;
	};

	class buffer_cpu_gpu_t
	{
	public:
		void create(const resource_desc_t& desc_cpu, const resource_desc_t& desc_gpu);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);
		void copy(gfx_id cmd_buffer);
		void copy_region(gfx_id cmd_buffer, size_t padding, size_t size);

		inline gfx_id get_staging() const
		{
			return _hw_staging;
		}

		inline gfx_id get_gpu() const
		{
			return _hw_gpu;
		}

		inline u8* get_mapped() const
		{
			return _mapped;
		}

	private:
		u8* _mapped = nullptr;

#ifdef SFG_DEBUG
		u32 _total_size = 0;
#endif
		gfx_id _hw_staging = NULL_GFX_ID;
		gfx_id _hw_gpu	   = NULL_GFX_ID;
	};

	class buffer_t
	{
	public:
		void create(const resource_desc_t& staging, const resource_desc_t& hw);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);
		void copy(gfx_id cmd_buffer);
		void copy_region(gfx_id cmd_buffer, size_t padding, size_t size);

		inline gfx_id get_staging() const
		{
			return _hw_staging;
		}
		inline gfx_id get_gpu() const
		{
			return _hw_gpu;
		}

		inline u32 get_index() const
		{
			return _index;
		}

		inline u32 get_index_secondary() const
		{
			return _index_secondary;
		}

		inline u8* get_mapped() const
		{
			return _mapped;
		}

	private:
		u8* _mapped = nullptr;

#ifdef SFG_DEBUG
		u32 _total_size = 0;
#endif

		gpu_index _index		   = NULL_GPU_INDEX;
		gpu_index _index_secondary = NULL_GPU_INDEX;

		gfx_id _hw_staging = NULL_GFX_ID;
		gfx_id _hw_gpu	   = NULL_GFX_ID;
	};

}
