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

#include "editor_panel_entities.hpp"
#include "editor/editor_theme.hpp"
#include "editor/editor.hpp"

// world
#include "world/world.hpp"
#include "world/entity_manager.hpp"

// math
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"

// misc
#include "gui/vekt.hpp"
#include "gui/icon_defs.hpp"
#include "input/input_mappings.hpp"
#include "app/app.hpp"
#include "platform/window.hpp"

namespace SFG
{
	void editor_panel_entities::init(vekt::builder* b)
	{
		_builder = b;

		_gui_builder.init(b);
		_root = _gui_builder.get_root();

		_gui_builder.callbacks.user_data   = this;
		_gui_builder.callbacks.callback_ud = this;
		_gui_builder.callbacks.on_mouse	   = on_mouse;

		_gui_builder.add_title("entities");
		_entity_area = _gui_builder.begin_area(true);
		{
			vekt::size_props& sz										= _builder->widget_get_size_props(_entity_area);
			sz.flags													= vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_fill;
			sz.size.x													= 1.0f;
			sz.child_margins.left										= 0.0f;
			sz.spacing													= 0.0f;
			_builder->widget_get_mouse_callbacks(_entity_area).on_mouse = on_mouse;
		}
		_gui_builder.end_area();

		// Reserve per-entity meta storage
		_entity_meta.resize(MAX_ENTITIES);
		_node_bindings.reserve(512);
		_root_entity_widgets.reserve(512);
		_text_icon_dd_collapsed = editor::get().get_text_allocator().allocate(ICON_DD_RIGHT);
		_text_icon_dd			= editor::get().get_text_allocator().allocate(ICON_DD_DOWN);
		_text_icon_template		= editor::get().get_text_allocator().allocate(ICON_HAMMER);
	}

	void editor_panel_entities::uninit()
	{
		editor::get().get_text_allocator().deallocate(_text_icon_dd);
		editor::get().get_text_allocator().deallocate(_text_icon_dd_collapsed);
		editor::get().get_text_allocator().deallocate(_text_icon_template);
		_gui_builder.uninit();
	}

	void editor_panel_entities::draw(world& w, const vector2ui16& window_size)
	{
		(void)window_size;

		entity_manager& em = w.get_entity_manager();
		if (em.get_hierarchy_dirty())
		{
			rebuild_tree(w);
			em.set_hierarchy_dirty(0);
		}
	}

	void editor_panel_entities::rebuild_tree(world& w)
	{
		const f32 old_scroll = _builder->widget_get_pos_props(_entity_area).scroll_offset;

		// Remove old nodes
		for (vekt::id nid : _root_entity_widgets)
		{
			_builder->deallocate(nid);
		}
		_root_entity_widgets.resize(0);
		_node_bindings.resize(0);

		_drag_source	 = {};
		_drag_src_widget = NULL_WIDGET_ID;

		entity_manager& em = w.get_entity_manager();
		// Build from roots
		for (auto it = em.get_entities()->handles_begin(); it != em.get_entities()->handles_end(); ++it)
		{
			world_handle		 h			  = *it;
			const entity_family& fam		  = em.get_entity_family(h);
			const bool			 is_transient = em.get_entity_flags(h).is_set(entity_flags::entity_flags_no_save);

			if (fam.parent.is_null() && !is_transient)
			{
				build_entity_node(w, h, 0);
			}
		}

		for (const node_binding& b : _node_bindings)
		{
			if (b.handle == _selected_entity)
			{
				_builder->set_focus(b.inner_row, false);
				break;
			}
		}

		_builder->widget_set_scroll_offset(_entity_area, old_scroll);

		if (!em.is_valid(_selected_entity))
			editor::get().get_gui_controller().set_selected_entity({});

		_builder->build_hierarchy();
	}

