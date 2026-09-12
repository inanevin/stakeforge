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
	struct input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct vec2i16_t;
	struct window_runtime_t;

	struct editor_widget_window_frame_config_t
	{
		window_runtime_t* runtime	 = nullptr;
		const char*		  title		 = "Stakeforge";
		bool			  only_close = false;
	};

	class editor_widget_window_frame_t final
	{
	public:
		editor_widget_window_frame_t()												 = default;
		~editor_widget_window_frame_t()												 = default;
		editor_widget_window_frame_t(const editor_widget_window_frame_t&)			 = delete;
		editor_widget_window_frame_t& operator=(const editor_widget_window_frame_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_window_frame_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool is_window_drag_region(const vec2i16_t& pos) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline ui::widget_id_t get_title_frame() const
		{
			return _root;
		}

		inline ui::widget_id_t get_window_buttons() const
		{
			return _window_buttons;
		}

	private:
		static void on_minimize_window(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_maximize_window(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_close_window(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		editor_widget_window_frame_config_t _config			= {};
		ui::ui_context*						_ui				= nullptr;
		ui::widget_id_t						_root			= NULL_WIDGET;
		ui::widget_id_t						_window_buttons = NULL_WIDGET;
		ui::widget_id_t						_minimize_frame = NULL_WIDGET;
		ui::widget_id_t						_maximize_frame = NULL_WIDGET;
		ui::widget_id_t						_close_frame	= NULL_WIDGET;
	};
}
