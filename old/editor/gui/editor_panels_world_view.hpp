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

#include "gui/vekt_defines.hpp"
#include "editor/gui/editor_gui_builder.hpp"
#include "editor/gui/editor_gui_user_data.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"

namespace SFG
{
	struct vector2ui16;
	struct window_event;

	class editor_panels_world_view
	{
	public:
		enum class gizmo_mode
		{
			position,
			rotation,
			scale,
		};

		enum class gizmo_axis
		{
			none,
			x,
			y,
			z,
		};

		enum class gizmo_space
		{
			global,
			local,
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* b);
		void uninit();
		void draw(const vector2ui16& window_size);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool on_mouse_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline vekt::id get_root() const
		{
			return _root;
		}

		inline void kill_context()
		{
			_ctx_world_rt		 = NULL_WIDGET_ID;
			_ctx_colors_rt		 = NULL_WIDGET_ID;
			_ctx_normals_rt		 = NULL_WIDGET_ID;
			_ctx_orm_rt			 = NULL_WIDGET_ID;
			_ctx_emissive_rt	 = NULL_WIDGET_ID;
			_ctx_depth_rt		 = NULL_WIDGET_ID;
			_ctx_lighting_rt	 = NULL_WIDGET_ID;
			_ctx_bloom_rt		 = NULL_WIDGET_ID;
			_ctx_ssao_rt		 = NULL_WIDGET_ID;
			_ctx_new_project	 = NULL_WIDGET_ID;
			_ctx_open_project	 = NULL_WIDGET_ID;
			_ctx_package_project = NULL_WIDGET_ID;
			_ctx_new_world		 = NULL_WIDGET_ID;
			_ctx_save_world		 = NULL_WIDGET_ID;
			_ctx_open_world		 = NULL_WIDGET_ID;
		}

		inline void kill_popup()
		{
			_popup_save_yes = NULL_WIDGET_ID;
			_popup_save_no	= NULL_WIDGET_ID;
			_popup_action	= popup_action::none;
		}
		static vekt::input_event_result on_widget_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev);
		static void						on_widget_draw(vekt::builder* b, vekt::id widget);

	private:
		enum class popup_action
		{
			none,
			new_world,
			open_world,
		};

		void update_gizmo_buttons();
		void update_toggle_button(vekt::id button, bool enabled);
		void open_save_popup(popup_action action);

		gui_builder	   _gui_builder = {};
		vekt::builder* _builder		= nullptr;
		vekt::id	   _root		= NULL_WIDGET_ID;

		editor_gui_user_data _user_data = {};

		// Icon column and buttons
		vekt::id _icon_column			= NULL_WIDGET_ID;
		vekt::id _icon_row				= NULL_WIDGET_ID;
		vekt::id _btn_view				= NULL_WIDGET_ID;
		vekt::id _btn_menu				= NULL_WIDGET_ID;
		vekt::id _btn_stats				= NULL_WIDGET_ID;
		vekt::id _btn_play				= NULL_WIDGET_ID;
		vekt::id _btn_gizmo_translation = NULL_WIDGET_ID;
		vekt::id _btn_gizmo_rotation	= NULL_WIDGET_ID;
		vekt::id _btn_gizmo_scale		= NULL_WIDGET_ID;
		vekt::id _btn_audio_on			= NULL_WIDGET_ID;
		vekt::id _btn_physics_debug_on	= NULL_WIDGET_ID;

		vekt::id _ctx_world_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_colors_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_normals_rt  = NULL_WIDGET_ID;
		vekt::id _ctx_orm_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_emissive_rt = NULL_WIDGET_ID;
		vekt::id _ctx_depth_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_lighting_rt = NULL_WIDGET_ID;
		vekt::id _ctx_bloom_rt	  = NULL_WIDGET_ID;
		vekt::id _ctx_ssao_rt	  = NULL_WIDGET_ID;

		// Context menu items
		vekt::id _ctx_new_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_open_project	  = NULL_WIDGET_ID;
		vekt::id _ctx_package_project = NULL_WIDGET_ID;
		vekt::id _ctx_new_world		  = NULL_WIDGET_ID;
		vekt::id _ctx_save_world	  = NULL_WIDGET_ID;
		vekt::id _ctx_open_world	  = NULL_WIDGET_ID;
		vekt::id _popup_save_yes	  = NULL_WIDGET_ID;
		vekt::id _popup_save_no		  = NULL_WIDGET_ID;

		// Stats area and texts
		vekt::id _stats_area		= NULL_WIDGET_ID;
		vekt::id _wv_game_res		= NULL_WIDGET_ID;
		vekt::id _wv_window_res		= NULL_WIDGET_ID;
		vekt::id _wv_fps			= NULL_WIDGET_ID;
		vekt::id _wv_main			= NULL_WIDGET_ID;
		vekt::id _wv_render			= NULL_WIDGET_ID;
		vekt::id _wv_ram			= NULL_WIDGET_ID;
		vekt::id _wv_vram			= NULL_WIDGET_ID;
		vekt::id _wv_vram_txt		= NULL_WIDGET_ID;
		vekt::id _wv_vram_res		= NULL_WIDGET_ID;
		vekt::id _wv_draw_calls		= NULL_WIDGET_ID;
		vekt::id _wv_loaded_project = NULL_WIDGET_ID;
		vekt::id _wv_loaded_level	= NULL_WIDGET_ID;

		gizmo_mode	 _gizmo_mode			  = gizmo_mode::position;
		gizmo_axis	 _gizmo_active_axis		  = gizmo_axis::none;
		gizmo_axis	 _gizmo_hover_axis		  = gizmo_axis::none;
		gizmo_space	 _gizmo_space			  = gizmo_space::global;
		bool		 _gizmo_center_hover	  = false;
		bool		 _gizmo_center_handle	  = false;
		bool		 _audio_enabled			  = false;
		bool		 _physics_debug_enabled	  = false;
		bool		 _gizmo_dragging		  = false;
		f32			 _gizmo_drag_amount		  = 0.0f;
		vector2		 _gizmo_last_mouse		  = vector2::zero;
		vector3		 _gizmo_start_pos		  = vector3::zero;
		quat		 _gizmo_start_rot		  = quat::identity;
		vector3		 _gizmo_start_scale		  = vector3::one;
		vector3		 _gizmo_drag_offset		  = vector3::zero;
		vector2		 _gizmo_angle_start_dir	  = vector2::zero;
		vector2		 _gizmo_angle_current_dir = vector2::zero;
		popup_action _popup_action			  = popup_action::none;
	};
}
