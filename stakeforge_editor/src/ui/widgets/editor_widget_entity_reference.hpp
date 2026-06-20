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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	using editor_widget_entity_reference_selected_fn = entity_id_t (*)(void* user_data);
	using editor_widget_entity_reference_pressed_fn	 = void (*)(entity_id_t entity, void* user_data);

	struct editor_widget_entity_reference_config_t
	{
		editor_widget_entity_reference_selected_fn selected	 = nullptr;
		editor_widget_entity_reference_pressed_fn  pressed	 = nullptr;
		void*									   user_data = nullptr;
		world_handle_t							   world	 = {};
	};

	class editor_widget_entity_reference_t final
	{
	public:
		editor_widget_entity_reference_t()													 = default;
		~editor_widget_entity_reference_t()													 = default;
		editor_widget_entity_reference_t(const editor_widget_entity_reference_t&)			 = delete;
		editor_widget_entity_reference_t& operator=(const editor_widget_entity_reference_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_entity_reference_config_t& config);
		void uninit();
		void refresh_title();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		entity_id_t get_selected() const;

		static void on_root_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_popup_entity_pressed(entity_id_t entity, void* user_data);

	private:
		ui::ui_context*							_ui		= nullptr;
		ui::widget_id_t							_root	= NULL_WIDGET;
		ui::widget_id_t							_label	= NULL_WIDGET;
		editor_widget_entity_reference_config_t _config = {};
	};
}
