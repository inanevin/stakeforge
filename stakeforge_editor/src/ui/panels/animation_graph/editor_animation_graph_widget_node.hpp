/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include <sfg/runtime/resources/animation_graph_def.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
}

namespace sfg
{
	class editor_animation_graph_widget_node_t final
	{
	public:
		struct config_t
		{
			const char* title = nullptr;
			u32			id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		};

		editor_animation_graph_widget_node_t()														 = default;
		~editor_animation_graph_widget_node_t()														 = default;
		editor_animation_graph_widget_node_t(const editor_animation_graph_widget_node_t&)			 = delete;
		editor_animation_graph_widget_node_t& operator=(const editor_animation_graph_widget_node_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_zoom(f32 zoom);
		void set_selected(bool selected);
		void set_start_state(bool start_state);
		void update_title(const char* title);
		void make_entry();
		void make_exit();
		void make_entry_and_exit();
		void clear_entry_and_exit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline u32 get_id() const
		{
			return _id;
		}

		inline ui::widget_id_t get_pin_frame() const
		{
			return _pin_frame;
		}

	private:
		void set_entry_and_exit(bool entry, bool exit);
		void update_title_paint();
		void update_status_paint();

		static void draw_pin(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context* _ui				   = nullptr;
		vec2f_t			_base_size		   = vec2f_t::zero;
		ui::widget_id_t _root			   = NULL_WIDGET;
		ui::widget_id_t _title_frame	   = NULL_WIDGET;
		ui::widget_id_t _title			   = NULL_WIDGET;
		ui::widget_id_t _body_frame		   = NULL_WIDGET;
		ui::widget_id_t _entry_label	   = NULL_WIDGET;
		ui::widget_id_t _exit_label		   = NULL_WIDGET;
		ui::widget_id_t _pin_frame		   = NULL_WIDGET;
		f32				_base_title_height = 0.0f;
		f32				_base_pin_size	   = 0.0f;
		f32				_zoom			   = 1.0f;
		u32				_id				   = ANIMATION_GRAPH_DEF_NULL_ID;
		bool			_is_entry		   = false;
		bool			_is_exit		   = false;
		bool			_is_start_state	   = false;
	};
}
