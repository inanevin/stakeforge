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

#include "layout_tree.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>

namespace sfg::ui
{
	namespace
	{
		inline vec4f_t intersect_rect(const vec4f_t& a, const vec4f_t& b)
		{
			const f32 x = math::max(a.x, b.x);
			const f32 y = math::max(a.y, b.y);
			const f32 r = math::min(a.x + a.z, b.x + b.z);
			const f32 t = math::min(a.y + a.w, b.y + b.w);
			if (r < x || t < y)
				return {0, 0, 0, 0};
			return {x, y, r - x, t - y};
		}

		inline vec4f_t scale_rect(const vec4f_t& v, f32 scale)
		{
			return {v.x * scale, v.y * scale, v.z * scale, v.w * scale};
		}
	}

	layout_tree_t::~layout_tree_t()
	{
		SFG_ASSERT(_nodes.empty());
	}

	void layout_tree_t::init(u32 max_widgets)
	{
		SFG_ASSERT(max_widgets > 0 && max_widgets <= 0xFFFEu);
		_max_widgets = max_widgets;
		_nodes.init(max_widgets);
		_layout_ins.init(max_widgets);
		_layout_outs.init(max_widgets);
		_custom_cbs.init(max_widgets);
		_free_list.init(max_widgets);
		_dfs.init(max_widgets);
		_dfs_descendants.init(max_widgets);
		_nodes.resize(max_widgets);
		_layout_ins.resize(max_widgets);
		_layout_outs.resize(max_widgets);
		_custom_cbs.resize(max_widgets);

		for (u32 i = 0; i < max_widgets; ++i)
			_free_list.push_back(static_cast<widget_id_t>(max_widgets - 1 - i));

		_root = allocate();
	}

	void layout_tree_t::uninit()
	{
		_dfs_descendants.uninit();
		_dfs.uninit();
		_free_list.uninit();
		_custom_cbs.uninit();
		_layout_outs.uninit();
		_layout_ins.uninit();
		_nodes.uninit();
		_alive_count = 0;
		_root		 = NULL_WIDGET;
	}

	widget_id_t layout_tree_t::allocate()
	{
		SFG_ASSERT(!_free_list.empty());
		const widget_id_t id = _free_list.back();
		_free_list.pop_back();
		_nodes[id]		 = {};
		_layout_ins[id]	 = {};
		_layout_outs[id] = {};
		_custom_cbs[id]	 = {};
		_nodes[id].alive = 1;
		_alive_count++;
		_topology_dirty = true;
		_layout_dirty	= true;
		return id;
	}

