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

#include "ui/panels/animation_graph/editor_animation_graph_widget_common.hpp"
#include "ui/widgets/editor_widget_button.hpp"

#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
	struct key_event_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_animation_graph_context_t;
	class editor_animation_graph_widget_node_t;

	class editor_animation_graph_grid_t final
	{
	public:
		struct config_t
		{
			editor_animation_graph_context_t* context		 = nullptr;
			f32								  grid_size		 = 32.0f;
			f32								  line_thickness = 1.0f;
		};

		editor_animation_graph_grid_t()												   = default;
		~editor_animation_graph_grid_t()											   = default;
		editor_animation_graph_grid_t(const editor_animation_graph_grid_t&)			   = delete;
		editor_animation_graph_grid_t& operator=(const editor_animation_graph_grid_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_mode(editor_animation_graph_display_mode_e mode);
		void refresh_nodes();
		void change_selection(u32 node_id);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void destroy_nodes();
		u32	 find_pin_index_at(const vec2f_t& pos) const;
		u32	 find_node_index_at(const vec2f_t& pos) const;
		u32	 find_node_at(const vec2f_t& pos) const;
		void open_context_menu(const vec2f_t& pos);
		void update_text(const char* text);
		void set_zoom_instant(f32 zoom);

		static void on_context_menu_command(u16 command, void* user_data);
		static void on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_double_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_back(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static void draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		vector_t<editor_animation_graph_widget_node_t*> _nodes						  = {};
		editor_widget_button_t							_back_button				  = {};
		ui::ui_context*									_ui							  = nullptr;
		config_t										_config						  = {};
		vec2f_t											_offset						  = vec2f_t::zero;
		vec2f_t											_zoom_pivot					  = vec2f_t::zero;
		vec2f_t											_context_menu_editor_position = vec2f_t::zero;
		u32												_context_menu_node_id		  = UINT32_MAX;
		u32												_drag_node_index			  = UINT32_MAX;
		u32												_drag_pin_node_index		  = UINT32_MAX;
		u32												_last_clicked_node_id		  = UINT32_MAX;
		u32												_double_click_node_id		  = UINT32_MAX;
		ui::widget_id_t									_root						  = NULL_WIDGET;
		ui::widget_id_t									_title						  = NULL_WIDGET;
		f32												_zoom						  = 1.0f;
		f32												_target_zoom				  = 1.0f;
		f32												_zoom_velocity				  = 0.0f;
	};
}
