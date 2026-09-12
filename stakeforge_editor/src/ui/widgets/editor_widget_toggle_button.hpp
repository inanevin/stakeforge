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
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	using editor_widget_toggle_button_toggled_fn = void (*)(bool is_toggled, void* user_data);

	struct editor_widget_toggle_button_config_t
	{
		vec4f_t								   frame_color			 = {};
		vec4f_t								   outline_color		 = {};
		vec4f_t								   toggled_frame_color	 = {};
		vec4f_t								   toggled_outline_color = {};
		vec4f_t								   hover_color			 = {};
		vec4f_t								   toggled_hover_color	 = {};
		vec4f_t								   pressed_color		 = {};
		vec4f_t								   text_color			 = {};
		vec4f_t								   toggled_text_color	 = {};
		const char*							   text					 = nullptr;
		const char*							   toggled_text			 = nullptr;
		editor_widget_toggle_button_toggled_fn on_toggle			 = nullptr;
		void*								   user_data			 = nullptr;
		f32									   width				 = 0.0f;
		bool								   is_toggled			 = false;
	};

	class editor_widget_toggle_button_t final
	{
	public:
		editor_widget_toggle_button_t()												   = default;
		~editor_widget_toggle_button_t()											   = default;
		editor_widget_toggle_button_t(const editor_widget_toggle_button_t&)			   = delete;
		editor_widget_toggle_button_t& operator=(const editor_widget_toggle_button_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_toggle_button_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_toggled(bool is_toggled);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline bool is_toggled() const
		{
			return _is_toggled;
		}

	private:
		void refresh();

		static void on_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		editor_widget_toggle_button_config_t _config	 = {};
		ui::ui_context*						 _ui		 = nullptr;
		ui::widget_id_t						 _root		 = NULL_WIDGET;
		ui::widget_id_t						 _label		 = NULL_WIDGET;
		bool								 _is_toggled = false;
	};
}
