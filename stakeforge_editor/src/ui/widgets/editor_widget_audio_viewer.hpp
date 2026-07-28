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
#include "ui/widgets/editor_widget_slider.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"

#include <sfg/audio/audio_engine.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_widget_audio_viewer_t final
	{
	public:
		editor_widget_audio_viewer_t()												 = default;
		~editor_widget_audio_viewer_t()												 = default;
		editor_widget_audio_viewer_t(const editor_widget_audio_viewer_t&)			 = delete;
		editor_widget_audio_viewer_t& operator=(const editor_widget_audio_viewer_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();
		void set_audio(sid_t audio_id);
		void clear_audio();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void start_preview();
		void stop_preview();
		void submit_config_edit();

		static void on_play_clicked(bool toggled, void* user_data);
		static void on_reset_clicked(bool toggled, void* user_data);
		static void on_config_edit_begin(void* user_data);
		static void on_config_edit_submitted(void* user_data);
		static void on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		ui::ui_context*			   _ui					  = nullptr;
		ui::widget_id_t			   _root				  = NULL_WIDGET;
		ui::widget_id_t			   _transport			  = NULL_WIDGET;
		editor_widget_reflection_t _reflection			  = {};
		editor_icon_button_t	   _play_button			  = {};
		editor_icon_button_t	   _reset_button		  = {};
		editor_slider_t			   _scrub_slider		  = {};
		audio_voice_handle_t	   _voice				  = {};
		audio_cook_config_t		   _config				  = {};
		audio_cook_config_t		   _previous_config		  = {};
		sid_t					   _audio_id			  = NULL_SID;
		f32						   _duration_seconds	  = 0.0f;
		f32						   _scrub_ratio			  = 0.0f;
		f32						   _displayed_scrub_ratio = 0.0f;
		f32*					   _scrub_field			  = nullptr;
		bool					   _resource_loaded		  = false;
	};
}
