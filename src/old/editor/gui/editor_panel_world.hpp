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

#include "editor/gui/editor_gui_builder.hpp"
#include "editor/gui/editor_gui_user_data.hpp"
#include "editor/gui/editor_gizmo_controls.hpp"
#include "editor/gui/editor_gizmo_2d.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace vekt
{
	class builder;
};

namespace SFG
{
	struct vector2ui16;
	struct window_event;

	class editor_panel_world
	{
	public:
		enum class aspect_ratio
		{
			native,
			aspect_16_9,
			aspect_4_3,
			aspect_1_1,
		};

		enum class audio_style
		{
			on,
			mute
		};

		enum class physics_debug_style
		{
			none,
			on,
		};

		enum class stats_view_style
		{
			none,
			full,
		};

		enum class playmode
		{
			none,
			playing,
			physics,
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void			  init(vekt::builder* b);
		void			  uninit();
		void			  draw(const vector2ui16& window_size);
		bool			  on_mouse_move(const vector2& p);
		bool			  on_key_event(const window_event& ev);
		const vector2ui16 get_world_size() const;
		void			  fetch_stats();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_gizmo_style(gizmo_style style);
		void set_gizmo_space(gizmo_space space);
		void set_aspect(aspect_ratio aspect);
		void set_physics_debug(physics_debug_style style);
		void set_audio_style(audio_style aud);
		void set_stats_view(stats_view_style style);
		void set_playmode(playmode m);
		void on_context_item(vekt::id widget);
		void on_popup_item(vekt::id widget);

		static vekt::input_event_result on_widget_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static void						on_toggle_button(void* callback_ud, vekt::builder* b, vekt::id id, bool toggled);
		static void						on_widget_draw(vekt::builder* b, vekt::id widget);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void kill_context()
		{
			_ctx_aspect_native	 = NULL_WIDGET_ID;
			_ctx_aspect_169		 = NULL_WIDGET_ID;
			_ctx_aspect_43		 = NULL_WIDGET_ID;
			_ctx_aspect_11		 = NULL_WIDGET_ID;
			_ctx_view_rt		 = NULL_WIDGET_ID;
			_ctx_view_albedo	 = NULL_WIDGET_ID;
			_ctx_view_normals	 = NULL_WIDGET_ID;
			_ctx_view_emissive	 = NULL_WIDGET_ID;
			_ctx_view_orm		 = NULL_WIDGET_ID;
			_ctx_view_depth		 = NULL_WIDGET_ID;
			_ctx_view_lighting	 = NULL_WIDGET_ID;
			_ctx_view_ssao		 = NULL_WIDGET_ID;
			_ctx_view_bloom		 = NULL_WIDGET_ID;
			_ctx_new_project	 = NULL_WIDGET_ID;
			_ctx_open_project	 = NULL_WIDGET_ID;
			_ctx_save_project	 = NULL_WIDGET_ID;
			_ctx_package_project = NULL_WIDGET_ID;
			_ctx_new_world		 = NULL_WIDGET_ID;
			_ctx_save_world		 = NULL_WIDGET_ID;
			_ctx_load_world		 = NULL_WIDGET_ID;
		}

		inline void kill_popup()
		{
			_popup_save_yes = NULL_WIDGET_ID;
			_popup_save_no	= NULL_WIDGET_ID;
			_popup_action	= popup_action::none;
		}

		inline vekt::id get_root() const
		{
			return _gui_builder.get_root();
		}

		inline vekt::id get_game_view() const
		{
			return _world_viewer;
		}
		inline void kill_playmode()
		{
			set_playmode(playmode::none);
		}

	private:
		enum class popup_action
		{
			none,
			new_world,
			load_world,
		};

		void open_save_popup(popup_action action);
		void build_toolbar();
		void destroy_toolbar();

		void draw_world_axes();

	private:
		gui_builder			  _gui_builder	  = {};
		vekt::builder*		  _builder		  = nullptr;
		editor_gui_user_data  _user_data	  = {};
		editor_gizmo_controls _gizmo_controls = {};
		editor_gizmo_2d		  _gizmo_2d		  = {};

		vekt::id _world_viewer		= NULL_WIDGET_ID;
		vekt::id _toolbar_row		= NULL_WIDGET_ID;
		vekt::id _btn_file			= NULL_WIDGET_ID;
		vekt::id _btn_stats			= NULL_WIDGET_ID;
		vekt::id _btn_translate		= NULL_WIDGET_ID;
		vekt::id _btn_rotate		= NULL_WIDGET_ID;
		vekt::id _btn_scale			= NULL_WIDGET_ID;
		vekt::id _btn_space			= NULL_WIDGET_ID;
		vekt::id _btn_mute			= NULL_WIDGET_ID;
		vekt::id _btn_physics_debug = NULL_WIDGET_ID;
		vekt::id _btn_aspect		= NULL_WIDGET_ID;
		vekt::id _btn_world_view	= NULL_WIDGET_ID;
		vekt::id _btn_play			= NULL_WIDGET_ID;

		vekt::id _ctx_aspect_native	  = NULL_WIDGET_ID;
		vekt::id _ctx_aspect_169	  = NULL_WIDGET_ID;
		vekt::id _ctx_aspect_43		  = NULL_WIDGET_ID;
		vekt::id _ctx_aspect_11		  = NULL_WIDGET_ID;
		vekt::id _ctx_view_rt		  = NULL_WIDGET_ID;
		vekt::id _ctx_view_albedo	  = NULL_WIDGET_ID;
		vekt::id _ctx_view_normals	  = NULL_WIDGET_ID;
		vekt::id _ctx_view_emissive	  = NULL_WIDGET_ID;
		vekt::id _ctx_view_orm		  = NULL_WIDGET_ID;
		vekt::id _ctx_view_depth	  = NULL_WIDGET_ID;
		vekt::id _ctx_view_lighting	  = NULL_WIDGET_ID;
		vekt::id _ctx_view_ssao		  = NULL_WIDGET_ID;
		vekt::id _ctx_view_bloom	  = NULL_WIDGET_ID;
		vekt::id _ctx_new_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_open_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_save_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_package_project = NULL_WIDGET_ID;
		vekt::id _ctx_new_world		  = NULL_WIDGET_ID;
		vekt::id _ctx_save_world	  = NULL_WIDGET_ID;
		vekt::id _ctx_load_world	  = NULL_WIDGET_ID;
		vekt::id _popup_save_yes	  = NULL_WIDGET_ID;
		vekt::id _popup_save_no		  = NULL_WIDGET_ID;

		playmode			_play_mode	   = playmode::none;
		aspect_ratio		_aspect		   = aspect_ratio::native;
		audio_style			_audio_style   = audio_style::on;
		physics_debug_style _physics_style = physics_debug_style::none;
		stats_view_style	_stats_style   = stats_view_style::none;
		popup_action		_popup_action  = popup_action::none;
	};
}
