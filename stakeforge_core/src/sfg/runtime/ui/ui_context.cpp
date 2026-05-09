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

#include "ui_context.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg::ui
{
#define SNAPSHOT_SLOT_COUNT 3
#define SNAPSHOT_SLOT_MASK	0x3
#define SNAPSHOT_FRESH_FLAG 0x80

	void ui_context::init(const ui_config_t& cfg)
	{
		_tree.init(cfg.max_widgets);
		_paint.init(cfg.max_widgets);
		_input.init(cfg.input);
		_canvas.init(cfg.canvas);
		_text_pool.init(cfg.text_pool_capacity);

		const u32 snap_vtx_cap = static_cast<u32>(cfg.canvas.vertex_buffer_bytes / sizeof(vg_vertex_t));
		const u32 snap_idx_cap = static_cast<u32>(cfg.canvas.index_buffer_bytes / sizeof(vg_index_t));
		for (snapshot_slot_t& slot : _snapshot_slots)
			allocate_snapshot_slot(slot, cfg.canvas.buffer_count, snap_vtx_cap, snap_idx_cap);

		_producer_slot = 0;
		_consumer_slot = 1;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
	}

	void ui_context::uninit()
	{
		for (snapshot_slot_t& slot : _snapshot_slots)
			free_snapshot_slot(slot);
		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_producer_slot = 0;
		_consumer_slot = 0;

		_widget_texts.clear();
		_text_pool.uninit();
		_canvas.uninit();
		_input.uninit();
		_paint.uninit();
		_tree.uninit();
	}

	void ui_context::allocate_snapshot_slot(snapshot_slot_t& slot, u32 draw_buffer_capacity, u32 vertex_capacity, u32 index_capacity)
	{
		slot.draw_buffers				= static_cast<vg_draw_buffer_final_t*>(SFG_MALLOC(sizeof(vg_draw_buffer_final_t) * draw_buffer_capacity));
		slot.vertices					= static_cast<vg_vertex_t*>(SFG_MALLOC(sizeof(vg_vertex_t) * vertex_capacity));
		slot.indices					= static_cast<vg_index_t*>(SFG_MALLOC(sizeof(vg_index_t) * index_capacity));
		slot.draw_buffer_capacity		= draw_buffer_capacity;
		slot.vertex_capacity			= vertex_capacity;
		slot.index_capacity				= index_capacity;
		slot.snapshot.draw_buffers		= slot.draw_buffers;
		slot.snapshot.vertices			= slot.vertices;
		slot.snapshot.indices			= slot.indices;
		slot.snapshot.draw_buffer_count = 0;
		slot.snapshot.vertex_count		= 0;
		slot.snapshot.index_count		= 0;
	}

	void ui_context::free_snapshot_slot(snapshot_slot_t& slot)
	{
		if (slot.draw_buffers)
			SFG_FREE(slot.draw_buffers);
		if (slot.vertices)
			SFG_FREE(slot.vertices);
		if (slot.indices)
			SFG_FREE(slot.indices);
		slot = {};
	}

	void ui_context::set_pipelines(const ui_pipelines_t& pipelines)
	{
		_canvas.set_pipelines(pipelines);
	}

	void ui_context::tick(const vec4f_t& screen_rect, f32 dt_seconds)
	{
		_tree.solve(screen_rect);
		_input.tick(_tree, dt_seconds);

		_canvas.frame_begin(screen_rect);
		_paint.paint_all(_tree, _input, _canvas);
		_canvas.frame_end();

		_canvas.resolve();
	}

	void ui_context::publish_frame()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		snapshot_slot_t& slot = _snapshot_slots[_producer_slot];

		const vector_t<vg_draw_buffer_t>& src_dbs = _canvas.get_draw_buffers();

		u32 vtx_offset = 0;
		u32 idx_offset = 0;
		u32 db_count   = 0;

		for (const vg_draw_buffer_t& src : src_dbs)
		{
			if (src.vertex_count == 0 || src.index_count == 0)
				continue;
			SFG_ASSERT(db_count < slot.draw_buffer_capacity);
			SFG_ASSERT(vtx_offset + src.vertex_count <= slot.vertex_capacity);
			SFG_ASSERT(idx_offset + src.index_count <= slot.index_capacity);

			vg_draw_buffer_final_t& dst = slot.draw_buffers[db_count];
			dst.resolved				= src.resolved;
			dst.clip					= src.clip;
			dst.draw_order				= src.draw_order;
			dst.vertex_count			= src.vertex_count;
			dst.index_count				= src.index_count;
			dst.vertex_offset			= vtx_offset;
			dst.index_offset			= idx_offset;
			dst.is_atlas_sdf			= src.state.is_atlas_sdf;

			SFG_MEMCPY(slot.vertices + vtx_offset, src.vertex_start, src.vertex_count * sizeof(vg_vertex_t));
			SFG_MEMCPY(slot.indices + idx_offset, src.index_start, src.index_count * sizeof(vg_index_t));

			vtx_offset += src.vertex_count;
			idx_offset += src.index_count;
			db_count++;
		}

		slot.snapshot.draw_buffer_count = db_count;
		slot.snapshot.vertex_count		= vtx_offset;
		slot.snapshot.index_count		= idx_offset;

		const u8 prev  = _snapshot_mailbox.exchange(_producer_slot | SNAPSHOT_FRESH_FLAG, std::memory_order_release);
		_producer_slot = static_cast<u8>((prev & SNAPSHOT_SLOT_MASK) % SNAPSHOT_SLOT_COUNT);
	}

	const vg_draw_snapshot_t* ui_context::acquire_render_snapshot()
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD());

		u8 cur = _snapshot_mailbox.load(std::memory_order_acquire);
		while (cur & SNAPSHOT_FRESH_FLAG)
		{
			if (_snapshot_mailbox.compare_exchange_weak(cur, _consumer_slot, std::memory_order_acquire, std::memory_order_acquire))
			{
				_consumer_slot = static_cast<u8>((cur & SNAPSHOT_SLOT_MASK) % SNAPSHOT_SLOT_COUNT);
				break;
			}
		}
		return &_snapshot_slots[_consumer_slot].snapshot;
	}

	void ui_context::on_mouse_move(const vec2f_t& pos)
	{
		_input.on_mouse_move(pos);
	}

	void ui_context::on_mouse_button(mouse_button_e btn, bool pressed)
	{
		_input.on_mouse_button(btn, pressed);
	}

	void ui_context::on_wheel(f32 delta)
	{
		_input.on_wheel(delta);
	}

	void ui_context::on_key(const key_event_t& ev)
	{
		_input.on_key(ev);
	}

	void ui_context::set_widget_text(widget_id_t id, const char* text, u32 len)
	{
		auto& e = _widget_texts[id];
		if (e.ptr != nullptr)
			_text_pool.deallocate(e.ptr);
		e.ptr = _text_pool.allocate(text, len);
		e.len = len;
	}

	void ui_context::clear_widget_text(widget_id_t id)
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return;
		if (it->second.ptr != nullptr)
			_text_pool.deallocate(it->second.ptr);
		_widget_texts.erase(it);
	}

	const char* ui_context::widget_text(widget_id_t id) const
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return nullptr;
		return it->second.ptr;
	}

	u32 ui_context::widget_text_len(widget_id_t id) const
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return 0;
		return it->second.len;
	}

	widget_id_t ui_context::make_panel(widget_id_t parent)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::parent_relative;
		in.size_mode_y	 = axis_mode_e::parent_relative;
		in.size_value	 = {1.0f, 1.0f};
		in.child_margins = {_theme.margin_vertical, _theme.margin_horizontal, _theme.margin_vertical, _theme.margin_horizontal};
		in.child_spacing = _theme.item_spacing;

		vg_rect_paint_t rect   = {};
		rect.fill_color_a	   = _theme.color_panel_bg;
		rect.fill_color_b	   = _theme.color_panel_bg;
		rect.outline_color	   = _theme.color_item_outline;
		rect.outline_thickness = _theme.outline_thickness;
		_paint.set_rect(id, rect);

		return id;
	}

	widget_id_t ui_context::make_row(widget_id_t parent)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::parent_relative;
		in.size_mode_y	 = axis_mode_e::parent_relative;
		in.size_value	 = {1.0f, 1.0f};
		in.flow			 = flow_e::row;
		in.child_spacing = _theme.item_spacing;
		return id;
	}

	widget_id_t ui_context::make_column(widget_id_t parent)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::parent_relative;
		in.size_mode_y	 = axis_mode_e::parent_relative;
		in.size_value	 = {1.0f, 1.0f};
		in.flow			 = flow_e::column;
		in.child_spacing = _theme.item_spacing;
		return id;
	}

	widget_id_t ui_context::make_spacer(widget_id_t parent, f32 size_px)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in = _tree.in(id);
		if (size_px > 0.0f)
		{
			in.size_mode_x = axis_mode_e::fixed;
			in.size_mode_y = axis_mode_e::fixed;
			in.size_value  = {size_px, size_px};
		}
		else
		{
			in.size_mode_x = axis_mode_e::fill;
			in.size_mode_y = axis_mode_e::fill;
		}
		return id;
	}

	widget_id_t ui_context::make_label(widget_id_t parent, const char* text, resource_handle_t font)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		const u32 len = static_cast<u32>(text ? strlen(text) : 0);
		set_widget_text(id, text, len);

		layout_in_t& in = _tree.in(id);
		in.size_mode_x	= axis_mode_e::sum_children;
		in.size_mode_y	= axis_mode_e::fixed;
		in.size_value	= {0.0f, _theme.item_height};

		const vg_text_style_t style = {.font = font, .color = _theme.color_item_fg, .scale = 1.0f, .spacing = 0};

		const font_runtime_t* fnt = resource_manager_t::get().find_runtime<font_runtime_t>(font);
		if (fnt != nullptr)
		{
			const vg_text_paint_t tp = {.font = fnt, .color = style.color, .scale = style.scale, .spacing = style.spacing, .flip_uv = style.flip_uv};
			const vec2f_t		  m	 = vg_canvas_t::measure_text(text, len, tp);
			in.size_mode_x			 = axis_mode_e::fixed;
			in.size_value.x			 = m.x;
			in.size_value.y			 = m.y > 0.0f ? m.y : _theme.item_height;
		}

		_paint.set_text(id, widget_text(id), widget_text_len(id), style);
		return id;
	}

	widget_id_t ui_context::make_button(widget_id_t parent, const char* text, resource_handle_t font)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		const u32 len = static_cast<u32>(text ? strlen(text) : 0);
		set_widget_text(id, text, len);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::sum_children;
		in.size_mode_y	 = axis_mode_e::fixed;
		in.size_value	 = {0.0f, _theme.item_height};
		in.flags		 = wf_visible | wf_focusable;
		in.child_margins = {_theme.margin_vertical, _theme.margin_horizontal * 2, _theme.margin_vertical, _theme.margin_horizontal * 2};
		in.flow			 = flow_e::row;

		vg_rect_paint_t rect   = {};
		rect.fill_color_a	   = _theme.color_item_bg;
		rect.fill_color_b	   = _theme.color_item_bg;
		rect.outline_color	   = _theme.color_item_outline;
		rect.outline_thickness = _theme.outline_thickness;
		_paint.set_rect(id, rect);
		_paint.set_hover_color(id, _theme.color_item_hover);
		_paint.set_press_color(id, _theme.color_item_press);
		_paint.set_focus_color(id, _theme.color_focus);

		const widget_id_t lbl = _tree.allocate();
		_tree.attach(id, lbl);

		const vg_text_style_t style = {.font = font, .color = _theme.color_item_fg, .scale = 1.0f, .spacing = 0};

		const font_runtime_t* fnt = resource_manager_t::get().find_runtime<font_runtime_t>(font);
		vec2f_t				  m	  = {0.0f, 0.0f};
		if (fnt != nullptr)
		{
			const vg_text_paint_t tp = {.font = fnt, .color = style.color, .scale = style.scale, .spacing = style.spacing, .flip_uv = style.flip_uv};
			m						 = vg_canvas_t::measure_text(text, len, tp);
		}

		layout_in_t& lin = _tree.in(lbl);
		lin.size_mode_x	 = axis_mode_e::fixed;
		lin.size_mode_y	 = axis_mode_e::fixed;
		lin.size_value	 = {m.x, m.y > 0.0f ? m.y : _theme.item_height};
		lin.flags		 = wf_visible | wf_no_input;
		_paint.set_text(lbl, widget_text(id), widget_text_len(id), style);

		return id;
	}

	widget_id_t ui_context::make_divider(widget_id_t parent, bool horizontal)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in = _tree.in(id);
		if (horizontal)
		{
			in.size_mode_x = axis_mode_e::parent_relative;
			in.size_mode_y = axis_mode_e::fixed;
			in.size_value  = {1.0f, 1.0f};
		}
		else
		{
			in.size_mode_x = axis_mode_e::fixed;
			in.size_mode_y = axis_mode_e::parent_relative;
			in.size_value  = {1.0f, 1.0f};
		}

		vg_rect_paint_t rect = {};
		rect.fill_color_a	 = _theme.color_divider;
		rect.fill_color_b	 = _theme.color_divider;
		_paint.set_rect(id, rect);
		return id;
	}
}
