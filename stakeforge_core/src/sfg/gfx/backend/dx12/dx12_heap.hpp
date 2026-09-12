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

#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/descriptor_handle.hpp>

struct ID3D12DescriptorHeap;
struct ID3D12Device;

namespace sfg
{
	class dx12_heap_t final
	{
	private:
		struct block_t
		{
			u32 start = 0;
			u32 count = 0;
		};

	public:
		dx12_heap_t()							   = default;
		~dx12_heap_t()							   = default;
		dx12_heap_t(const dx12_heap_t&)			   = delete;
		dx12_heap_t& operator=(const dx12_heap_t&) = delete;

		void				init(ID3D12Device* device, u32 heap_type, u32 num_descriptors, u32 descriptor_size, u32 free_block_initial_capacity, bool shader_access);
		void				uninit();
		void				reset();
		void				reset(u32 newStart);
		void				remove_handle(const descriptor_handle_t& handle);
		descriptor_handle_t get_heap_handle_block(u32 count);
		descriptor_handle_t get_offsetted_handle(u32 count);

		inline ID3D12DescriptorHeap* get_heap()
		{
			return _heap;
		}

		inline u32 get_type() const
		{
			return _type;
		}

		inline u64 get_cpu_start() const
		{
			return _cpu_start;
		}

		inline u64 get_gpu_start() const
		{
			return _gpu_start;
		}

		inline u32 get_max_descriptors() const
		{
			return _max_descriptors;
		}

		inline u32 get_descriptor_size() const
		{
			return _descriptor_size;
		}

		inline u32 get_current_index()
		{
			return _current_index;
		};

	private:
		ID3D12DescriptorHeap* _heap		 = nullptr;
		u32					  _type		 = 0;
		u64					  _cpu_start = {};
		u64					  _gpu_start = {};
		vector_t<block_t>	  _available_blocks;
		u32					  _max_descriptors = 0;
		u32					  _descriptor_size = 0;
		u32					  _current_index   = 0;
		bool				  _shader_access   = false;
	};
}