	vekt::id editor_panel_entities::build_entity_node(world& w, world_handle e, unsigned int depth)
	{
		entity_manager& em			= w.get_entity_manager();
		const u8		is_template = em.get_entity_flags(e).is_set(entity_flags::entity_flags_template);
		const u8		selected	= _selected_entity == e;
		if (is_template)
			_entity_meta[e.index].collapsed = true;

		const u8 is_collapsed = _entity_meta[e.index].collapsed;

		// Row container
		const vekt::id row = _builder->allocate();
		{
			_builder->widget_add_child(_entity_area, row);

			vekt::pos_props& pp = _builder->widget_get_pos_props(row);
			pp.flags			= vekt::pos_flags::pf_x_relative;
			pp.pos.x			= 0.0f;

			vekt::size_props& sz   = _builder->widget_get_size_props(row);
			sz.flags			   = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_abs;
			sz.size.x			   = 1.0f;
			sz.size.y			   = editor_theme::get().row_height;
			sz.child_margins	   = {0.0f, 0.0f, 0.0f, editor_theme::get().inner_margin};
			sz.child_margins.left  = editor_theme::get().outer_margin;
			sz.child_margins.right = editor_theme::get().outer_margin;
		}

		// Row container
		const vekt::id row_inner = _builder->allocate();
		{
			_builder->widget_add_child(row, row_inner);

			vekt::pos_props& pp = _builder->widget_get_pos_props(row_inner);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_child_pos_row;
			pp.pos.x			= 0.0f;
			pp.pos.y			= 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(row_inner);
			sz.flags			 = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_relative;
			sz.size.x			 = 1.0f;
			sz.size.y			 = 1.0f;
			sz.child_margins	 = {0.0f, 0.0f, (depth + 1) * editor_theme::get().outer_margin, editor_theme::get().inner_margin};
			sz.spacing			 = editor_theme::get().item_spacing;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(row_inner);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_has_rounding | vekt::gfx_flags::gfx_has_stroke | vekt::gfx_flags::gfx_focusable;
			gfx.color			  = selected ? editor_theme::get().col_accent_second_dim : (vector4());
			gfx.draw_order		  = 1;

			vekt::stroke_props& sp = _builder->widget_get_stroke(row_inner);
			sp.thickness		   = editor_theme::get().frame_thickness;
			sp.color			   = vector4(0.0f, 0.0f, 0.0f, 0.0f);

			vekt::rounding_props& rp = _builder->widget_get_rounding(row_inner);
			rp.rounding				 = editor_theme::get().frame_rounding;
			rp.segments				 = 12;

			vekt::hover_callback& hb = _builder->widget_get_hover_callbacks(row_inner);
			hb.on_hover_move		 = on_tree_item_hover_move;
			hb.on_hover_end			 = on_tree_item_hover_end;
			hb.on_focus_gained		 = on_focus_gained;

			vekt::input_color_props& c = _builder->widget_get_input_colors(row_inner);
			c.focus_color			   = {};

			vekt::mouse_callback& mc = _builder->widget_get_mouse_callbacks(row_inner);
			mc.on_mouse				 = on_mouse;
			mc.on_drag				 = on_tree_item_drag;

			vekt::key_callback& kb = _builder->widget_get_key_callbacks(row_inner);
			kb.on_key			   = on_key;

			_builder->widget_get_user_data(row_inner).ptr = this;
		}

		const vekt::id icon_wrap = _builder->allocate();
		{
			_builder->widget_add_child(row_inner, icon_wrap);

			vekt::pos_props& pp = _builder->widget_get_pos_props(icon_wrap);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::size_props& sz = _builder->widget_get_size_props(icon_wrap);
			sz.flags			 = vekt::size_flags::sf_y_relative | vekt::size_flags::sf_x_copy_y;
			sz.size.y			 = 1.0f;
		}

		const vekt::id icon = _builder->allocate();
		{
			_builder->widget_add_child(icon_wrap, icon);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(icon);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = is_template ? editor_theme::get().col_accent_third : editor_theme::get().col_text;

			vekt::pos_props& pp = _builder->widget_get_pos_props(icon);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_anchor_center | vekt::pos_flags::pf_x_anchor_center;
			pp.pos.y			= 0.5f;
			pp.pos.x			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(icon);
			tp.text				 = is_template ? _text_icon_template : (is_collapsed ? _text_icon_dd_collapsed : _text_icon_dd);
			tp.font				 = editor_theme::get().font_icons;
			tp.scale			 = 0.7f;
			_builder->widget_update_text(icon);
		}

		const vekt::id txt = _builder->allocate();
		{
			_builder->widget_add_child(row_inner, txt);

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = is_template ? editor_theme::get().col_accent_third : editor_theme::get().col_text;

			vekt::pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(txt);
			tp.text				 = em.get_entity_meta(e).name;
			tp.font				 = editor_theme::get().font_default;
			_builder->widget_update_text(txt);
		}
		_node_bindings.push_back({row, row_inner, txt, e});
		_root_entity_widgets.push_back(row);

		const entity_family& fam = em.get_entity_family(e);
		world_handle		 ch	 = fam.first_child;

		if (!is_template)
		{
			_builder->widget_set_visible(icon, !ch.is_null());
		}

		if (!is_collapsed)
		{
			while (!ch.is_null())
			{
				build_entity_node(w, ch, depth + 1);
				ch = em.get_entity_family(ch).next_sibling;
			}
		}

		return row;
	}

	bool editor_panel_entities::is_ancestor_of(world_handle ancestor, world_handle node)
	{
		if (ancestor.is_null() || node.is_null())
			return false;
		world&			w	= editor::get().get_app().get_world();
		entity_manager& em	= w.get_entity_manager();
		bool			res = false;
		em.visit_parents(node, [&res, ancestor](world_handle e) {
			if (e == ancestor)
				res = true;
		});
		return res;
	}

