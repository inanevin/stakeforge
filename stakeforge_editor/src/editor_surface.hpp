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
