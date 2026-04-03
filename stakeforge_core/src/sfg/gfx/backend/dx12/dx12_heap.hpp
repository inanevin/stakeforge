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

#include "data/vector.hpp"
#include "gfx/common/descriptor_handle.hpp"

struct ID3D12DescriptorHeap;
struct ID3D12Device;

namespace sfg
{
	class dx12_heap_t
	{
	private:
		struct block_t
		{
			u32 start = 0;
			u32 count = 0;
		};

	public:
		dx12_heap_t()  = default;
		~dx12_heap_t() = default;

		void				init(ID3D12Device* device, u32 heap_type, u32 num_descriptors, u32 descriptor_size, bool shader_access);
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
