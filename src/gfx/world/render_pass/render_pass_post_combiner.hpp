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
#include "math/vector2.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;

	class render_pass_post_combiner
	{
	private:
		struct ubo
		{
			vector2 screen_size			 = vector2::zero;
			f32		bloom_strength		 = 0.0f;
			f32		exposure			 = 0;
			int		tonemap_mode		 = 0;
			f32		saturation			 = 1.0f;
			f32		wb_temp				 = 0.0f;
			f32		wb_tint				 = 0.0f;
			f32		reinhard_white_point = 3.0f;
			f32		pad;
		};

		struct per_frame_data
		{
			buffer_gpu ubo					   = {};
			gfx_id	   cmd_buffer			   = 0;
			gfx_id	   render_target		   = 0;
			gpu_index  gpu_index_render_target = 0;
		};

	public:
		struct render_params
		{
			u8				   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_lighting;
			gpu_index		   gpu_index_bloom;
			gpu_index		   gpu_index_selection_outline;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, u8 frame_index, const vector2ui16& resolution);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(u8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_render_target;
		}

		inline gfx_id get_output_hw(u8 frame_index) const
		{
			return _pfd[frame_index].render_target;
		}

		inline gfx_id get_cmd_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_post_combiner = 0;
	};
}
