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

#define NOMINMAX
#include <limits>
#include "memory/pool_handle.hpp"

namespace sfg
{
#define BACK_BUFFER_COUNT 3
#define FRAME_LATENCY	  2

	// 0 discrete, 1 integratd
#define GPU_DEVICE 0

	typedef unsigned short gfx_id_t;
	typedef unsigned short primitive_index;
	typedef unsigned int   gpu_index_t;

	struct gfx_resource_handle_tag
	{
	};
	struct gfx_texture_handle_tag
	{
	};
	struct gfx_texture_shared_handle_tag
	{
	};
	struct gfx_sampler_handle_tag
	{
	};
	struct gfx_swapchain_handle_tag
	{
	};
	struct gfx_semaphore_handle_tag
	{
	};
	struct gfx_shader_handle_tag
	{
	};
	struct gfx_bind_group_handle_tag
	{
	};
	struct gfx_command_buffer_handle_tag
	{
	};
	struct gfx_command_allocator_handle_tag
	{
	};
	struct gfx_queue_handle_tag
	{
	};
	struct gfx_indirect_signature_handle_tag
	{
	};
	struct gfx_descriptor_handle_tag
	{
	};
	struct gfx_bind_layout_handle_tag
	{
	};

	typedef pool_handle_t<gfx_id_t, gfx_resource_handle_tag>		   gfx_resource_handle;
	typedef pool_handle_t<gfx_id_t, gfx_texture_handle_tag>			   gfx_texture_handle;
	typedef pool_handle_t<gfx_id_t, gfx_texture_shared_handle_tag>	   gfx_texture_shared_handle;
	typedef pool_handle_t<gfx_id_t, gfx_sampler_handle_tag>			   gfx_sampler_handle;
	typedef pool_handle_t<gfx_id_t, gfx_swapchain_handle_tag>		   gfx_swapchain_handle;
	typedef pool_handle_t<gfx_id_t, gfx_semaphore_handle_tag>		   gfx_semaphore_handle;
	typedef pool_handle_t<gfx_id_t, gfx_shader_handle_tag>			   gfx_shader_handle;
	typedef pool_handle_t<gfx_id_t, gfx_bind_group_handle_tag>		   gfx_bind_group_handle;
	typedef pool_handle_t<gfx_id_t, gfx_command_buffer_handle_tag>	   gfx_command_buffer_handle;
	typedef pool_handle_t<gfx_id_t, gfx_command_allocator_handle_tag>  gfx_command_allocator_handle;
	typedef pool_handle_t<gfx_id_t, gfx_queue_handle_tag>			   gfx_queue_handle;
	typedef pool_handle_t<gfx_id_t, gfx_indirect_signature_handle_tag> gfx_indirect_signature_handle;
	typedef pool_handle_t<gfx_id_t, gfx_descriptor_handle_tag>		   gfx_descriptor_handle;
	typedef pool_handle_t<gfx_id_t, gfx_bind_layout_handle_tag>		   gfx_bind_layout_handle;

#define NULL_GFX_ID	   (unsigned short)0xFFFF
#define NULL_GPU_INDEX (unsigned int)0xFFFFFFFF
}
