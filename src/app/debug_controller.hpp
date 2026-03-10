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
#include "math/matrix4x4.hpp"
#include "math/vector4ui16.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "data/static_vector.hpp"
#include "math/vector2.hpp"
#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "memory/text_allocator.hpp"
#include "vendor/moodycamel/readerwriterqueue.h"

namespace vekt
{
	class builder;
	struct draw_buffer;
	class atlas;
	struct font;
	class font_manager;
}

namespace SFG
{
	enum class log_level;
	class render_pass;
	class bump_allocator;
	class buffer_queue;
	class texture_queue;
	class window;
	struct window_event;
	struct barrier;

#define MAX_GUI_DRAW_CALLS 32
#define MAX_KEY_EVENTS	   64

	class debug_controller
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(window& w, texture_queue* texture_queue, gfx_id global_bind_layout, const vector2ui16& screen_size);
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// render
		// -----------------------------------------------------------------------------

		void prepare(u8 frame_index);
		void render(gfx_id cmd_buffer, u8 frame_index, bump_allocator& alloc);
		void on_window_resize(const vector2ui16& size);
		bool on_window_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		static f32 get_field_height();

		inline u32 get_output_gpu_index(u8 frame_index) const
		{
			return _pfd[frame_index].rt_fullscreen_index;
		}

	private:
		static void on_log(log_level lvl, const char* msg, void* user_data);

		void flush_key_events();
		void build_console();
		void console_logic();
		void rebuild_ui();
		void add_console_text(const char* text, log_level level);
		void update_console_input_field();
		void draw_vekt(u8 frame_index, const vekt::draw_buffer& db);
		void set_console_visible(bool visible);
		void init_fonts();
		void init_builder();
		void uninit_fonts();
		void uninit_builder();

		static void on_atlas_created(vekt::atlas* atlas, void* user_data);
		static void on_atlas_updated(vekt::atlas* atlas, void* user_data);
		static void on_atlas_destroyed(vekt::atlas* atlas, void* user_data);

	private:
		enum class console_state : u8
		{
			invisible = 0,
			visible,
		};

		struct input_event
		{
			u16 button = 0;
			i16 wheel  = 0;
		};

		struct gui_draw_call
		{
			vector4ui16 scissors		= vector4ui16::zero;
			u16			start_vtx		= 0;
			u16			start_idx		= 0;
			u16			index_count		= 0;
			gfx_id		shader			= 0;
			u32			atlas_gpu_index = 0;
		};

		struct per_frame_data
		{
			buffer_cpu_gpu buf_gui_vtx		   = {};
			buffer_cpu_gpu buf_gui_idx		   = {};
			buffer_gpu	   buf_pass_data	   = {};
			u32			   rt_console_index	   = 0;
			u32			   rt_fullscreen_index = 0;
			u32			   counter_vtx		   = 0;
			u32			   counter_idx		   = 0;
			gfx_id		   rt_console		   = 0;
			gfx_id		   rt_post			   = 0;
			u16			   draw_call_count	   = 0;

			inline void reset()
			{
				counter_vtx = counter_idx = 0;
				draw_call_count			  = 0;
			}
		};

		struct gui_pass_view
		{
			matrix4x4 proj			= matrix4x4::identity;
			f32		  sdf_thickness = 0.5f;
			f32		  sdf_softness	= 0.02f;
		};

		struct atlas_ref
		{
			vekt::atlas*   atlas			   = nullptr;
			gfx_id		   texture			   = 0;
			u32			   texture_gpu_index   = 0;
			gfx_id		   intermediate_buffer = 0;
			texture_buffer buffer			   = {};
		};

		struct shaders
		{
			gfx_id gui_default					 = {};
			gfx_id gui_text						 = {};
			gfx_id gui_sdf						 = {};
			gfx_id debug_controller_console_draw = {};
		};

		struct gfx_data
		{
			vector<atlas_ref> atlases;
			texture_queue*	  texture_queue = nullptr;
			vector2ui16		  screen_size	= vector2ui16::zero;
			vector2ui16		  rt_size		= vector2ui16::zero;
			f32				  frame_counter = 0;
		};

		struct vekt_data
		{
			vector<i32>			console_texts			  = {};
			vekt::builder*		builder					  = nullptr;
			vekt::font*			font_debug				  = nullptr;
			vekt::font*			font_icon				  = nullptr;
			vekt::font_manager* font_manager			  = nullptr;
			i32					widget_console_bg		  = -1;
			i32					widget_input_field		  = -1;
			i32					widget_input_text		  = {};
			i32					widget_border			  = {};
			i32					widget_fps				  = 0;
			i32					widget_main_thread		  = 0;
			i32					widget_render_thread	  = 0;
			i32					widget_global_mem		  = 0;
			i32					widget_gfx_mem			  = 0;
			f32					console_total_text_size_y = 0.0f;
		};

		struct input_field
		{
			vector<const char*> history			  = {};
			const char*			text			  = nullptr;
			i16					scroll_amt		  = 0;
			i8					history_traversal = 0;
			i8					caret_pos		  = 0;
			i8					text_size		  = 0;
		};

	private:
		text_allocator											   _text_allocator = {};
		shaders													   _shaders		   = {};
		gfx_data												   _gfx_data	   = {};
		vekt_data												   _vekt_data	   = {};
		input_field												   _input_field	   = {};
		per_frame_data											   _pfd[BACK_BUFFER_COUNT];
		gui_draw_call											   _gui_draw_calls[MAX_GUI_DRAW_CALLS];
		moodycamel::ReaderWriterQueue<input_event, MAX_KEY_EVENTS> _input_events;
		moodycamel::ReaderWriterQueue<const char*, 2>			   _commands;
		console_state											   _console_state = console_state::invisible;
	};
}
