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

#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	struct input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_tab_area_t;

	struct editor_tab_t
	{
		sid_t			identifier		= 0;
		ui::widget_id_t widget			= NULL_WIDGET;
		ui::widget_id_t marker			= NULL_WIDGET;
		ui::widget_id_t marker_inner	= NULL_WIDGET;
		ui::widget_id_t icon			= NULL_WIDGET;
		ui::widget_id_t label			= NULL_WIDGET;
		ui::widget_id_t close_button	= NULL_WIDGET;
		f32				pos_x			= 0.0f;
		f32				pos_y			= 0.0f;
		f32				velocity_x		= 0.0f;
		f32				velocity_y		= 0.0f;
		f32				marker_height	= 0.0f;
		f32				marker_velocity = 0.0f;
	};

	using editor_tab_drag_out_allowed_fn = bool (*)(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
	using editor_tab_close_allowed_fn	 = bool (*)(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
	using editor_tab_callback_fn		 = void (*)(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);

	struct editor_tab_area_config_t
	{
		editor_tab_callback_fn		   tab_switched			= nullptr;
		editor_tab_callback_fn		   tab_removed			= nullptr;
		editor_tab_callback_fn		   tab_dragged_out		= nullptr;
		editor_tab_drag_out_allowed_fn drag_out_allowed		= nullptr;
		editor_tab_close_allowed_fn	   close_allowed		= nullptr;
		void*						   user_data			= nullptr;
		bool						   can_close_single_tab = false;
		bool						   can_close			= true;
		bool						   can_drag_out			= false;
	};

	class editor_tab_area_t final
	{
	public:
		editor_tab_area_t()										   = default;
		~editor_tab_area_t()									   = default;
		editor_tab_area_t(const editor_tab_area_t&)				   = delete;
		editor_tab_area_t& operator=(const editor_tab_area_t&)	   = delete;
		editor_tab_area_t(editor_tab_area_t&&) noexcept			   = default;
		editor_tab_area_t& operator=(editor_tab_area_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_tab_area_config_t& config = {});
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void add_tab(const char* title, const char* icon = nullptr);
		void remove_tab(sid_t identifier);
		void rename_tab(sid_t identifier, const char* title);
		void select_tab(sid_t identifier);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}
		inline sid_t get_active_tab() const
		{
			return _active_tab;
		}
		bool is_over_tab(const vec2f_t& pos) const;

	private:
		void		  update_markers(f32 dt);
		void		  update_tab_positions(f32 dt);
		void		  refresh_status();
		void		  switch_tab(sid_t identifier);
		void		  reorder_dragged_tab(const vec2f_t& pos);
		void		  finish_tab_drag(const vec2f_t& pos);
		bool		  is_drag_out_allowed(sid_t identifier);
		bool		  is_drag_out_position(const vec2f_t& pos);
		void		  request_drag_out(sid_t identifier);
		void		  request_close(sid_t identifier);
		void		  remove_tab(sid_t identifier, bool notify_removed);
		bool		  consume_pending_removals();
		size_t		  find_tab_index(sid_t identifier) const;
		editor_tab_t& find_tab_by_widget(ui::widget_id_t widget);

		static void on_pre_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);
		static void draw_tab_frame(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);
		static void on_tab_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_tab_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_tab_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_tab_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_close_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		editor_tab_area_config_t _config = {};
		vector_t<editor_tab_t>	 _tabs;
		ui::ui_context*			 _ui				   = nullptr;
		sid_t					 _active_tab		   = 0;
		sid_t					 _drag_tab			   = 0;
		sid_t					 _pending_close_tab	   = 0;
		sid_t					 _pending_drag_out_tab = 0;
		vec2f_t					 _drag_offset		   = {};
		ui::widget_id_t			 _root				   = NULL_WIDGET;
	};
}
