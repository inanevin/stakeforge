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
#include "math/vector2ui16.hpp"

#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/world/view.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/draw_stream.hpp"
#include "gfx/common/barrier_description.hpp"

#include "game/game_max_defines.hpp"
#include "memory/bump_allocator.hpp"
#include "math/vector4.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;
	struct view;
	struct world_render_data;

	class render_pass_shadows
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			gfx_id cmd_buffer = NULL_GFX_ID;
		};

		struct pass
		{
			draw_stream				  stream				  = {};
			vector<renderable_object> renderables			  = {};
			view					  pass_view				  = {};
			buffer_gpu				  ubos[BACK_BUFFER_COUNT] = {};
			vector2ui16				  resolution			  = vector2ui16::zero;
			gfx_id					  texture				  = {};
			u8						  view_index			  = 0;
			u8						  transition_owner		  = 0;
		};

	public:
		struct render_params
		{
			u8				   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_entities;
			gpu_index		   gpu_index_bones;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		struct pass_props
		{
			proxy_manager&	   pm;
			u8				   frame_index;
			const vector2ui16& res;
			gfx_id			   texture;
			u8				   transition_owner;
			u8				   view_index;
			const matrix4x4&   proj;
			const matrix4x4&   view;
			const vector3&	   position;
			f32				   cascade_near;
			f32				   cascade_far;
			f32				   fov;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const view& main_camera_view, const vector2ui16& resolution, u8 frame_index);
		void render(const render_params& params);
		void add_pass(const pass_props& p);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline u16 get_pass_count() const
		{
			return _pass_count;
		}

	private:
		pass*			_passes		= nullptr;
		u16				_pass_count = 0;
		per_frame_data	_pfd[BACK_BUFFER_COUNT];
		vector<barrier> _barriers;
		bump_allocator	_alloc = {};
	};
}
