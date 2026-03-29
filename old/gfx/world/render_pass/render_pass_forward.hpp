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
#include "data/static_vector.hpp"

#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/draw_stream.hpp"
#include "gfx/common/gfx_constants.hpp"

#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4.hpp"
#include "math/vector2.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	struct view;
	struct renderable_object;
	class proxy_manager;

	class render_pass_forward
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj	 = matrix4x4::identity;
			vector4	  ambient	 = vector4::zero;
			vector4	  camera_pos = vector4::zero;

			vector2 resolution	= vector2::zero;
			vector2 proj_params = vector2::zero; // tanhalffov, orthohalfheight
		};

		struct per_frame_data
		{
			buffer_gpu ubo;
			gfx_id	   cmd_buffer = 0;
		};

	public:
		struct render_params
		{
			u8				   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_entities;
			gpu_index		   gpu_index_bones;
			gpu_index		   gpu_index_dir_lights;
			gfx_id			   depth_texture;
			gfx_id			   input_texture;
			gfx_id			   global_layout;
			gfx_id			   global_group;
			bool			   transition;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& sizes);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, const vector2ui16& resolution, u8 frame_index);
		void render(const render_params& params);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		per_frame_data		 _pfd[BACK_BUFFER_COUNT];
		draw_stream_distance _draw_stream;
		bump_allocator		 _alloc = {};
	};
}
