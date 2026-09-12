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
	using editor_icon_button_clicked_fn = void (*)(bool toggled, void* user_data);

	struct editor_icon_button_config_t
	{
		vec4f_t						  frame_color			= {};
		vec4f_t						  outline_color			= {};
		vec4f_t						  toggled_frame_color	= {};
		vec4f_t						  toggled_outline_color = {};
		vec4f_t						  hover_color			= {};
		vec4f_t						  toggled_hover_color	= {};
		vec4f_t						  press_color			= {};
		vec4f_t						  icon_color			= {1.0f, 1.0f, 1.0f, 1.0f};
		vec4f_t						  disabled_color		= {1.0f, 1.0f, 1.0f, 1.0f};
		vec4f_t						  toggled_icon_color	= vec4f_t::zero; // zero alpha keeps icon_color
		const char*					  icon					= nullptr;
		const char*					  toggled_icon			= nullptr;
		const char*					  tooltip				= nullptr;
		editor_icon_button_clicked_fn on_clicked			= nullptr;
		void*						  user_data				= nullptr;
		f32							  size					= 0.0f;
		f32							  icon_size				= 0.0f;
		f32							  rounding				= 0.0f;
		bool						  toggle_enabled		= false;
		bool						  toggled				= false;
	};

	class editor_icon_button_t final
	{
	public:
		editor_icon_button_t()										 = default;
		~editor_icon_button_t()										 = default;
		editor_icon_button_t(const editor_icon_button_t&)			 = delete;
		editor_icon_button_t& operator=(const editor_icon_button_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_icon_button_config_t& config);
		void uninit();
		void set_toggled(bool toggled);
		void set_disabled(bool disabled);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline bool is_toggled() const
		{
			return _toggled;
		}

	private:
		void		refresh();
		const char* get_icon() const;

		static void on_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		editor_icon_button_config_t _config	  = {};
		ui::ui_context*				_ui		  = nullptr;
		ui::widget_id_t				_root	  = NULL_WIDGET;
		ui::widget_id_t				_icon	  = NULL_WIDGET;
		bool						_toggled  = false;
		bool						_disabled = false;
	};
}
