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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/fixed_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	enum class axis_mode_e : u8
	{
		fixed,
		parent_relative,
		fill,
		sum_children,
		max_children,
		copy_other,
	};

	enum class pos_mode_e : u8
	{
		flow,
		offset_in_parent,
		relative_in_parent,
		absolute_screen,
	};

	enum class anchor_e : u8
	{
		start,
		center,
		end,
	};

	enum class flow_e : u8
	{
		none,
		row,
		column,
	};

	enum widget_flag_e : u16
	{
		wf_visible		 = 1 << 0,
		wf_clip_children = 1 << 1,
		wf_overlay		 = 1 << 2,
		wf_focusable	 = 1 << 3,
		wf_input		 = 1 << 4,
		wf_scroll_x		 = 1 << 5,
		wf_scroll_y		 = 1 << 6,
		wf_custom_solve	 = 1 << 7,
		wf_disabled		 = 1 << 8,
	};

	struct layout_in_t
	{
		vec2f_t		size_value	  = {0.0f, 0.0f};
		vec2f_t		pos_value	  = {0.0f, 0.0f};
		vec4f_t		child_margins = {0.0f, 0.0f, 0.0f, 0.0f}; // top, right, bottom, left
		vec2f_t		scroll_offset = {0.0f, 0.0f};
		f32			child_spacing = 0.0f;
		axis_mode_e size_mode_x	  = axis_mode_e::fixed;
		axis_mode_e size_mode_y	  = axis_mode_e::fixed;
		pos_mode_e	pos_mode_x	  = pos_mode_e::flow;
		pos_mode_e	pos_mode_y	  = pos_mode_e::flow;
		anchor_e	anchor_x	  = anchor_e::start;
		anchor_e	anchor_y	  = anchor_e::start;
		flow_e		flow		  = flow_e::none;
		u16			flags		  = wf_visible;
	};

	struct layout_out_t
	{
		vec2f_t pos		   = {0.0f, 0.0f};
		vec2f_t size	   = {0.0f, 0.0f};
		vec4f_t clip	   = {0.0f, 0.0f, 0.0f, 0.0f}; // intersected with ancestor clips
		vec2f_t max_scroll = {0.0f, 0.0f};
	};

	struct tree_node_t
	{
		widget_id_t parent		 = NULL_WIDGET;
		widget_id_t first_child	 = NULL_WIDGET;
		widget_id_t last_child	 = NULL_WIDGET;
		widget_id_t next_sibling = NULL_WIDGET;
		widget_id_t prev_sibling = NULL_WIDGET;
		u16			child_count	 = 0;
		u32			draw_order	 = 0;
		u8			depth		 = 0;
		u8			alive		 = 0;
	};

	class layout_tree_t;

	using custom_solve_fn = void (*)(layout_tree_t& tree, widget_id_t id, void* user_data);

	class layout_tree_t
	{
	public:
		layout_tree_t()								   = default;
		layout_tree_t(const layout_tree_t&)			   = delete;
		layout_tree_t& operator=(const layout_tree_t&) = delete;
		~layout_tree_t();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 max_widgets);
		void uninit();
		void solve(const vec4f_t& screen_rect, f32 ui_scale = 1.0f);

		// -----------------------------------------------------------------------------
		// widgets
		// -----------------------------------------------------------------------------

		widget_id_t			allocate();
		void				deallocate(widget_id_t id);
		void				attach(widget_id_t parent, widget_id_t child);
		void				detach(widget_id_t child);
		void				set_custom_solve(widget_id_t id, custom_solve_fn fn, void* user_data);
		const tree_node_t&	node(widget_id_t id) const;
		u32&				draw_order(widget_id_t id);
		u32					draw_order_const(widget_id_t id) const;
		layout_in_t&		in(widget_id_t id);
		const layout_in_t&	in_const(widget_id_t id) const;
		const layout_out_t& out(widget_id_t id) const;
		vec4f_t				bounds(widget_id_t id) const;
		bool				is_alive(widget_id_t id) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline widget_id_t get_root() const
		{
			return _root;
		}

		inline bool topology_dirty() const
		{
			return _topology_dirty;
		}
		inline bool layout_dirty() const
		{
			return _layout_dirty;
		}

		inline void mark_layout_dirty()
		{
			_layout_dirty = true;
		}

		inline span_t<const widget_id_t> get_dfs() const
		{
			return {_dfs.data(), _dfs.size()};
		}

		inline span_t<const u32> get_dfs_descendants() const
		{
			return {_dfs_descendants.data(), _dfs_descendants.size()};
		}

	private:
		void flatten();

	private:
		struct custom_cb_t
		{
			custom_solve_fn fn		  = nullptr;
			void*			user_data = nullptr;
		};

	private:
		fixed_vector_t<tree_node_t>	 _nodes;
		fixed_vector_t<layout_in_t>	 _layout_ins;
		fixed_vector_t<layout_out_t> _layout_outs;
		fixed_vector_t<custom_cb_t>	 _custom_cbs;
		fixed_vector_t<widget_id_t>	 _free_list;
		fixed_vector_t<widget_id_t>	 _dfs;
		fixed_vector_t<u32>			 _dfs_descendants;
		widget_id_t					 _root			 = NULL_WIDGET;
		u32							 _max_widgets	 = 0;
		u32							 _alive_count	 = 0;
		bool						 _topology_dirty = true;
		bool						 _layout_dirty	 = true;
	};
}