	void editor_panel_entities::set_collapse(world_handle h, u8 collapsed)
	{
		_entity_meta[h.index].collapsed = collapsed;
		editor::get().get_app().get_world().get_entity_manager().set_hierarchy_dirty(1);
	}

	void editor_panel_entities::toggle_collapse(world_handle h)
	{
		u8 collapsed = _entity_meta[h.index].collapsed;
		collapsed ^= 1;
		set_collapse(h, collapsed);
	}

	void editor_panel_entities::drop_drag(world_handle target)
	{
		if (!_is_payload_on || _drag_source.is_null())
			return;
		editor::get().get_gui_controller().disable_payload();

		world_handle src = _drag_source;
		world_handle dst = target;

		world&			world		= editor::get().get_app().get_world();
		entity_manager& em			= world.get_entity_manager();
		const bool		is_template = dst.is_null() ? false : em.get_entity_flags(dst).is_set(entity_flags::entity_flags_template);

		if (!(src == dst) && !is_ancestor_of(src, dst) && !is_template)
		{

			const vector3 pos	= em.get_entity_position_abs(src);
			const quat	  rot	= em.get_entity_rotation_abs(src);
			const vector3 scale = em.get_entity_scale_abs(src);

			if (!em.get_entity_family(src).parent.is_null())
				em.remove_from_parent(src);

			if (!dst.is_null())
				em.add_child(dst, src);

			em.set_entity_position_abs(src, pos);
			em.set_entity_rotation_abs(src, rot);
			em.set_entity_scale_abs(src, scale);
		}
		_is_payload_on = false;
		_drag_source   = {};
	}

	void editor_panel_entities::kill_drag()
	{
		_is_payload_on	 = false;
		_drag_src_widget = {};
	}

