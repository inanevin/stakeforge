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

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
#define WORLD_RENDER_REFLECTION_ALLOCATION_CAPACITY	 256
#define WORLD_RENDER_REFLECTION_FACE_COUNT			 6
#define WORLD_RENDER_REFLECTION_SH_COEFFICIENT_COUNT 9

	struct world_render_reflection_context_config_t
	{
		u16 allocation_max = 0;
	};

	struct world_render_reflection_allocation_t
	{
		u8*			 mapped_view_data[BACK_BUFFER_COUNT][WORLD_RENDER_REFLECTION_FACE_COUNT]  = {};
		gfx_handle_t view_data[BACK_BUFFER_COUNT][WORLD_RENDER_REFLECTION_FACE_COUNT]		  = {};
		gfx_handle_t radiance_texture														  = {};
		gfx_handle_t specular_texture														  = {};
		gfx_handle_t depth_texture															  = {};
		u64			 last_used_id															  = 0;
		u64			 retire_id																  = 0;
		gpu_index_t	 view_data_indices[BACK_BUFFER_COUNT][WORLD_RENDER_REFLECTION_FACE_COUNT] = {};
		gpu_index_t	 radiance_texture_index													  = NULL_GPU_INDEX;
		gpu_index_t	 specular_texture_index													  = NULL_GPU_INDEX;
		gpu_index_t	 specular_texture_uav_indices[TEXTURE_MAX_VIEWS]						  = {};
		gpu_index_t	 depth_texture_index													  = NULL_GPU_INDEX;
		u32			 diffuse_sh_coefficient_offset											  = UINT32_MAX;
		u32			 stable_id																  = UINT32_MAX;
		u32			 generation																  = UINT32_MAX;
		u16			 resolution																  = 0;
		u8			 specular_mip_count														  = 0;
		u8			 pending_render															  = 0;
		u8			 ready																	  = 0;
		u8			 pad[3]																	  = {};
	};

	class world_render_reflection_context_t final
	{
	public:
		world_render_reflection_context_t()													   = default;
		~world_render_reflection_context_t()												   = default;
		world_render_reflection_context_t(const world_render_reflection_context_t&)			   = delete;
		world_render_reflection_context_t& operator=(const world_render_reflection_context_t&) = delete;
		world_render_reflection_context_t(world_render_reflection_context_t&& other) noexcept;
		world_render_reflection_context_t& operator=(world_render_reflection_context_t&& other) noexcept;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------
		void init(const world_render_reflection_context_config_t& config);
		void uninit();
		void begin_allocations();
		void end_allocations();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		world_render_reflection_allocation_t* get_or_create_allocation(u32 stable_id, u16 resolution, u32 generation);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------
		const world_render_reflection_allocation_t* find_allocation(u32 stable_id) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		inline gfx_handle_t get_diffuse_sh_buffer() const
		{
			return _diffuse_sh_buffer;
		}

		inline gfx_handle_t get_probe_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].probe_buffer;
		}

		inline gfx_handle_t get_command_buffer_graphics(u8 frame_index) const
		{
			return _pfd[frame_index].command_buffer_graphics;
		}

		inline gfx_handle_t get_command_buffer_compute(u8 frame_index) const
		{
			return _pfd[frame_index].command_buffer_compute;
		}

		inline gfx_handle_t get_semaphore(u8 frame_index) const
		{
			return _pfd[frame_index].semaphore;
		}

		inline u64 next_semaphore_value(u8 frame_index) const
		{
			return ++_pfd[frame_index].semaphore_value;
		}

		inline u8* get_mapped_probe_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_probe_buffer;
		}

		inline gpu_index_t get_probe_buffer_index(u8 frame_index) const
		{
			return _pfd[frame_index].probe_buffer_index;
		}

		inline gpu_index_t get_diffuse_sh_buffer_index() const
		{
			return _diffuse_sh_buffer_index;
		}

		inline gpu_index_t get_diffuse_sh_buffer_uav_index() const
		{
			return _diffuse_sh_buffer_uav_index;
		}

		inline gfx_handle_t get_specular_prefilter_shader() const
		{
			return _specular_prefilter_shader;
		}

		inline gfx_handle_t get_diffuse_sh_shader() const
		{
			return _diffuse_sh_shader;
		}

		inline gpu_index_t get_view_data_index(const world_render_reflection_allocation_t& allocation, u8 frame_index, u8 face) const
		{
			return allocation.view_data_indices[frame_index][face];
		}

		inline u8* get_mapped_view_data(const world_render_reflection_allocation_t& allocation, u8 frame_index, u8 face) const
		{
			return allocation.mapped_view_data[frame_index][face];
		}

		inline u16 get_probe_max() const
		{
			return _config.allocation_max;
		}

		inline world_render_reflection_allocation_t& get_allocation(u16 index)
		{
			return _allocations[index];
		}

	private:
		void destroy_allocation(world_render_reflection_allocation_t& allocation);

	private:
		struct per_frame_data_t
		{
			u8*			 mapped_probe_buffer	 = nullptr;
			gfx_handle_t command_buffer_graphics = {};
			gfx_handle_t command_buffer_compute	 = {};
			gfx_handle_t semaphore				 = {};
			gfx_handle_t probe_buffer			 = {};
			mutable u64	 semaphore_value		 = 0;
			gpu_index_t	 probe_buffer_index		 = NULL_GPU_INDEX;
		};

	private:
		per_frame_data_t						 _pfd[BACK_BUFFER_COUNT]								   = {};
		world_render_reflection_allocation_t	 _allocations[WORLD_RENDER_REFLECTION_ALLOCATION_CAPACITY] = {};
		u64										 _id_counter											   = 0;
		gfx_handle_t							 _diffuse_sh_buffer										   = {};
		gfx_handle_t							 _specular_prefilter_shader								   = {};
		gfx_handle_t							 _diffuse_sh_shader										   = {};
		gpu_index_t								 _diffuse_sh_buffer_index								   = NULL_GPU_INDEX;
		gpu_index_t								 _diffuse_sh_buffer_uav_index							   = NULL_GPU_INDEX;
		world_render_reflection_context_config_t _config												   = {};
	};
}
