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
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg::ui
{
#define SNAPSHOT_SLOT_COUNT	  3
#define SNAPSHOT_SLOT_MASK	  0x3
#define SNAPSHOT_FRESH_FLAG	  0x80
#define DEBUG_TEXT_POINT_SIZE 12.0f
#define DEBUG_TEXT_PADDING	  2.0f

	namespace
	{
		bool point_in_rect(const vec4f_t& rect, const vec2f_t& point)
		{
			return point.x >= rect.x && point.x <= rect.x + rect.z && point.y >= rect.y && point.y <= rect.y + rect.w;
		}
	}

	void ui_context::init(const ui_config_t& cfg)
	{
		_user_ui_scale = cfg.user_ui_scale;

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

		_widget_debug_names.clear();
		_widget_texts.clear();
		_text_pool.uninit();
		_canvas.uninit();
		_input.uninit();
		_paint.uninit();
		_tree.uninit();
		_debug_font = NULL_RESOURCE_HANDLE;
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

	void ui_context::tick(const vec4f_t& screen_rect, f32 dpi_scale, f32 dt_seconds)
	{
		_dpi_scale = dpi_scale > 0.0f ? dpi_scale : 1.0f;
		_ui_scale  = _dpi_scale * _user_ui_scale;
		_paint.update_text_layout(_tree, _ui_scale, _dpi_scale);
		_tree.solve(screen_rect, _ui_scale);
		_input.tick(_tree, dt_seconds);

		_canvas.frame_begin(screen_rect);
		_paint.paint_all(_tree, _input, _canvas, _ui_scale, _dpi_scale);
		if (_debug_draw)
			draw_debug_hovered_widget();
		_canvas.frame_end();

		_canvas.resolve();
	}

	void ui_context::set_debug_draw(bool enabled)
	{
		_debug_draw = enabled;
	}

	void ui_context::set_debug_font(resource_handle_t font)
	{
		_debug_font = font;
	}

	void ui_context::draw_debug_hovered_widget()
	{
		const span_t<const widget_id_t> dfs	   = _tree.get_dfs();
		const vec2f_t&					mouse  = _input.get_mouse_position();
		widget_id_t						target = NULL_WIDGET;

		for (size_t i = 0; i < dfs.size; ++i)
		{
			const widget_id_t id = dfs.data[i];
			if (id == _tree.get_root())
				continue;

			const layout_in_t& in = _tree.in_const(id);
			if (!(in.flags & wf_visible))
				continue;

			const layout_out_t& out = _tree.out(id);
			if (out.clip.z <= 0.0f || out.clip.w <= 0.0f)
				continue;
			if (point_in_rect(out.clip, mouse))
				target = id;
		}

		if (target == NULL_WIDGET)
			return;

		const layout_out_t& out = _tree.out(target);

		vg_rect_paint_t rect   = {};
		rect.filled			   = false;
		rect.outline_color	   = {1.0f, 0.0f, 1.0f, 1.0f};
		rect.outline_thickness = 1.0f;

		ui_render_state_t state = {};
		state.pipeline			= _paint.get_pipelines().default_pipeline;
		_canvas.add_rect({out.pos.x, out.pos.y}, {out.pos.x + out.size.x, out.pos.y + out.size.y}, rect, state, 0xFFFFFFFFu);

		const char* name = widget_debug_name(target);
		const u32	len	 = widget_debug_name_len(target);
		if (name == nullptr || len == 0)
			return;

		const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(_debug_font);
		if (font == nullptr || font->face == nullptr)
		{
			const paint_def_t& pd = _paint.def_const(target);
			if (pd.kind != paint_kind_e::text)
				return;
			font = resource_manager_t::get().find_runtime<font_runtime_t>(pd.text.font);
			if (font == nullptr || font->face == nullptr)
				return;
		}

		const f32 scale = _ui_scale > 0.0f ? _ui_scale : 1.0f;
		const f32 dpi	= _dpi_scale > 0.0f ? _dpi_scale : 1.0f;
		i32		  px	= static_cast<i32>(DEBUG_TEXT_POINT_SIZE * scale * dpi + 0.5f);
		if (px < 1)
			px = 1;

		vg_text_paint_t text_paint = {};
		text_paint.font			   = font;
		text_paint.color		   = {1.0f, 0.0f, 1.0f, 1.0f};
		text_paint.size_px		   = DEBUG_TEXT_POINT_SIZE * scale;
		text_paint.raster_px	   = static_cast<u32>(px);
		text_paint.raster_mode	   = glyph_raster_mode_e::grayscale;

		const vec2f_t text_size = vg_canvas_t::measure_text(name, len, text_paint);
		if (text_size.x <= 0.0f || text_size.y <= 0.0f)
			return;

		const layout_out_t& root_out  = _tree.out(_tree.get_root());
		const f32			screen_x0 = root_out.clip.x;
		const f32			screen_y0 = root_out.clip.y;
		const f32			screen_x1 = root_out.clip.x + root_out.clip.z;
		const f32			screen_y1 = root_out.clip.y + root_out.clip.w;

		const f32 x0 = out.pos.x;
		const f32 y0 = out.pos.y;
		const f32 x1 = out.pos.x + out.size.x;
		const f32 y1 = out.pos.y + out.size.y;
		const f32 w	 = text_size.x;
		const f32 h	 = text_size.y;
		const f32 p	 = DEBUG_TEXT_PADDING;

		vec2f_t candidates[8] = {
			{x0, y0 - h - p},
			{x1 - w, y0 - h - p},
			{x0, y1 + p},
			{x1 - w, y1 + p},
			{x0 + p, y0 + p},
			{x1 - w - p, y0 + p},
			{x0 + p, y1 - h - p},
			{x1 - w - p, y1 - h - p},
		};

		vec2f_t text_pos = candidates[0];
		bool	found	 = false;
		for (const vec2f_t& c : candidates)
		{
			if (c.x >= screen_x0 && c.y >= screen_y0 && c.x + w <= screen_x1 && c.y + h <= screen_y1)
			{
				text_pos = c;
				found	 = true;
				break;
			}
		}

		if (!found)
		{
			text_pos.x = math::clamp(x0 + p, screen_x0, math::max(screen_x0, screen_x1 - w));
			text_pos.y = math::clamp(y0 + p, screen_y0, math::max(screen_y0, screen_y1 - h));
		}

		ui_render_state_t text_state = {};
		text_state.pipeline			 = _paint.get_pipelines().grayscale_text_pipeline;
		if (text_state.pipeline == NULL_RESOURCE_HANDLE)
			return;

		_canvas.add_text(name, len, text_pos, text_paint, text_state, 0xFFFFFFFFu);
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

	void ui_context::set_widget_text(widget_id_t id, const char* text)
	{
		auto& e = _widget_texts[id];
		if (e.ptr != nullptr)
			_text_pool.deallocate(e.ptr);
		const u32 len = static_cast<u32>(strlen(text));
		e.ptr		  = _text_pool.allocate(text, len);
		e.len		  = len;

		paint_def_t& pd = _paint.def(id);
		if (pd.kind == paint_kind_e::text)
		{
			pd.text_data = e.ptr;
			pd.text_len	 = e.len;
		}
	}

	void ui_context::clear_widget_text(widget_id_t id)
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return;
		if (it->second.ptr != nullptr)
			_text_pool.deallocate(it->second.ptr);
		_widget_texts.erase(it);

		paint_def_t& pd = _paint.def(id);
		if (pd.kind == paint_kind_e::text)
		{
			pd.text_data = nullptr;
			pd.text_len	 = 0;
		}
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

	void ui_context::set_widget_debug_name(widget_id_t id, const char* text)
	{
		auto& e = _widget_debug_names[id];
		if (e.ptr != nullptr)
			_text_pool.deallocate(e.ptr);
		const u32 len = static_cast<u32>(strlen(text));
		e.ptr		  = _text_pool.allocate(text, len);
		e.len		  = len;
	}

	void ui_context::clear_widget_debug_name(widget_id_t id)
	{
		auto it = _widget_debug_names.find(id);
		if (it == _widget_debug_names.end())
			return;
		if (it->second.ptr != nullptr)
			_text_pool.deallocate(it->second.ptr);
		_widget_debug_names.erase(it);
	}

	const char* ui_context::widget_debug_name(widget_id_t id) const
	{
		auto it = _widget_debug_names.find(id);
		if (it == _widget_debug_names.end())
			return nullptr;
		return it->second.ptr;
	}

	u32 ui_context::widget_debug_name_len(widget_id_t id) const
	{
		auto it = _widget_debug_names.find(id);
		if (it == _widget_debug_names.end())
			return 0;
		return it->second.len;
	}

}