	void editor_panel_entities::set_selected(world_handle h)
	{
		if (_selected_entity == h && h.is_null())
			return;

		if (!h.is_null())
		{
			entity_manager& em		= editor::get().get_app().get_world().get_entity_manager();
			bool			changed = false;
			em.visit_parents(h, [&](world_handle p) {
				if (_entity_meta[p.index].collapsed)
				{
					_entity_meta[p.index].collapsed = 0;
					changed							= true;
				}
			});
			if (changed)
				em.set_hierarchy_dirty(1);
		}

		if (_selected_entity == h)
			return;

		// deselect previous
		const world_handle existing = _selected_entity;
		if (!existing.is_null())
		{
			auto it = std::find_if(_node_bindings.begin(), _node_bindings.end(), [existing](const node_binding& nb) -> bool { return existing == nb.handle; });
			if (it != _node_bindings.end())
			{
				vekt::widget_gfx& gfx = _builder->widget_get_gfx(it->inner_row);
				gfx.color			  = {0, 0, 0, 0};
			}
		}

		// select new
		_selected_entity = h;

		auto it = std::find_if(_node_bindings.begin(), _node_bindings.end(), [h](const node_binding& nb) -> bool { return h == nb.handle; });
		if (it != _node_bindings.end())
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(it->inner_row);
			gfx.color			  = editor_theme::get().col_accent_second_dim;
		}
	}

	void editor_panel_entities::update_entity_name(world_handle h)
	{
		if (h.is_null())
			return;

		auto it = std::find_if(_node_bindings.begin(), _node_bindings.end(), [h](const node_binding& b) -> bool { return b.handle == h; });
		if (it == _node_bindings.end())
			return;

		entity_manager& em						 = editor::get().get_app().get_world().get_entity_manager();
		_builder->widget_get_text(it->text).text = em.get_entity_meta(h).name;
		_builder->widget_update_text(it->text);
	}

	void editor_panel_entities::set_entity_collapsed(world_handle h, bool collapsed)
	{
		if (h.is_null())
			return;

		set_collapse(h, collapsed ? 1 : 0);
	}

	void editor_panel_entities::on_context_item(vekt::id widget)
	{
		if (widget == _ctx_new_entity)
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.create_entity("new_entity");
			editor::get().get_gui_controller().set_selected_entity(h);
			return;
		}

		if (widget == _ctx_add_child && !_selected_entity.is_null())
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.create_entity("new_entity");

			_entity_meta[_selected_entity.index].collapsed = 0;
			em.add_child(_selected_entity, h);

			editor::get().get_gui_controller().set_selected_entity(h);
			return;
		}

		if (widget == _ctx_duplicate && !_selected_entity.is_null())
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.clone_entity(_selected_entity);
			editor::get().get_gui_controller().set_selected_entity(h);
			return;
		}

		if (widget == _ctx_delete && !_selected_entity.is_null())
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.destroy_entity(_selected_entity);
			editor::get().get_gui_controller().set_selected_entity({});
			return;
		}
	}

	vekt::input_event_result editor_panel_entities::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		if (widget == self->_entity_area)
		{
			if (ev.type == vekt::input_event_type::pressed)
			{
				editor::get().get_gui_controller().set_selected_entity({});

				if (ev.button == input_code::mouse_1)
				{
					editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);
					self->_ctx_new_entity = editor::get().get_gui_controller().add_context_menu_item("new_entity");
					editor::get().get_gui_controller().end_context_menu();
				}

				return vekt::input_event_result::handled;
			}

			if (ev.button == input_code::mouse_0 && ev.type == vekt::input_event_type::released)
			{
				if (self->_is_payload_on)
					self->drop_drag({});
				return vekt::input_event_result::handled;
			}

			return vekt::input_event_result::handled;
		}

		world_handle clicked = {};
		for (const node_binding& nb : self->_node_bindings)
		{
			if (nb.inner_row == widget)
			{
				clicked = nb.handle;
				break;
			}
		}

		if (self->_is_payload_on && ev.type == vekt::input_event_type::released)
		{
			if (self->_is_payload_on)
				self->drop_drag(clicked);
			return vekt::input_event_result::handled;
		}

		if (!clicked.is_null())
		{
			// select entity
			if (ev.type == vekt::input_event_type::pressed)
			{
				if (ev.button == static_cast<u16>(input_code::mouse_1))
				{
					editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);
					self->_ctx_add_child = editor::get().get_gui_controller().add_context_menu_item("add_child");
					self->_ctx_duplicate = editor::get().get_gui_controller().add_context_menu_item("duplicate");
					self->_ctx_delete	 = editor::get().get_gui_controller().add_context_menu_item("delete");
					editor::get().get_gui_controller().end_context_menu();

					editor::get().get_gui_controller().set_selected_entity(clicked);

					return vekt::input_event_result::handled;
				}
				else
				{
					editor::get().get_gui_controller().set_selected_entity(clicked);
					self->_drag_source	   = clicked;
					self->_drag_src_widget = widget;
					self->_drag_y		   = ev.position.y;
					return vekt::input_event_result::handled;
				}
			}

			// toggle entity
			if (ev.type == vekt::input_event_type::repeated)
			{
				const u8 is_template = editor::get().get_app().get_world().get_entity_manager().get_entity_flags(clicked).is_set(entity_flags::entity_flags_template);
				if (!is_template)
					self->toggle_collapse(clicked);
				editor::get().get_gui_controller().set_selected_entity(clicked);
				return vekt::input_event_result::handled;
			}

			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	vekt::input_event_result editor_panel_entities::on_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		if (self->_selected_entity.is_null())
			return vekt::input_event_result::not_handled;

		if (ev.type != vekt::input_event_type::pressed)
			return vekt::input_event_result::not_handled;

		if (ev.key == input_code::key_d && window::is_key_down(input_code::key_lctrl))
		{
			world&			   w  = editor::get().get_app().get_world();
			entity_manager&	   em = w.get_entity_manager();
			const world_handle h  = em.clone_entity(self->_selected_entity);
			editor::get().get_gui_controller().set_selected_entity(h);
			return vekt::input_event_result::handled;
		}

		if (ev.key == input_code::key_delete)
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.destroy_entity(self->_selected_entity);
			editor::get().get_gui_controller().set_selected_entity({});
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	void editor_panel_entities::on_tree_item_drag(vekt::builder* b, vekt::id widget, f32 mp_x, f32 mp_y, f32 delta_x, f32 delta_y, unsigned int button)
	{
		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
	}

	void editor_panel_entities::on_tree_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		editor_panel_entities* self		   = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		b->widget_get_stroke(widget).color = {};

		if (self->_drag_src_widget == widget && !self->_is_payload_on && !self->_drag_source.is_null() && b->widget_get_hover_callbacks(self->_drag_src_widget).is_pressing)
		{
			const char* name = editor::get().get_app().get_world().get_entity_manager().get_entity_meta(self->_drag_source).name;
			editor::get().get_gui_controller().enable_payload(name);
			self->_is_payload_on = true;
		}
	}

	void editor_panel_entities::on_tree_item_hover_move(vekt::builder* b, vekt::id widget)
	{

		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);
		if (self->_is_payload_on && widget != self->_drag_src_widget)
		{
			b->widget_get_stroke(widget).color = editor_theme::get().col_highlight;
		}
	}

	void editor_panel_entities::on_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav)
	{

		editor_panel_entities* self = static_cast<editor_panel_entities*>(b->widget_get_user_data(widget).ptr);

		for (const node_binding& binding : self->_node_bindings)
		{
			if (binding.inner_row == widget)
			{
				editor::get().get_gui_controller().set_selected_entity(binding.handle);
				return;
			}
		}
	}

}
