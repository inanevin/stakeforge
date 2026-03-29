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
#include "world/world_constants.hpp"
#include "data/vector.hpp"

namespace vekt
{
	class builder;
};

namespace SFG
{
	class world;
	class editor_panel_entities;

	struct vector2ui16;

	class editor_panel_inspector
	{
	public:
		enum class debug_draw_type
		{
			physics,
			character_controller,
			point_light,
			spot_light,
			audio,
			camera,
		};

		void init(vekt::builder* b, editor_panel_entities* entities);
		void uninit();
		void draw(world& w, const vector2ui16& window_size);
		void on_context_item(vekt::id widget);
		void on_popup_item(vekt::id widget);

		inline void kill_context()
		{
			_add_component_buttons.resize(0);
		}

		inline void kill_popup()
		{
		}

		inline vekt::id get_root() const
		{
			return _root;
		}

	private:
		static void						on_input_field_changed(void* callback_ud, vekt::builder* b, vekt::id id, const char* txt, f32 value);
		static void						on_checkbox(void* callback_ud, vekt::builder* b, vekt::id id, unsigned char value);
		static void						invoke_button(void* callback_ud, void* object_ptr, string_id type_id, string_id button_id);
		static vekt::input_event_result on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		bool							is_component_collapsed(string_id type_id) const;
		void							set_component_collapsed(string_id type_id, bool collapsed);
		void							clear_component_view();
		void							build_component_view();
		void							set_selected_controls();
		void							sync_selected(world& w);
		void							open_info_popup(const char* text);

	private:
		struct comp_header_button
		{
			world_handle handle		   = {};
			vekt::id	 collapse_row  = NULL_WIDGET_ID;
			vekt::id	 reset_button  = NULL_WIDGET_ID;
			vekt::id	 remove_button = NULL_WIDGET_ID;
			string_id	 comp_type	   = 0;
		};

		struct add_comp_button
		{
			vekt::id  button = NULL_WIDGET_ID;
			string_id type	 = 0;
		};

		struct selection_debug_draw
		{
			debug_draw_type type	  = debug_draw_type::physics;
			world_handle	component = {};
		};

		struct component_collapse_state
		{
			string_id comp_type = 0;
			bool	  collapsed = false;
		};

	private:
		vekt::builder*		   _builder		= nullptr;
		editor_panel_entities* _entities	= nullptr;
		gui_builder			   _gui_builder = {};
		vekt::id			   _root		= 0;

		vekt::id _prop_name			= NULL_WIDGET_ID;
		vekt::id _prop_tag			= NULL_WIDGET_ID;
		vekt::id _prop_vis			= NULL_WIDGET_ID;
		vekt::id _prop_handle		= NULL_WIDGET_ID;
		vekt::id _selected_pos_x	= NULL_WIDGET_ID;
		vekt::id _selected_pos_y	= NULL_WIDGET_ID;
		vekt::id _selected_pos_z	= NULL_WIDGET_ID;
		vekt::id _selected_rot_x	= NULL_WIDGET_ID;
		vekt::id _selected_rot_y	= NULL_WIDGET_ID;
		vekt::id _selected_rot_z	= NULL_WIDGET_ID;
		vekt::id _selected_scale_x	= NULL_WIDGET_ID;
		vekt::id _selected_scale_y	= NULL_WIDGET_ID;
		vekt::id _selected_scale_z	= NULL_WIDGET_ID;
		vekt::id _components_parent = NULL_WIDGET_ID;
		vekt::id _components_area	= NULL_WIDGET_ID;
		vekt::id _add_component		= NULL_WIDGET_ID;
		vekt::id _unlock_template	= NULL_WIDGET_ID;
		vekt::id _save_template		= NULL_WIDGET_ID;

		vector<add_comp_button>			 _add_component_buttons		= {};
		vector<comp_header_button>		 _comp_header_buttons		= {};
		vector<component_collapse_state> _component_collapse_states = {};
		vector<selection_debug_draw>	 _selection_debug_draws		= {};

		world_handle _selected_entity = {};
	};
}
