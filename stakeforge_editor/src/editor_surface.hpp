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

#include <sfg/data/unique.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	class editor_action_menu_controller_t;
	class editor_modal_controller_t;
	class editor_popup_controller_t;
	class editor_primary_base_t;
	class editor_widget_project_creator_t;
	class editor_widget_window_frame_t;
	class editor_secondary_base_t;
	class editor_splash_screen_t;
	class editor_tooltip_controller_t;
	struct window_runtime_t;
	namespace ui
	{
		class ui_context;
	}

	enum class editor_surface_type_e : u8
	{
		primary,
		secondary,
		payload,
		splash,
		project_creator,
	};

	struct editor_surface_t
	{
		editor_surface_t();
		~editor_surface_t();
		editor_surface_t(const editor_surface_t&)			 = delete;
		editor_surface_t& operator=(const editor_surface_t&) = delete;
		editor_surface_t(editor_surface_t&& other) noexcept;
		editor_surface_t& operator=(editor_surface_t&& other) noexcept;

		unique_t<editor_primary_base_t>			  primary;
		unique_t<editor_secondary_base_t>		  secondary;
		unique_t<editor_splash_screen_t>		  splash;
		unique_t<editor_widget_project_creator_t> project_creator;
		unique_t<editor_widget_window_frame_t>	  window_frame;
		unique_t<editor_action_menu_controller_t> action_menu_controller;
		unique_t<editor_modal_controller_t>		  modal_controller;
		unique_t<editor_popup_controller_t>		  popup_controller;
		unique_t<editor_tooltip_controller_t>	  tooltip_controller;
		unique_t<window_runtime_t>				  runtime;
		unique_t<ui::ui_context>				  ui;
		gfx_handle_t							  swapchain		 = {};
		vec2u16_t								  swapchain_size = {};
		ui::widget_id_t							  root			 = NULL_WIDGET;
		ui::widget_id_t							  content_root	 = NULL_WIDGET;
		ui::widget_id_t							  owner_root	 = NULL_WIDGET;
		ui::widget_id_t							  payload_root	 = NULL_WIDGET;
		ui::widget_id_t							  payload_text	 = NULL_WIDGET;
		editor_surface_type_e					  type			 = editor_surface_type_e::secondary;
		bool									  is_minimized	 = false;
		bool									  is_hidden		 = false;
	};
}
