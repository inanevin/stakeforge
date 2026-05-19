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

#include "ui/docking/dock_area.hpp"
#include <sfg/math/vec2i16.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
}

namespace sfg
{
	struct editor_payload_t;
	struct window_runtime_t;

	enum class dock_widget_root_drag_out_e : u8
	{
		disabled,
		close_window,
	};

	struct dock_widget_config_t
	{
		window_runtime_t*			runtime				   = nullptr;
		dock_widget_root_drag_out_e root_drag_out_behavior = dock_widget_root_drag_out_e::disabled;
	};

	class dock_widget_t final
	{
	public:
		dock_widget_t()									   = default;
		~dock_widget_t()								   = default;
		dock_widget_t(const dock_widget_t&)				   = delete;
		dock_widget_t& operator=(const dock_widget_t&)	   = delete;
		dock_widget_t(dock_widget_t&&) noexcept			   = default;
		dock_widget_t& operator=(dock_widget_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const dock_widget_config_t& config = {});
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		dock_node_handle_t create_leaf_node(ui::widget_id_t parent);
		void			   set_root_node(dock_node_handle_t handle);
		void			   dock_node_add_panel(dock_node_handle_t handle, editor_panel_t* panel);
		nlohmann::json	   to_json() const;
		bool			   from_json(const nlohmann::json& j);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool is_window_drag_region(const vec2i16_t& pos) const;

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static constexpr u32 DOCK_POOL_INITIAL_CAPACITY = 32;
		static constexpr u32 DOCK_PREVIEW_COUNT			= static_cast<u32>(dock_preview_e::none);

		void				 init_leaf_node_content(dock_node_t& node);
		dock_node_handle_t	 create_split_node(ui::widget_id_t parent, dock_split_direction_e direction, f32 split_value);
		void				 dock_node_add_panel(dock_node_t& node, editor_panel_t* panel);
		void				 dock_node_remove_panel(dock_node_t& node, sid_t identifier);
		void				 destroy_dock_node(dock_node_handle_t handle);
		void				 set_leaf_active_panel(dock_node_t& node, sid_t active_tab);
		void				 update_dock_previews(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos);
		void				 clear_dock_previews();
		void				 update_leaf_dock_previews(dock_node_t& node, const vec2f_t& mouse);
		void				 collapse_empty_leaf_after_drag_out(dock_node_t& node);
		dock_node_handle_t	 find_node_handle(const dock_node_t& node) const;
		bool				 find_parent_split(dock_node_handle_t child, dock_node_handle_t& out_parent, bool& out_is_negative) const;
		void				 init_split_border(dock_node_t& split_node, dock_node_handle_t split_handle);
		void				 destroy_split_border(dock_node_t& split_node);
		void				 apply_split_border_drag(dock_border_t& border, const vec2f_t& pos);
		void				 configure_split_child_layout(dock_node_t& split_node);
		bool				 can_split_leaf(const dock_node_t& node, dock_preview_e preview) const;
		bool				 split_leaf_node(dock_node_handle_t handle, dock_preview_e preview, editor_panel_t* panel);
		nlohmann::json		 dock_node_to_json(const dock_node_t& node) const;
		dock_node_handle_t	 dock_node_from_json(ui::widget_id_t parent, const nlohmann::json& j);
		dock_node_t*		 find_leaf_at(const vec2f_t& mouse);
		const dock_node_t*	 find_node_by_widget(ui::widget_id_t widget) const;
		dock_border_t*		 find_border_by_widget(ui::widget_id_t widget);
		bool				 apply_payload_to_preview(dock_node_handle_t handle, dock_preview_e preview, editor_panel_t* panel);
		bool				 apply_payload_to_split_preview(dock_node_handle_t handle, dock_preview_e preview, editor_panel_t* panel);
		dock_node_handle_t	 alloc_dock_node();
		void				 free_dock_node(dock_node_handle_t handle);
		dock_border_handle_t alloc_dock_border();
		void				 free_dock_border(dock_border_handle_t handle);

		static void on_leaf_tab_switched(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
		static void on_leaf_tab_dragged_out(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
		static void on_leaf_tab_closed(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
		static bool is_leaf_tab_drag_out_allowed(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
		static bool is_leaf_tab_close_allowed(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);
		static void on_payload_tick(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos, void* user_data);
		static void on_payload_end(const editor_payload_t& payload, void* user_data);
		static void on_split_border_hover_enter(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_split_border_hover_exit(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_split_border_hover_move(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_split_border_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_split_border_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_split_border_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void draw_split_border(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);
		static void draw_leaf_dock_previews(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context*											  _ui					= nullptr;
		window_runtime_t*										  _runtime				= nullptr;
		ui::widget_id_t											  _root					= NULL_WIDGET;
		dock_node_handle_t										  _root_node			= {};
		dock_widget_config_t									  _config				= {};
		dynamic_gen_pool_t<dock_node_t, u16, dock_node_tag_t>	  _dock_nodes			= {};
		dynamic_gen_pool_t<dock_border_t, u16, dock_border_tag_t> _dock_borders			= {};
		bool													  _panel_payload_active = false;
	};
}
