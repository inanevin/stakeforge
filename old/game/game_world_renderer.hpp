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
#include "data/vector.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/world/render_pass/render_pass_opaque.hpp"
#include "gfx/world/render_pass/render_pass_pre_depth.hpp"
#include "gfx/world/render_pass/render_pass_lighting.hpp"
#include "gfx/world/render_pass/render_pass_shadows.hpp"
#include "gfx/world/render_pass/render_pass_ssao.hpp"
#include "gfx/world/render_pass/render_pass_bloom.hpp"
#include "gfx/world/render_pass/render_pass_post_combiner.hpp"
#include "gfx/world/render_pass/render_pass_forward.hpp"
#include "gfx/world/render_pass/render_pass_canvas_2d.hpp"
#include "gfx/world/render_pass/render_pass_particles.hpp"
#include "gfx/world/render_pass/render_pass_sprite.hpp"
#include "gfx/proxy/proxy_manager.hpp"

#ifdef SFG_TOOLMODE
#include "gfx/world/render_pass/render_pass_object_id.hpp"
#include "gfx/world/render_pass/render_pass_selection_outline.hpp"
#endif

#include "gfx/world/render_pass/render_pass_debug_rendering.hpp"

#include "gfx/world/view.hpp"

namespace SFG
{
	class texture_queue;
	class buffer_queue;
	class texture;
	class proxy_manager;
	class world;

	class game_world_renderer
	{
	public:
		struct task_common
		{
			game_world_renderer* rend;
			vector2ui16			 resolution;
			gpu_index			 gpu_index_entities;
			gpu_index			 gpu_index_bones;
			const gpu_index*	 gpu_index_gbuffer_textures;
			gpu_index			 gpu_index_point_lights;
			gpu_index			 gpu_index_spot_lights;
			gpu_index			 gpu_index_dir_lights;
			gpu_index			 gpu_index_shadow_data_buffer;
			gpu_index			 gpu_index_float_buffer;
			gpu_index			 gpu_index_depth_texture;
			gpu_index			 gpu_index_ao_out;
			gpu_index			 gpu_index_lighting;
			gpu_index			 gpu_index_bloom;
			gpu_index			 gpu_index_selection_outline;
			gfx_id				 layout_global;
			gfx_id				 layout_global_compute;
			gfx_id				 bind_group_global;
			gfx_id				 depth_texture;
			gfx_id				 lighting_texture;
			gfx_id				 post_combiner_texture;
			gfx_id				 canvas_input_texture;
			u8					 frame_index;
		};

	private:
		static void run_pre_depth(const void* ctx);
		static void run_shadows(const void* ctx);
		static void run_opaque(const void* ctx);
		static void run_ssao(const void* ctx);
		static void run_particles_compute(const void* ctx);
		static void run_lighting(const void* ctx);
		static void run_forward(const void* ctx);
		static void run_particles_render(const void* ctx);
		static void run_sprite(const void* ctx);
		static void run_post(const void* ctx);
		static void run_bloom(const void* ctx);
		static void run_canvas_2d(const void* ctx);

#ifdef SFG_TOOLMODE
		static void run_obj_id(const void* ctx);
		static void run_selection_outline(const void* ctx);
		static void run_debug_rendering(const void* ctx);
#endif

		struct task
		{
			void  (*fn)(const void*) = nullptr;
			void* ctx				 = nullptr;
			void  operator()() const
			{
				fn(ctx);
			}
		};

		struct per_frame_data
		{
			buffer		   shadow_data_buffer;
			buffer		   entity_buffer;
			buffer		   bones_buffer;
			buffer		   point_lights_buffer;
			buffer		   dir_lights_buffer;
			buffer		   spot_lights_buffer;
			buffer		   float_buffer;
			semaphore_data semp_frame		   = {};
			semaphore_data semp_ssao		   = {};
			semaphore_data semp_lighting	   = {};
			gfx_id		   cmd_upload		   = NULL_GFX_ID;
			u32			   _float_buffer_count = 0;

			inline void reset()
			{
				_float_buffer_count = 0;
			}
		};

