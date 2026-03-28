#pragma once

#include "gfx/buffer.hpp"
#include "gfx/draw_stream.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "data/vector.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;

	class render_pass_sprite
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj;
		};

		struct per_frame_data
		{
			buffer_gpu ubo			 = {};
			buffer	   instance_data = {};
			gfx_id	   cmd_buffer	 = NULL_GFX_ID;
		};

		struct sprite_instance_data
		{
			u32 entity_index = 0;
		};

		struct sprite_group
		{
			gfx_id		pipeline = NULL_GFX_ID;
			resource_id material = NULL_RESOURCE_ID;
			u32			start	 = 0;
			u32			count	 = 0;
			u32			cursor	 = 0;
		};

		struct sprite_instance
		{
			u32 group		 = 0;
			u32 idx			 = 0;
			u32 entity_index = 0;
		};

	public:
		struct render_params
		{
			u8				   frame_index;
			const vector2ui16& size;
			gfx_id			   global_layout;
			gfx_id			   global_group;
			gfx_id			   input_texture;
			gfx_id			   depth_texture;
			gpu_index		   gpu_index_entities;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size);
		void uninit();
		void prepare(u8 frame_index, proxy_manager& pm, const view& main_camera_view);
		void render(const render_params& params);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		vector<sprite_group>	_reuse_groups;
		vector<sprite_instance> _reuse_instances;
		per_frame_data			_pfd[BACK_BUFFER_COUNT];
		draw_stream_sprite		_draw_stream;
		bump_allocator			_alloc = {};
	};
}
