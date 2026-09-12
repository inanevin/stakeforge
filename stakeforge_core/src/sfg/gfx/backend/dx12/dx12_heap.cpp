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

#include "dx12_heap.hpp"

#include "sdk/d3d12.h"
#include "dx12_common.hpp"
#include <sfg/io/log.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory_tracer.hpp>

namespace sfg
{

	void dx12_heap_t::init(ID3D12Device* device, u32 heap_type, u32 num_descriptors, u32 descriptor_size, u32 free_block_initial_capacity, bool shader_access)
	{
		_type			 = heap_type;
		_max_descriptors = num_descriptors;
		_shader_access	 = shader_access;
		_available_blocks.reserve(free_block_initial_capacity);
		_descriptor_size = descriptor_size;

		try
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors				= _max_descriptors;
			heapDesc.Type						= static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(_type);
			heapDesc.Flags						= _shader_access ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NodeMask					= 0;
			throw_if_failed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap)));
		}
		catch (HrException e)
		{
			SFG_ERR("Exception when creating a descriptor heap! {0}", e.what());
		}

		{
			SFG_MEMTRACE_SCOPE("GPU");
			SFG_MEMTRACE_ALLOC(_heap, num_descriptors * descriptor_size);
		}

		_heap->SetName(L"Descriptor Heap");
		_cpu_start = static_cast<u64>(_heap->GetCPUDescriptorHandleForHeapStart().ptr);
		if (_shader_access)
			_gpu_start = static_cast<u64>(_heap->GetGPUDescriptorHandleForHeapStart().ptr);
	}

	void dx12_heap_t::uninit()
	{
		{
			SFG_MEMTRACE_SCOPE("GPU");
			SFG_MEMTRACE_DEALLOC(_heap);
		}

		_heap->Release();
		_heap = NULL;
	}

	void dx12_heap_t::reset()
	{
		_current_index = 0;
	}

	void dx12_heap_t::reset(u32 newStart)
	{
		_current_index = newStart;
	}

	descriptor_handle_t dx12_heap_t::get_heap_handle_block(u32 count)
	{
		for (auto it = _available_blocks.begin(); it != _available_blocks.end(); ++it)
		{
			auto& block_t = *it;

			if (block_t.count >= count)
			{
				const descriptor_handle_t handle = {
					.cpu   = _cpu_start + block_t.start * _descriptor_size,
					.gpu   = _gpu_start + block_t.start * _descriptor_size,
					.index = block_t.start,
					.count = count,
				};

				block_t.start += count;
				block_t.count -= count;

				if (block_t.count == 0)
					_available_blocks.erase(it);

				return handle;
			}
		}

		u32 new_id	  = 0;
		u32 block_end = _current_index + count;

		if (block_end <= _max_descriptors)
		{
			new_id		   = _current_index;
			_current_index = block_end;
		}
		else
		{
			SFG_ASSERT(false);
			SFG_ERR("DX12Backend -> Ran out of descriptor heap handles, need to increase heap size.");
		}

		return {
			.cpu   = _cpu_start + new_id * _descriptor_size,
			.gpu   = _gpu_start + new_id * _descriptor_size,
			.index = block_end - count,
			.count = count,
		};
	}

	descriptor_handle_t dx12_heap_t::get_offsetted_handle(u32 count)
	{
		return {
			.cpu = get_cpu_start() + count * get_descriptor_size(),
			.gpu = get_gpu_start() + count * get_descriptor_size(),
		};
	}

	void dx12_heap_t::remove_handle(const descriptor_handle_t& handle)
	{
		const auto start = handle.index;
		block_t	   b	 = {handle.index, handle.count};
		_available_blocks.push_back(b);
	}
}
