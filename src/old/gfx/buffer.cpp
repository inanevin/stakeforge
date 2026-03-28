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

#include "buffer.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"
#include "memory/memory.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"

namespace SFG
{

	void buffer::create(const resource_desc& staging, const resource_desc& hw)
	{
		gfx_backend* backend = gfx_backend::get();
		SFG_ASSERT(staging.size == hw.size);

#ifdef SFG_DEBUG
		_total_size = staging.size;
#endif

		_hw_staging = backend->create_resource(staging);
		_hw_gpu		= backend->create_resource(hw);
		backend->map_resource(_hw_staging, _mapped);

		_index = backend->get_resource_gpu_index(_hw_gpu);

		if (hw.flags.is_all_set(resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write))
			_index_secondary = backend->get_resource_gpu_index(_hw_gpu, true);
	}

	void buffer::destroy()
	{
		SFG_ASSERT(_hw_gpu != NULL_GFX_ID && _hw_staging != NULL_GFX_ID);
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_resource(_hw_staging);
		backend->destroy_resource(_hw_gpu);
		_mapped			 = nullptr;
		_index			 = UINT32_MAX;
		_index_secondary = UINT32_MAX;
		_hw_staging		 = NULL_GFX_ID;
		_hw_gpu			 = NULL_GFX_ID;
	}

	void buffer::buffer_data(size_t padding, const void* data, size_t size)
	{
#ifdef SFG_DEBUG
		SFG_ASSERT(padding + size <= _total_size);
#endif
		SFG_MEMCPY(_mapped + padding, data, size);
	}

	void buffer::copy(gfx_id cmd_buffer)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource(cmd_buffer,
								   {
									   .source		= _hw_staging,
									   .destination = _hw_gpu,
								   });
	}

	void buffer::copy_region(gfx_id cmd_buffer, size_t padding, size_t size)
	{
		SFG_ASSERT(size != 0);

		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource_region(cmd_buffer,
										  {
											  .source	   = _hw_staging,
											  .destination = _hw_gpu,
											  .dst_offset  = padding,
											  .src_offset  = padding,
											  .size		   = size,
										  });
	}

	void buffer_gpu::create(const resource_desc& desc)
	{
		gfx_backend* backend = gfx_backend::get();
		_hw					 = backend->create_resource(desc);
		_index				 = backend->get_resource_gpu_index(_hw);

		backend->map_resource(_hw, _mapped);

		SFG_ASSERT(desc.flags.is_set(resource_flags::rf_cpu_visible));

#ifdef SFG_DEBUG
		_total_size = desc.size;
#endif
	}

	void buffer_gpu::destroy()
	{
		SFG_ASSERT(_hw != NULL_GFX_ID);
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_resource(_hw);
		_hw		= NULL_GFX_ID;
		_index	= NULL_GPU_INDEX;
		_mapped = nullptr;
	}

	void buffer_gpu::buffer_data(size_t padding, const void* data, size_t size)
	{
#ifdef SFG_DEBUG
		SFG_ASSERT(padding + size <= _total_size);
#endif
		SFG_MEMCPY(_mapped + padding, data, size);
	}

	void buffer_cpu_gpu::create(const resource_desc& desc_cpu, const resource_desc& desc_gpu)
	{
		gfx_backend* backend = gfx_backend::get();
		SFG_ASSERT(desc_cpu.size == desc_gpu.size);

		_hw_staging = backend->create_resource(desc_cpu);
		_hw_gpu		= backend->create_resource(desc_gpu);

		backend->map_resource(_hw_staging, _mapped);

#ifdef SFG_DEBUG
		_total_size = desc_cpu.size;
#endif
	}

	void buffer_cpu_gpu::destroy()
	{
		SFG_ASSERT(_hw_staging != NULL_GFX_ID);
		SFG_ASSERT(_hw_gpu != NULL_GFX_ID);
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_resource(_hw_staging);
		backend->destroy_resource(_hw_gpu);
		_hw_staging = _hw_gpu = NULL_GFX_ID;
		_mapped				  = nullptr;
	}

	void buffer_cpu_gpu::buffer_data(size_t padding, const void* data, size_t size)
	{
#ifdef SFG_DEBUG
		SFG_ASSERT(padding + size <= _total_size);
#endif
		SFG_MEMCPY(_mapped + padding, data, size);
	}

	void buffer_cpu_gpu::copy(gfx_id cmd_buffer)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->cmd_copy_resource(cmd_buffer,
								   {
									   .source		= _hw_staging,
									   .destination = _hw_gpu,
								   });
	}

	void buffer_cpu_gpu::copy_region(gfx_id cmd_buffer, size_t padding, size_t size)
	{
		gfx_backend* backend = gfx_backend::get();

#ifdef SFG_DEBUG
		SFG_ASSERT(padding + size <= _total_size);
#endif

		backend->cmd_copy_resource_region(cmd_buffer,
										  {
											  .source	   = _hw_staging,
											  .destination = _hw_gpu,
											  .dst_offset  = padding,
											  .src_offset  = padding,
											  .size		   = size,
										  });
	}

}
