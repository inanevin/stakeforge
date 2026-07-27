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
#include <sfg/data/atomic.hpp>
#include <sfg/data/fixed_vector.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/memory/text_allocator.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg::ui
{
	class ui_context;

	using ui_pre_layout_tick_fn	 = void (*)(ui_context& ui, widget_id_t id, f32 dt_seconds, void* user_data);
	using ui_post_layout_tick_fn = void (*)(ui_context& ui, widget_id_t id, f32 dt_seconds, void* user_data);
	using ui_mutation_fn		 = void (*)(ui_context& ui, void* user_data);

	enum class ui_phase_e : u8
	{
		idle,
		mutation,
		pre_layout,
		text_layout,
		layout,
		post_layout,
		input,
		paint,
	};

	struct ui_config_t
	{
		vg_canvas_config_t canvas					 = {};
		input_config_t	   input					 = {};
		f32				   user_ui_scale			 = 1.0f;
		f32				   dpi_scale				 = 1.0f;
		u32				   max_widgets				 = 1024;
		u32				   text_pool_budget_bytes	 = 64 * 1024;
		u32				   snapshot_vertex_max_bytes = 1u << 20;
		u32				   snapshot_index_max_bytes	 = 1u << 20;
	};

	struct widget_text_ref_t
	{
		const char* ptr = nullptr;
		u32			len = 0;
	};

	class ui_context final
	{
	public:
		ui_context()							 = default;
		~ui_context()							 = default;
		ui_context(const ui_context&)			 = delete;
		ui_context& operator=(const ui_context&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const ui_config_t& cfg);
		void uninit();
		void tick(const vec4f_t& screen_rect, f32 dpi_scale, f32 dt_seconds);
		void publish_frame();
		void set_debug_draw(bool enabled);
		void set_debug_font(resource_handle_t font);

		// -----------------------------------------------------------------------------
		// widgets
		// -----------------------------------------------------------------------------

		widget_id_t allocate_widget();
		void		deallocate_widget(widget_id_t id);
		void		set_pre_layout_tick(widget_id_t id, ui_pre_layout_tick_fn fn, void* user_data);
		void		clear_pre_layout_tick(widget_id_t id);
		void		set_post_layout_tick(widget_id_t id, ui_post_layout_tick_fn fn, void* user_data);
		void		clear_post_layout_tick(widget_id_t id);
		void		request_mutation(ui_mutation_fn fn, void* user_data);
		void		request_unique_mutation(ui_mutation_fn fn, void* user_data);
		void		cancel_mutations(void* user_data);
		void		request_post_layout_solve();

		// -----------------------------------------------------------------------------
		// render-thread snapshot
		// -----------------------------------------------------------------------------

		const vg_draw_snapshot_t* acquire_render_snapshot();

		// -----------------------------------------------------------------------------
		// events
		// -----------------------------------------------------------------------------

		void on_mouse_move(const vec2f_t& pos);
		void on_mouse_button(mouse_button_e btn, bool pressed);
		void on_wheel(f32 delta);
		void on_key(const key_event_t& ev);

		// -----------------------------------------------------------------------------
		// text
		// -----------------------------------------------------------------------------

		void		set_widget_text(widget_id_t id, const char* text);
		void		clear_widget_text(widget_id_t id);
		const char* widget_text(widget_id_t id) const;
		u32			widget_text_len(widget_id_t id) const;

		// -----------------------------------------------------------------------------
		// debug
		// -----------------------------------------------------------------------------

		void		set_widget_debug_name(widget_id_t id, const char* text);
		void		clear_widget_debug_name(widget_id_t id);
		const char* widget_debug_name(widget_id_t id) const;
		u32			widget_debug_name_len(widget_id_t id) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bool is_debug_draw_enabled() const
		{
			return _debug_draw;
		}

		inline f32 get_dpi_scale() const
		{
			return _dpi_scale;
		}

		inline f32 get_ui_scale() const
		{
			return _ui_scale;
		}

		inline void set_user_ui_scale(f32 f)
		{
			_user_ui_scale = get_valid_scale(f);
		}

		inline layout_tree_t& get_tree()
		{
			return _tree;
		}
		inline const layout_tree_t& get_tree() const
		{
			return _tree;
		}
		inline paint_layer_t& get_paint()
		{
			return _paint;
		}
		inline input_router_t& get_input()
		{
			return _input;
		}
		inline vg_canvas_t& get_canvas()
		{
			return _canvas;
		}

		inline widget_id_t get_root() const
		{
			return _tree.get_root();
		}

		inline ui_phase_e get_phase() const
		{
			return _phase;
		}

		inline u32 get_snapshot_vertex_max_bytes() const
		{
			return _snapshot_slots[0].vertex_capacity * static_cast<u32>(sizeof(vg_vertex_t));
		}

		inline u32 get_snapshot_index_max_bytes() const
		{
			return _snapshot_slots[0].index_capacity * static_cast<u32>(sizeof(vg_index_t));
		}

	private:
		struct snapshot_slot_t
		{
			vg_draw_buffer_final_t* draw_buffers		 = nullptr;
			vg_vertex_t*			vertices			 = nullptr;
			vg_index_t*				indices				 = nullptr;
			u32						draw_buffer_capacity = 0;
			u32						vertex_capacity		 = 0;
			u32						index_capacity		 = 0;
			vg_draw_snapshot_t		snapshot			 = {};
		};

		struct pre_layout_tick_def_t
		{
			ui_pre_layout_tick_fn fn		= nullptr;
			void*				  user_data = nullptr;
		};

		struct post_layout_tick_def_t
		{
			ui_post_layout_tick_fn fn		 = nullptr;
			void*				   user_data = nullptr;
		};

		struct mutation_request_t
		{
			ui_mutation_fn fn		 = nullptr;
			void*		   user_data = nullptr;
		};

		void allocate_snapshot_slot(snapshot_slot_t& slot, u32 draw_buffer_capacity, u32 vertex_capacity, u32 index_capacity);
		void free_snapshot_slot(snapshot_slot_t& slot);
		void clear_widget_state_recursive(widget_id_t id);
		void drain_mutations();
		void run_pre_layout_ticks(f32 dt_seconds);
		void run_post_layout_ticks(f32 dt_seconds);
		void draw_debug_hovered_widget();
		void set_phase(ui_phase_e phase);
		bool is_topology_mutation_allowed() const;

	private:
		vg_canvas_t								   _canvas;
		input_router_t							   _input;
		layout_tree_t							   _tree;
		snapshot_slot_t							   _snapshot_slots[3] = {};
		paint_layer_t							   _paint;
		fixed_vector_t<mutation_request_t>		   _mutation_requests;
		fixed_vector_t<widget_id_t>				   _pre_layout_tick_widgets;
		fixed_vector_t<widget_id_t>				   _post_layout_tick_widgets;
		hash_map_t<widget_id_t, widget_text_ref_t> _widget_texts;
		hash_map_t<widget_id_t, widget_text_ref_t> _widget_debug_names;
		text_allocator_t						   _text_pool;
		widget_text_ref_t						   _debug_hover_text;
		atomic_t<u8>							   _snapshot_mailbox = {};
		u8										   _producer_slot	 = 0;
		u8										   _consumer_slot	 = 0;
		f32										   _user_ui_scale	 = 1.0f;
		f32										   _ui_scale		 = 1.0f;
		f32										   _dpi_scale		 = 1.0f;
		resource_handle_t						   _debug_font		 = NULL_RESOURCE_HANDLE;
		ui_phase_e								   _phase			 = ui_phase_e::idle;
		bool									   _debug_draw		 = false;
		bool									   _post_solve		 = false;
		fixed_vector_t<pre_layout_tick_def_t>	   _pre_layout_tick_defs;
		fixed_vector_t<post_layout_tick_def_t>	   _post_layout_tick_defs;
	};
}
