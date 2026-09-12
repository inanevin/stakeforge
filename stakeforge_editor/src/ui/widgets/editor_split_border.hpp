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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
}

namespace sfg
{
	enum class editor_split_border_direction_e : u8
	{
		horizontal,
		vertical,
	};

	struct vec2f_t;

	class editor_split_border_t final
	{
	public:
		using drag_fn = void (*)(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

		struct config_t
		{
			drag_fn							on_drag	  = nullptr;
			void*							user_data = nullptr;
			editor_split_border_direction_e direction = editor_split_border_direction_e::horizontal;
		};

		editor_split_border_t()										   = default;
		~editor_split_border_t()									   = default;
		editor_split_border_t(const editor_split_border_t&)			   = delete;
		editor_split_border_t& operator=(const editor_split_border_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline bool is_dragging() const
		{
			return _is_dragging;
		}

		inline editor_split_border_direction_e get_direction() const
		{
			return _config.direction;
		}

	private:
		void apply_drag(const vec2f_t& pos, const vec2f_t& delta);
		void set_resize_cursor() const;

		static void on_hover_enter(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_hover_exit(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_hover_move(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context* _ui			 = nullptr;
		ui::widget_id_t _root		 = NULL_WIDGET;
		config_t		_config		 = {};
		bool			_is_dragging = false;
	};
}