	void layout_tree_t::deallocate(widget_id_t id)
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);

		widget_id_t c = _nodes[id].first_child;
		while (c != NULL_WIDGET)
		{
			const widget_id_t next = _nodes[c].next_sibling;
			deallocate(c);
			c = next;
		}

		if (_nodes[id].parent != NULL_WIDGET)
			detach(id);

		_nodes[id].alive = 0;
		_free_list.push_back(id);
		_alive_count--;
		_topology_dirty = true;
		_layout_dirty	= true;
	}

	void layout_tree_t::attach(widget_id_t parent, widget_id_t child)
	{
		SFG_ASSERT(parent < _max_widgets && _nodes[parent].alive);
		SFG_ASSERT(child < _max_widgets && _nodes[child].alive);
		SFG_ASSERT(parent != child);

		if (_nodes[child].parent != NULL_WIDGET)
			detach(child);

		tree_node_t& p = _nodes[parent];
		tree_node_t& c = _nodes[child];

		c.parent	   = parent;
		c.prev_sibling = p.last_child;
		c.next_sibling = NULL_WIDGET;
		c.draw_order   = p.draw_order;

		if (p.last_child != NULL_WIDGET)
			_nodes[p.last_child].next_sibling = child;
		else
			p.first_child = child;

		p.last_child = child;
		p.child_count++;
		_topology_dirty = true;
		_layout_dirty	= true;
	}

	void layout_tree_t::detach(widget_id_t child)
	{
		SFG_ASSERT(child < _max_widgets && _nodes[child].alive);
		tree_node_t& c = _nodes[child];
		if (c.parent == NULL_WIDGET)
			return;

		tree_node_t& p = _nodes[c.parent];
		if (c.prev_sibling != NULL_WIDGET)
			_nodes[c.prev_sibling].next_sibling = c.next_sibling;
		else
			p.first_child = c.next_sibling;

		if (c.next_sibling != NULL_WIDGET)
			_nodes[c.next_sibling].prev_sibling = c.prev_sibling;
		else
			p.last_child = c.prev_sibling;

		p.child_count--;

		c.parent		= NULL_WIDGET;
		c.prev_sibling	= NULL_WIDGET;                 
		c.next_sibling	= NULL_WIDGET;
		_topology_dirty = true;
		_layout_dirty	= true;
	}

	layout_in_t& layout_tree_t::in(widget_id_t id)
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		_layout_dirty = true;
		return _layout_ins[id];
	}

	const layout_in_t& layout_tree_t::in_const(widget_id_t id) const
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		return _layout_ins[id];
	}

	const layout_out_t& layout_tree_t::out(widget_id_t id) const
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		return _layout_outs[id];
	}

	void layout_tree_t::set_custom_solve(widget_id_t id, custom_solve_fn fn, void* user_data)
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		_custom_cbs[id] = {fn, user_data};
	}

	const tree_node_t& layout_tree_t::node(widget_id_t id) const
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		return _nodes[id];
	}

	u32& layout_tree_t::draw_order(widget_id_t id)
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		return _nodes[id].draw_order;
	}

	u32 layout_tree_t::draw_order_const(widget_id_t id) const
	{
		SFG_ASSERT(id < _max_widgets && _nodes[id].alive);
		return _nodes[id].draw_order;
	}

	bool layout_tree_t::is_alive(widget_id_t id) const
	{
		return id < _max_widgets && _nodes[id].alive != 0;
	}

	void layout_tree_t::set_visible(widget_id_t id, bool visible)
	{
		if (visible)
			in(id).flags |= wf_visible;
		else
			in(id).flags &= ~wf_visible;
	}

	vec4f_t layout_tree_t::bounds(widget_id_t id) const
	{
		const layout_out_t& o = out(id);
		return {o.pos.x, o.pos.y, o.size.x, o.size.y};
	}

	namespace
	{
		u32 dfs_emit(fixed_vector_t<tree_node_t>& nodes, fixed_vector_t<widget_id_t>& dfs, fixed_vector_t<u32>& dfs_desc, widget_id_t id, u8 depth)
		{
			nodes[id].depth	 = depth;
			const u32 my_idx = static_cast<u32>(dfs.size());
			dfs.push_back(id);
			dfs_desc.push_back(0);

			u32			descendants = 0;
			widget_id_t c			= nodes[id].first_child;
			while (c != NULL_WIDGET)
			{
				descendants += 1 + dfs_emit(nodes, dfs, dfs_desc, c, static_cast<u8>(depth + 1));
				c = nodes[c].next_sibling;
			}
			dfs_desc[my_idx] = descendants;
			return descendants;
		}
	}

	void layout_tree_t::flatten()
	{
		_dfs.resize(0);
		_dfs_descendants.resize(0);
		dfs_emit(_nodes, _dfs, _dfs_descendants, _root, 0);
		_topology_dirty = false;
	}

	void layout_tree_t::solve(const vec4f_t& screen_rect, f32 ui_scale)
	{
		if (_topology_dirty)
			flatten();

		const f32 scale = ui_scale > 0.0f ? ui_scale : 1.0f;

		layout_in_t&  ri = _layout_ins[_root];
		layout_out_t& ro = _layout_outs[_root];

		ri.pos_mode_x  = pos_mode_e::absolute_screen;
		ri.pos_mode_y  = pos_mode_e::absolute_screen;
		ri.size_mode_x = axis_mode_e::fixed;
		ri.size_mode_y = axis_mode_e::fixed;
		ri.pos_value   = {screen_rect.x, screen_rect.y};
		ri.size_value  = {screen_rect.z, screen_rect.w};
		ro.pos		   = ri.pos_value;
		ro.size		   = ri.size_value;
		ro.clip		   = {ri.pos_value.x, ri.pos_value.y, ri.size_value.x, ri.size_value.y};

		for (widget_id_t id : _dfs)
		{
			layout_in_t&	   in  = _layout_ins[id];
			layout_out_t&	   out = _layout_outs[id];
			const tree_node_t& n   = _nodes[id];

			if (in.flags & wf_custom_solve)
			{
				const custom_cb_t& cb = _custom_cbs[id];
				if (cb.fn)
					cb.fn(*this, id, cb.user_data);
				continue;
			}

			if (id != _root && in.size_mode_x == axis_mode_e::fixed)
				out.size.x = in.size_value.x * scale;
			if (id != _root && in.size_mode_y == axis_mode_e::fixed)
				out.size.y = in.size_value.y * scale;
		}

		for (i32 i = static_cast<i32>(_dfs.size()) - 1; i >= 0; --i)
		{
			const widget_id_t  id  = _dfs[i];
			layout_in_t&	   in  = _layout_ins[id];
			layout_out_t&	   out = _layout_outs[id];
			const tree_node_t& n   = _nodes[id];

			const bool dx = in.size_mode_x == axis_mode_e::sum_children || in.size_mode_x == axis_mode_e::max_children;
			const bool dy = in.size_mode_y == axis_mode_e::sum_children || in.size_mode_y == axis_mode_e::max_children;
			if (!dx && !dy)
				continue;

			f32			total_x = 0.0f;
			f32			total_y = 0.0f;
			u32			counted = 0;
			widget_id_t c		= n.first_child;
			while (c != NULL_WIDGET)
			{
				const layout_in_t&	cin	 = _layout_ins[c];
				const layout_out_t& cout = _layout_outs[c];
				const widget_id_t	next = _nodes[c].next_sibling;
				if ((cin.flags & wf_overlay) || !(cin.flags & wf_visible))
				{
					c = next;
					continue;
				}

				if (in.size_mode_x == axis_mode_e::sum_children && cin.size_mode_x != axis_mode_e::parent_relative)
					total_x += cout.size.x + (counted > 0 ? in.child_spacing * scale : 0.0f);
				else if (in.size_mode_x == axis_mode_e::max_children && cin.size_mode_x != axis_mode_e::parent_relative)
					total_x = math::max(total_x, cout.size.x);

				if (in.size_mode_y == axis_mode_e::sum_children && cin.size_mode_y != axis_mode_e::parent_relative)
					total_y += cout.size.y + (counted > 0 ? in.child_spacing * scale : 0.0f);
				else if (in.size_mode_y == axis_mode_e::max_children && cin.size_mode_y != axis_mode_e::parent_relative)
					total_y = math::max(total_y, cout.size.y);

				counted++;
				c = next;
			}

			const vec4f_t margins = scale_rect(in.child_margins, scale);
			const f32	  mh	  = margins.x + margins.z;
			const f32	  mw	  = margins.w + margins.y;

			if (dx)
				out.size.x = total_x + mw;
			if (dy)
				out.size.y = total_y + mh;
		}

		for (widget_id_t id : _dfs)
		{
			layout_in_t&	   in  = _layout_ins[id];
			layout_out_t&	   out = _layout_outs[id];
			const tree_node_t& n   = _nodes[id];
			if (id == _root)
				continue;
			if (n.parent == NULL_WIDGET)
				continue;

			const layout_in_t&	pin			= _layout_ins[n.parent];
			const layout_out_t& pout		= _layout_outs[n.parent];
			const vec4f_t		pin_margins = scale_rect(pin.child_margins, scale);
			const f32			pinner_w	= pout.size.x - pin_margins.w - pin_margins.y;
			const f32			pinner_h	= pout.size.y - pin_margins.x - pin_margins.z;

			if (in.size_mode_x == axis_mode_e::parent_relative)
				out.size.x = pinner_w * in.size_value.x;
			if (in.size_mode_y == axis_mode_e::parent_relative)
				out.size.y = pinner_h * in.size_value.y;

			if (in.size_mode_x == axis_mode_e::copy_other)
				out.size.x = out.size.y;
			if (in.size_mode_y == axis_mode_e::copy_other)
				out.size.y = out.size.x;
		}

		for (widget_id_t id : _dfs)
		{
			const layout_in_t&	in	 = _layout_ins[id];
			const layout_out_t& pout = _layout_outs[id];
			if (in.flow == flow_e::none)
				continue;
			const tree_node_t& n = _nodes[id];

			const vec4f_t margins = scale_rect(in.child_margins, scale);
			const f32	  inner_w = pout.size.x - margins.w - margins.y;
			const f32	  inner_h = pout.size.y - margins.x - margins.z;

			f32 used		  = 0.0f;
			u32 fill_count	  = 0;
			u32 visible_count = 0;

			widget_id_t c = n.first_child;
			while (c != NULL_WIDGET)
			{
				const layout_in_t&	cin	 = _layout_ins[c];
				const layout_out_t& cout = _layout_outs[c];
				const widget_id_t	nxt	 = _nodes[c].next_sibling;
				if ((cin.flags & wf_overlay) || !(cin.flags & wf_visible))
				{
					c = nxt;
					continue;
				}

				if (in.flow == flow_e::row)
				{
					if (cin.size_mode_x == axis_mode_e::fill)
						fill_count++;
					else
						used += cout.size.x;
				}
				else
				{
					if (cin.size_mode_y == axis_mode_e::fill)
						fill_count++;
					else
						used += cout.size.y;
				}
				visible_count++;
				c = nxt;
			}

			if (visible_count == 0)
				continue;

			const f32 spacing_total	  = in.child_spacing * scale * static_cast<f32>(visible_count - 1);
			const f32 main_axis_avail = (in.flow == flow_e::row) ? inner_w : inner_h;
			const f32 leftover		  = math::max(0.0f, main_axis_avail - used - spacing_total);
			const f32 per_fill		  = fill_count > 0 ? leftover / static_cast<f32>(fill_count) : 0.0f;

			c = n.first_child;
			while (c != NULL_WIDGET)
			{
				const layout_in_t& cin = _layout_ins[c];
				layout_out_t&	   o   = _layout_outs[c];
				const widget_id_t  nxt = _nodes[c].next_sibling;

				if ((cin.flags & wf_overlay) || !(cin.flags & wf_visible))
				{
					c = nxt;
					continue;
				}

				if (in.flow == flow_e::row)
				{
					if (cin.size_mode_x == axis_mode_e::fill)
						o.size.x = per_fill;
					if (cin.size_mode_y == axis_mode_e::fill)
						o.size.y = inner_h;
				}
				else
				{
					if (cin.size_mode_y == axis_mode_e::fill)
						o.size.y = per_fill;
					if (cin.size_mode_x == axis_mode_e::fill)
						o.size.x = inner_w;
				}
				c = nxt;
			}
		}

		for (widget_id_t id : _dfs)
		{
			layout_in_t&		in	= _layout_ins[id];
			const layout_out_t& out = _layout_outs[id];
			const tree_node_t&	n	= _nodes[id];

			const vec4f_t margins = scale_rect(in.child_margins, scale);
			const f32	  inner_x = out.pos.x + margins.w;
			const f32	  inner_y = out.pos.y + margins.x;
			const f32	  inner_w = out.size.x - margins.w - margins.y;
			const f32	  inner_h = out.size.y - margins.x - margins.z;
			in.scroll_offset.x	  = math::clamp(in.scroll_offset.x, -out.max_scroll.x, 0.0f);
			in.scroll_offset.y	  = math::clamp(in.scroll_offset.y, -out.max_scroll.y, 0.0f);
			f32 flow_x			  = inner_x + in.scroll_offset.x * scale;
			f32 flow_y			  = inner_y + in.scroll_offset.y * scale;

			widget_id_t c = n.first_child;
			while (c != NULL_WIDGET)
			{
				const layout_in_t& cin = _layout_ins[c];
				layout_out_t&	   co  = _layout_outs[c];
				const widget_id_t  nxt = _nodes[c].next_sibling;

				if (cin.pos_mode_x == pos_mode_e::absolute_screen)
				{
					co.pos.x = cin.pos_value.x;
				}
				else if (cin.pos_mode_x == pos_mode_e::offset_in_parent)
				{
					co.pos.x = inner_x + cin.pos_value.x * scale;
				}
				else if (cin.pos_mode_x == pos_mode_e::relative_in_parent)
				{
					f32 base = inner_x + cin.pos_value.x * inner_w;
					if (cin.anchor_x == anchor_e::center)
						base -= co.size.x * 0.5f;
					else if (cin.anchor_x == anchor_e::end)
						base -= co.size.x;
					co.pos.x = base;
				}
				else
				{
					if (in.flow == flow_e::row && !(cin.flags & wf_overlay) && (cin.flags & wf_visible))
					{
						co.pos.x = flow_x;
						flow_x += co.size.x + in.child_spacing * scale;
					}
					else
					{
						co.pos.x = inner_x + in.scroll_offset.x * scale;
					}
				}

				if (cin.pos_mode_y == pos_mode_e::absolute_screen)
				{
					co.pos.y = cin.pos_value.y;
				}
				else if (cin.pos_mode_y == pos_mode_e::offset_in_parent)
				{
					co.pos.y = inner_y + cin.pos_value.y * scale;
				}
				else if (cin.pos_mode_y == pos_mode_e::relative_in_parent)
				{
					f32 base = inner_y + cin.pos_value.y * inner_h;
					if (cin.anchor_y == anchor_e::center)
						base -= co.size.y * 0.5f;
					else if (cin.anchor_y == anchor_e::end)
						base -= co.size.y;
					co.pos.y = base;
				}
				else
				{
					if (in.flow == flow_e::column && !(cin.flags & wf_overlay) && (cin.flags & wf_visible))
					{
						co.pos.y = flow_y;
						flow_y += co.size.y + in.child_spacing * scale;
					}
					else
					{
						co.pos.y = inner_y + in.scroll_offset.y * scale;
					}
				}

				c = nxt;
			}
		}

		for (widget_id_t id : _dfs)
		{
			const layout_in_t& in  = _layout_ins[id];
			layout_out_t&	   out = _layout_outs[id];
			const tree_node_t& n   = _nodes[id];

			if ((in.flags & (wf_scroll_x | wf_scroll_y)) == 0)
			{
				out.max_scroll = {};
				continue;
			}

			const vec4f_t margins = scale_rect(in.child_margins, scale);
			const f32	  inner_x = out.pos.x + margins.w;
			const f32	  inner_y = out.pos.y + margins.x;
			const f32	  inner_w = out.size.x - margins.w - margins.y;
			const f32	  inner_h = out.size.y - margins.x - margins.z;
			f32			  min_x	  = inner_x;
			f32			  min_y	  = inner_y;
			f32			  max_x	  = inner_x + inner_w;
			f32			  max_y	  = inner_y + inner_h;

			widget_id_t c = n.first_child;
			while (c != NULL_WIDGET)
			{
				const layout_in_t&	cin	 = _layout_ins[c];
				const layout_out_t& cout = _layout_outs[c];
				const widget_id_t	next = _nodes[c].next_sibling;
				if ((cin.flags & wf_overlay) || !(cin.flags & wf_visible))
				{
					c = next;
					continue;
				}

				min_x = math::min(min_x, cout.pos.x - in.scroll_offset.x * scale);
				min_y = math::min(min_y, cout.pos.y - in.scroll_offset.y * scale);
				max_x = math::max(max_x, cout.pos.x - in.scroll_offset.x * scale + cout.size.x);
				max_y = math::max(max_y, cout.pos.y - in.scroll_offset.y * scale + cout.size.y);
				c	  = next;
			}

			out.max_scroll.x = (in.flags & wf_scroll_x) ? math::max(0.0f, max_x - min_x - inner_w) / scale : 0.0f;
			out.max_scroll.y = (in.flags & wf_scroll_y) ? math::max(0.0f, max_y - min_y - inner_h) / scale : 0.0f;
		}

		for (widget_id_t id : _dfs)
		{
			const layout_in_t& in  = _layout_ins[id];
			layout_out_t&	   out = _layout_outs[id];
			const tree_node_t& n   = _nodes[id];

			const vec4f_t bbox = {out.pos.x, out.pos.y, out.size.x, out.size.y};
			if (id == _root)
			{
				out.clip = bbox;
				continue;
			}

			const layout_out_t& pout = _layout_outs[n.parent];
			out.clip				 = intersect_rect(pout.clip, bbox);
		}

		_layout_dirty = false;
	}
}
