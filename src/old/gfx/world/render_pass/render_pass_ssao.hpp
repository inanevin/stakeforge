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

#include "data/static_vector.hpp"

// gfx
#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"

#include "math/vector4.hpp"
#include "math/vector2ui.hpp"
#include "math/vector2.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;
	class texture_queue;

	class render_pass_ssao
	{
	private:
		struct ubo
		{
			matrix4x4 proj_matrix;
			matrix4x4 inv_proj_matrix;
			matrix4x4 view_matrix;

			vector2ui full_size;
			vector2ui half_size;
			vector2	  inv_full;
			vector2	  inv_half;

			// Projection
			f32 z_near;
			f32 z_far;

			// AO controls
			f32 radius_world;		 // AO radius in *world/view* units (meters)
			f32 bias;				 // small bias to reduce self-occlusion (e.g., 0.02)
			f32 intensity;			 // AO strength (e.g., 1.2)
			f32 power;				 // contrast shaping (e.g., 1.1)
			u32 num_dirs;			 // e.g., 8
			u32 num_steps;			 // e.g., 6
			f32 random_rot_strength; // e.g., 1.0
		};

		struct per_frame_data
		{
			buffer_gpu ubo						 = {};
			gfx_id	   cmd_buffer				 = 0;
			gfx_id	   ao_out					 = 0;
			gfx_id	   ao_upsample_out			 = 0;
			gpu_index  gpu_index_ao_uav			 = 0;
			gpu_index  gpu_index_ao_srv			 = 0;
			gpu_index  gpu_index_ao_upsample_uav = 0;
			gpu_index  gpu_index_ao_upsample_srv = 0;
		};

	public:
		struct render_params
		{
			u8				   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_depth;
			gpu_index		   gpu_index_normals;
			gfx_id			   global_layout_compute;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size, texture_queue& tq);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const view& main_camera_view, const vector2ui16& resolution, u8 frame_index);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline gpu_index get_output_gpu_index(u8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_ao_upsample_srv;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_hbao			   = 0;
		gfx_id		   _shader_hbao_upsample   = 0;
		gfx_id		   _noise_tex			   = 0;
		gfx_id		   _noise_tex_intermediate = 0;
		gpu_index	   _gpu_index_noise		   = 0;
	};
}
