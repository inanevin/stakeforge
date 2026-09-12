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
