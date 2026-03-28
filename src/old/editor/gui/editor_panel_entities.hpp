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

	class editor_panel_entities
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* b);
		void uninit();
		void draw(world& w, const struct vector2ui16& window_size);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_selected(world_handle h);
		void drop_drag(world_handle target);
		void kill_drag();
		void update_entity_name(world_handle h);
		void set_entity_collapsed(world_handle h, bool collapsed);
		void on_context_item(vekt::id id);
		void on_popup_item(vekt::id id) {};

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void kill_context()
		{
			_ctx_new_entity = NULL_WIDGET_ID;
			_ctx_add_child	= NULL_WIDGET_ID;
			_ctx_duplicate	= NULL_WIDGET_ID;
			_ctx_delete		= NULL_WIDGET_ID;
		}

		inline void kill_popup()
		{
		}

		inline vekt::id get_root() const
		{
			return _root;
		}
		inline world_handle get_selected() const
		{
			return _selected_entity;
		}

	private:
		// -----------------------------------------------------------------------------
		// callbacks
		// -----------------------------------------------------------------------------

		static vekt::input_event_result on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev);
		static void						on_tree_item_drag(vekt::builder* b, vekt::id widget, f32 mp_x, f32 mp_y, f32 delta_x, f32 delta_y, unsigned int button);
		static void						on_tree_item_hover_end(vekt::builder* b, vekt::id widget);
		static void						on_tree_item_hover_move(vekt::builder* b, vekt::id widget);
		static void						on_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void	 rebuild_tree(class world& w);
		vekt::id build_entity_node(class world& w, world_handle e, unsigned int depth);
		bool	 is_ancestor_of(world_handle ancestor, world_handle node);
		void	 set_collapse(world_handle h, u8 collapsed);
		void	 toggle_collapse(world_handle h);

	private:
		struct entity_panel_meta
		{
			bool collapsed = true;
		};

		struct node_binding
		{
			vekt::id	 root_row  = NULL_WIDGET_ID;
			vekt::id	 inner_row = NULL_WIDGET_ID;
			vekt::id	 text	   = NULL_WIDGET_ID;
			world_handle handle	   = {};
		};

	private:
		vekt::builder*			  _builder			   = nullptr;
		gui_builder				  _gui_builder		   = {};
		vekt::id				  _root				   = 0;
		vekt::id				  _entity_area		   = 0;
		vekt::id				  _ctx_new_entity	   = NULL_WIDGET_ID;
		vekt::id				  _ctx_add_child	   = NULL_WIDGET_ID;
		vekt::id				  _ctx_duplicate	   = NULL_WIDGET_ID;
		vekt::id				  _ctx_delete		   = NULL_WIDGET_ID;
		vector<entity_panel_meta> _entity_meta		   = {};
		vector<vekt::id>		  _root_entity_widgets = {};
		vector<node_binding>	  _node_bindings	   = {};

		const char* _text_icon_dd			= nullptr;
		const char* _text_icon_dd_collapsed = nullptr;
		const char* _text_icon_template		= nullptr;

		vekt::id _drag_src_widget = 0;
		bool	 _is_payload_on	  = false;
		f32	 _drag_y		  = 0.0f;

		world_handle _drag_source	  = {};
		world_handle _selected_entity = {};
	};
}
