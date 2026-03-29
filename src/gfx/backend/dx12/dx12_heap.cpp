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

#include "dx12_heap.hpp"

#include "sdk/d3d12.h"
#include "dx12_common.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

	void dx12_heap_t::init(ID3D12Device* device, u32 heap_type, u32 num_descriptors, u32 descriptor_size, bool shader_access)
	{
		_type			 = heap_type;
		_max_descriptors = num_descriptors;
		_shader_access	 = shader_access;
		_available_blocks.reserve(num_descriptors / 2);
		_descriptor_size = descriptor_size;

		try
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
			heapDesc.NumDescriptors = _max_descriptors;
			heapDesc.Type			= static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(_type);
			heapDesc.Flags			= _shader_access ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NodeMask		= 0;
			throw_if_failed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap)));
		}
		catch (HrException e)
		{
			SFG_ERR("Exception when creating a descriptor heap! {0}", e.what());
		}

		TracyAllocN(_heap, num_descriptors * descriptor_size, "GPU: Total");

		_heap->SetName(L"Descriptor Heap");
		_cpu_start = static_cast<u64>(_heap->GetCPUDescriptorHandleForHeapStart().ptr);
		if (_shader_access)
			_gpu_start = static_cast<u64>(_heap->GetGPUDescriptorHandleForHeapStart().ptr);
	}

	void dx12_heap_t::uninit()
	{
		TracyFreeN(_heap, "GPU: Total");
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

		if (block_end < _max_descriptors)
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