	public:
		game_world_renderer() = delete;
		game_world_renderer(proxy_manager& pm, world& w);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq, gfx_id bind_layout, gfx_id bind_layout_compute);
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(u8 frame_index);
		void render(u8 frame_index, gfx_id layout_global, gfx_id layout_global_compute, gfx_id bind_group_global, u64 prev_copy, u64 next_copy, gfx_id sem_copy);
		void resize(const vector2ui16& size);
		u32	 add_to_float_buffer(u8 frame_index, f32 f);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(u8 frame_index)
		{
			return _pass_post.get_output_gpu_index(frame_index);
		}

		inline gpu_index get_albedo_gpu_index(u8 frame_index)
		{
			return _pass_opaque.get_output_gpu_index(frame_index, 0);
		}

		inline gpu_index get_normal_gpu_index(u8 frame_index)
		{
			return _pass_opaque.get_output_gpu_index(frame_index, 1);
		}

		inline gpu_index get_orm_gpu_index(u8 frame_index)
		{
			return _pass_opaque.get_output_gpu_index(frame_index, 2);
		}

		inline gpu_index get_emissive_gpu_index(u8 frame_index)
		{
			return _pass_opaque.get_output_gpu_index(frame_index, 3);
		}

		inline gpu_index get_lighting_gpu_index(u8 frame_index)
		{
			return _pass_lighting.get_output_gpu_index(frame_index);
		}

		inline gpu_index get_ssao_gpu_index(u8 frame_index)
		{
			return _pass_ssao.get_output_gpu_index(frame_index);
		}

		inline gpu_index get_bloom_gpu_index(u8 frame_index)
		{
			return _pass_bloom.get_output_gpu_index(frame_index);
		}

		inline gpu_index get_depth_gpu_index(u8 frame_index)
		{
			return _pass_pre_depth.get_output_gpu_index(frame_index);
		}

		inline const semaphore_data& get_final_semaphore(u8 frame_index)
		{
			return _pfd[frame_index].semp_frame;
		}

#ifdef SFG_TOOLMODE

		inline render_pass_object_id& get_render_pass_object_id()
		{
			return _pass_object_id;
		}

		inline render_pass_selection_outline& get_render_pass_selection_outline()
		{
			return _pass_selection_outline;
		}

		inline render_pass_debug& get_render_pass_debug_rendering()
		{
			return _pass_debug_rendering;
		}

#endif

		inline const view& get_main_view() const
		{
			return _main_camera_view;
		}

	private:
		// -----------------------------------------------------------------------------
		// render util
		// -----------------------------------------------------------------------------

		// -----------------------------------------------------------------------------
		// world collect
		// -----------------------------------------------------------------------------

		void collect_and_upload(u8 frame_index);
		void collect_and_upload_entities(gfx_id cmd_buffer, u8 frame_index);
		void collect_and_upload_bones(gfx_id cmd_buffer, u8 frame_index);
		void collect_and_upload_lights(gfx_id cmd_buffer, u8 frame_index);

	private:
		proxy_manager&			  _proxy_manager;
		world&					  _world;
		per_frame_data			  _pfd[BACK_BUFFER_COUNT];
		view					  _main_camera_view = {};
		vector<renderable_object> _renderables;

		render_pass_pre_depth	  _pass_pre_depth = {};
		render_pass_opaque		  _pass_opaque	  = {};
		render_pass_lighting	  _pass_lighting  = {};
		render_pass_shadows		  _pass_shadows	  = {};
		render_pass_ssao		  _pass_ssao	  = {};
		render_pass_bloom		  _pass_bloom	  = {};
		render_pass_post_combiner _pass_post	  = {};
		render_pass_forward		  _pass_forward	  = {};
		render_pass_canvas_2d	  _pass_canvas_2d = {};
		render_pass_particles	  _pass_particles = {};
		render_pass_sprite		  _pass_sprite	  = {};

#ifdef SFG_TOOLMODE
		render_pass_debug			  _pass_debug_rendering	  = {};
		render_pass_object_id		  _pass_object_id		  = {};
		render_pass_selection_outline _pass_selection_outline = {};
		u32							  _target_world_id		  = 0;
#endif

		vector2ui16 _base_size = vector2ui16::zero;
	};
}
