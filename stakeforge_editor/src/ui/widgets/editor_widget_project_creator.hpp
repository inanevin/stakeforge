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

#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	namespace ui
	{
		struct input_router_t;
		enum class mouse_button_e : u8;
	}

	struct editor_widget_project_create_config_t
	{
		string_t directory;
		string_t name;
	};

	struct editor_widget_project_load_config_t
	{
		string_t path;
	};

	SFG_DEFINE_TYPE_ID(editor_widget_project_create_config_t);
	SFG_DEFINE_TYPE_ID(editor_widget_project_load_config_t);

	struct editor_widget_project_create_config_reflection_t
	{
		editor_widget_project_create_config_reflection_t();
	};

	struct editor_widget_project_load_config_reflection_t
	{
		editor_widget_project_load_config_reflection_t();
	};

	inline editor_widget_project_create_config_reflection_t g_reflect_editor_widget_project_create_config;
	inline editor_widget_project_load_config_reflection_t	g_reflect_editor_widget_project_load_config;

	using editor_widget_project_ready_fn = void (*)(void* user_data);

	struct editor_widget_project_creator_config_t
	{
		editor_widget_project_ready_fn on_project_ready = nullptr;
		void*						   user_data		= nullptr;
	};

	class editor_widget_project_creator_t final
	{
	public:
		editor_widget_project_creator_t()												   = default;
		~editor_widget_project_creator_t()												   = default;
		editor_widget_project_creator_t(const editor_widget_project_creator_t&)			   = delete;
		editor_widget_project_creator_t& operator=(const editor_widget_project_creator_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_project_creator_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static void on_load_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_create_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

		void show_error(const char* text);
		void hide_error();
		void notify_project_ready();
		void try_load();
		void try_create();

		editor_widget_project_creator_config_t _creator_config	   = {};
		editor_widget_project_load_config_t	   _load_config		   = {};
		editor_widget_project_create_config_t  _create_config	   = {};
		editor_widget_reflection_t			   _load_reflection	   = {};
		editor_widget_reflection_t			   _create_reflection  = {};
		editor_scrollbar_t					   _scrollbar		   = {};
		editor_widget_button_t				   _load_button		   = {};
		editor_widget_button_t				   _create_button	   = {};
		ui::ui_context*						   _ui				   = nullptr;
		ui::widget_id_t						   _root			   = NULL_WIDGET;
		ui::widget_id_t						   _inner_root		   = NULL_WIDGET;
		ui::widget_id_t						   _scroll_area		   = NULL_WIDGET;
		ui::widget_id_t						   _wrapper			   = NULL_WIDGET;
		ui::widget_id_t						   _load_description   = NULL_WIDGET;
		ui::widget_id_t						   _create_description = NULL_WIDGET;
		ui::widget_id_t						   _error_frame		   = NULL_WIDGET;
		ui::widget_id_t						   _error_label		   = NULL_WIDGET;
	};
}
