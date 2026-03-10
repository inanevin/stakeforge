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
#include "math/vector4ui16.hpp"

namespace SFG
{
	struct draw_command
	{
		// geometry
		u32 start_index = 0;
		u32 index_count = 0;
		u32 base_vertex = 0;

		// state bindings
		gpu_index material_constant_index = NULL_GPU_INDEX;
		gpu_index texture_constant_index  = NULL_GPU_INDEX;
		gpu_index sampler_constant_index  = NULL_GPU_INDEX;
		gpu_index entity_constant_index	  = NULL_GPU_INDEX;
		gpu_index bone_constant_index	  = NULL_GPU_INDEX;

#ifdef SFG_TOOLMODE
		gpu_index entity_world_id = NULL_GPU_INDEX;
#endif

		// state bindings
		gfx_id vb_hw	   = NULL_GFX_ID;
		gfx_id ib_hw	   = NULL_GFX_ID;
		gfx_id pipeline_hw = NULL_GFX_ID;

		u16 vertex_size = 0;
		u16 priority	= 0;
	};

	struct draw_command_distance
	{
		// geometry
		u32 start_index = 0;
		u32 index_count = 0;
		u32 base_vertex = 0;

		// state bindings
		gpu_index material_constant_index = NULL_GPU_INDEX;
		gpu_index texture_constant_index  = NULL_GPU_INDEX;
		gpu_index sampler_constant_index  = NULL_GPU_INDEX;
		gpu_index entity_constant_index	  = NULL_GPU_INDEX;
		gpu_index bone_constant_index	  = NULL_GPU_INDEX;

#ifdef SFG_TOOLMODE
		gpu_index entity_world_id = NULL_GPU_INDEX;
#endif

		// state bindings
		gfx_id vb_hw	   = NULL_GFX_ID;
		gfx_id ib_hw	   = NULL_GFX_ID;
		gfx_id pipeline_hw = NULL_GFX_ID;

		u16 vertex_size = 0;
		u16 priority	= 0;

		f32 distance = 0.0f;
	};

	struct draw_command_gui
	{
		vector4ui16 clip = vector4ui16::zero;

		u32 start_index = 0;
		u32 index_count = 0;
		u32 base_vertex = 0;

		gpu_index material_constant_index = NULL_GPU_INDEX;
		gpu_index texture_constant_index  = NULL_GPU_INDEX;
		gpu_index font_index			  = NULL_GPU_INDEX;

		gfx_id vb_hw	   = NULL_GFX_ID;
		gfx_id ib_hw	   = NULL_GFX_ID;
		gfx_id pipeline_hw = NULL_GFX_ID;

		u16 vertex_size = 0;
		u8	idx_size	= 0;
	};

	struct draw_command_particle
	{
		u32		  system_index		   = 0;
		gpu_index material_index	   = NULL_GPU_INDEX;
		gpu_index texture_buffer_index = NULL_GPU_INDEX;
		gfx_id	  pipeline_hw		   = 0;
	};

	struct draw_command_sprite
	{
		u32		  start_instance		  = 0;
		u32		  instance_count		  = 0;
		gpu_index material_constant_index = NULL_GPU_INDEX;
		gpu_index texture_constant_index  = NULL_GPU_INDEX;
		gpu_index sampler_constant_index  = NULL_GPU_INDEX;
		gfx_id	  pipeline_hw			  = 0;
	};

	static_assert(sizeof(draw_command) <= 64, "cache line pls");
	static_assert(sizeof(draw_command_distance) <= 64, "cache line pls");
	static_assert(sizeof(draw_command_gui) <= 64, "cache line pls");
	static_assert(sizeof(draw_command_particle) <= 64, "cache line pls");
	static_assert(sizeof(draw_command_sprite) <= 64, "cache line pls");

	class bump_allocator;

	struct draw_stream_bound_state_pipeline
	{
		gfx_id pipeline = NULL_GFX_ID;

		inline u8 diff_mask(gfx_id cur_pipe)
		{
			if (cur_pipe == pipeline)
				return 0;

			pipeline = cur_pipe;
			return 1;
		}

		inline static u32 make_sort_key(gfx_id pipeline)
		{
			return pipeline;
		}
	};

	struct draw_stream_bound_state
	{
		gfx_id vb		= NULL_GFX_ID;
		gfx_id ib		= NULL_GFX_ID;
		gfx_id pipeline = NULL_GFX_ID;

		inline u8 diff_mask(gfx_id cur_pipe, gfx_id cur_ib, gfx_id cur_vb)
		{
			u8 mask = 0;
			if (cur_pipe != pipeline)
				mask |= 1u;
			if (cur_ib != ib)
				mask |= 1u << 1;
			if (cur_vb != vb)
				mask |= 1u << 2;

			// Update
			if (mask & 1u)
				pipeline = cur_pipe;
			if (mask & (1u << 1))
				ib = cur_ib;
			if (mask & (1u << 2))
				vb = cur_vb;
			return mask;
		}

		inline static u64 make_sort_key(gfx_id pipeline, gfx_id vb, gfx_id ib)
		{
			constexpr u32 PIPE_BITS = 20;
			constexpr u32 IB_BITS	= 22;
			constexpr u32 VB_BITS	= 22;
			return ((u64)pipeline << (IB_BITS + VB_BITS)) | ((u64)ib << VB_BITS) | ((u64)vb);
		}
	};

	class draw_stream
	{
	public:
		void prepare(bump_allocator& alloc, size_t max_commands);
		void build();
		void draw(gfx_id command_buffer);
		void add_command(const draw_command& cmd);

	private:
		draw_command* _commands		  = nullptr;
		u32			  _max_commands	  = 0;
		u32			  _commands_count = 0;
	};

	class draw_stream_distance
	{
	public:
		void prepare(bump_allocator& alloc, size_t max_commands);
		void build();
		void draw(gfx_id command_buffer);
		void add_command(const draw_command_distance& cmd);

	private:
		draw_command_distance* _commands	   = nullptr;
		u32					   _max_commands   = 0;
		u32					   _commands_count = 0;
	};

	class draw_stream_gui
	{
	public:
		void prepare(bump_allocator& alloc, size_t max_commands);
		void build();
		void draw(gfx_id command_buffer);
		void draw_no_clip(gfx_id command_buffer);
		void add_command(const draw_command_gui& cmd);

	private:
		draw_command_gui* _commands		  = nullptr;
		u32				  _max_commands	  = 0;
		u32				  _commands_count = 0;
	};

	class draw_stream_particle
	{
	public:
		void prepare(bump_allocator& alloc, size_t max_commands);
		void build();
		void draw(gfx_id command_buffer, gfx_id indirect_buffer, gfx_id indirect_signature, u32 indirect_buffer_size, u32 max_instances_per_system);
		void add_command(const draw_command_particle& cmd);

	private:
		draw_command_particle* _commands	   = nullptr;
		u32					   _max_commands   = 0;
		u32					   _commands_count = 0;
	};

	class draw_stream_sprite
	{
	public:
		void prepare(bump_allocator& alloc, size_t max_commands);
		void build();
		void draw(gfx_id command_buffer);
		void add_command(const draw_command_sprite& cmd);

	private:
		draw_command_sprite* _commands		 = nullptr;
		u32					 _max_commands	 = 0;
		u32					 _commands_count = 0;
	};
}
