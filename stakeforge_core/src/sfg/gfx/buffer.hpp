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

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/data/bitmask.hpp>

namespace sfg
{
	struct resource_desc_t;

	class buffer_gpu_t
	{
	public:
		void create(const resource_desc_t& desc);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);

		inline gfx_handle_t get_gpu() const
		{
			return _hw;
		}

		inline gpu_index_t get_index() const
		{
			return _index;
		}

	private:
		u8*			_mapped = nullptr;
		gpu_index_t _index	= NULL_GPU_INDEX;

#ifdef SFG_DEBUG
		u32 _total_size = 0;
#endif

		gfx_handle_t _hw = {};
	};

	class buffer_cpu_gpu_t
	{
	public:
		void create(const resource_desc_t& desc_cpu, const resource_desc_t& desc_gpu);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);
		void copy(gfx_handle_t cmd_buffer);
		void copy_region(gfx_handle_t cmd_buffer, size_t padding, size_t size);

		inline gfx_handle_t get_staging() const
		{
			return _hw_staging;
		}

		inline gfx_handle_t get_gpu() const
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
		gfx_handle_t _hw_staging = {};
		gfx_handle_t _hw_gpu	 = {};
	};

	class buffer_t
	{
	public:
		void create(const resource_desc_t& staging, const resource_desc_t& hw);
		void destroy();
		void buffer_data(size_t padding, const void* data, size_t size);
		void copy(gfx_handle_t cmd_buffer);
		void copy_region(gfx_handle_t cmd_buffer, size_t padding, size_t size);

		inline gfx_handle_t get_staging() const
		{
			return _hw_staging;
		}
		inline gfx_handle_t get_gpu() const
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

		gpu_index_t _index			 = NULL_GPU_INDEX;
		gpu_index_t _index_secondary = NULL_GPU_INDEX;

		gfx_handle_t _hw_staging = {};
		gfx_handle_t _hw_gpu	 = {};
	};

}
