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
#include "game/game_max_defines.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	struct view;
	struct renderable_object;
	class proxy_manager;

	class render_pass_opaque
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			buffer_gpu										 ubo;
			static_vector<gfx_id, GBUFFER_COLOR_TEXTURES>	 color_textures			  = NULL_GFX_ID;
			static_vector<gpu_index, GBUFFER_COLOR_TEXTURES> gpu_index_color_textures = NULL_GFX_ID;
			gfx_id											 cmd_buffer				  = 0;
		};

	public:
		struct render_params
		{
			u8				   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_entities;
			gpu_index		   gpu_index_bones;
			gfx_id			   depth_texture;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& sizes);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, u8 frame_index);
		void render(const render_params& params);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(u8 frame_index, u8 texture_index) const
		{
			return _pfd[frame_index].gpu_index_color_textures[texture_index];
		}

		inline const static_vector<gpu_index, GBUFFER_COLOR_TEXTURES>& get_output_gpu_index(u8 frame_index) const
		{
			return _pfd[frame_index].gpu_index_color_textures;
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
		draw_stream	   _draw_stream;
		bump_allocator _alloc = {};
	};
}
