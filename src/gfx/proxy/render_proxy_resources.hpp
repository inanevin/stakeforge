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

#include "render_proxy_common.hpp"
#include "resources/common_resources.hpp"
#include "world/particles/common_particles.hpp"

#include "gfx/buffer.hpp"
#include "math/aabb.hpp"
#include "data/bitmask.hpp"
#include "memory/chunk_handle.hpp"

namespace SFG
{
	struct render_proxy_custom_buffer
	{
		buffer_cpu_gpu buffers[BACK_BUFFER_COUNT] = {};
		u32			   size						  = 0;
		resource_id	   handle					  = {};
		u8			   status					  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_texture
	{
		resource_id handle		 = {};
		gpu_index	heap_index	 = 0;
		gfx_id		hw			 = 0;
		gfx_id		intermediate = 0;
		u8			status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_material
	{
		buffer_gpu buffers[BACK_BUFFER_COUNT];
		buffer_gpu texture_buffers[BACK_BUFFER_COUNT] = {};
		u32		   buffer_size						  = 0;
		u8		   texture_count					  = 0;
		u8		   status							  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_material_runtime
	{
		gpu_index	 gpu_index_buffers[BACK_BUFFER_COUNT]		  = {NULL_GPU_INDEX};
		gpu_index	 gpu_index_texture_buffers[BACK_BUFFER_COUNT] = {NULL_GPU_INDEX};
		gpu_index	 gpu_index_sampler							  = NULL_GPU_INDEX;
		bitmask<u32> flags										  = 0;
		resource_id	 shader_handle								  = NULL_RESOURCE_ID;
		u16			 draw_priority								  = 0;
	};

	struct render_proxy_shader_variant
	{
		gfx_id		 hw = 0;
		bitmask<u32> variant_flags;
	};

	struct render_proxy_shader
	{
		resource_id	   handle		 = {};
		chunk_handle32 variants		 = {};
		u32			   variant_count = 0;
		u8			   status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_sampler
	{
		resource_id handle	   = {};
		gpu_index	heap_index = 0;
		gfx_id		hw		   = 0;
		u8			status	   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_primitive
	{
		u32 vertex_start   = 0;
		u32 index_start	   = 0;
		u32 index_count	   = 0;
		u16 material_index = 0;
	};

	struct render_proxy_mesh
	{
		buffer_cpu_gpu vertex_buffer = {};
		buffer_cpu_gpu index_buffer	 = {};
		aabb		   local_aabb	 = {};
		chunk_handle32 primitives;
		u32			   primitive_count = 0;
		resource_id	   handle		   = {};
		u8			   status		   = render_proxy_status::rps_inactive;
		u8			   is_skinned	   = 0;
	};

	struct render_proxy_skin
	{
		chunk_handle32 nodes	  = {};
		chunk_handle32 matrices	  = {};
		u16			   node_count = 0;
		i16			   root_node  = -1;
		u8			   status	  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_particle_resource
	{
		particle_emit_properties emit_props = {};
		u8						 status		= render_proxy_status::rps_inactive;
	};

}
