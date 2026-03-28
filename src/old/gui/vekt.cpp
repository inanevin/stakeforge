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

#include "vekt.hpp"
#include "memory/memory_tracer.hpp"
#include "data/char_util.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb/stb_truetype.h"

namespace vekt
{

#define ALIGN_8(SZ) ((SZ + 7) & ~size_t(7))

	config_data config = {};

	////////////////////////////////////////////////////////////////////////////////
	// :: THEME IMPL
	////////////////////////////////////////////////////////////////////////////////

	VEKT_VEC4 theme::color_item_bg		= {3.0f / 255.0f, 3.0f / 255.0f, 3.0f / 255.0f, 1.0f};
	VEKT_VEC4 theme::color_item_hover	= {32.0f / 255.0f, 32.0f / 255.0f, 32.0f / 255.0f, 1.0f};
	VEKT_VEC4 theme::color_item_press	= {24.0f / 255.0f, 24.0f / 255.0f, 24.0f / 255.0f, 1.0f};
	VEKT_VEC4 theme::color_panel_bg		= {24.0f / 255.0f, 24.0f / 255.0f, 24.0f / 255.0f, 1.0f};
	VEKT_VEC4 theme::color_divider		= {12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f};
	VEKT_VEC4 theme::color_item_outline = {42.0f / 255.0f, 42.0f / 255.0f, 42.0f / 255.0f, 1.0f};
	VEKT_VEC4 theme::color_item_fg		= {200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.0f};
	f32		  theme::item_height		= 24.0f;
	f32		  theme::item_spacing		= 8.0f;
	f32		  theme::indent_horizontal	= 8.0f;
	f32		  theme::margin_horizontal	= 4.0f;
	f32		  theme::margin_vertical	= 2.0f;
	f32		  theme::border_thickness	= 6.0f;
	f32		  theme::outline_thickness	= 2.0f;

	////////////////////////////////////////////////////////////////////////////////
	// :: BUILDER IMPL
	////////////////////////////////////////////////////////////////////////////////

	void builder::init(const init_config& conf)
	{
		ASSERT(conf.vertex_buffer_sz > 0 && conf.index_buffer_sz > 0 && conf.vertex_buffer_sz > 0 && conf.widget_count > 0);
		ASSERT(conf.buffer_count * sizeof(vertex) < conf.vertex_buffer_sz && conf.buffer_count * sizeof(index) < conf.index_buffer_sz);

		_widget_count = conf.widget_count;

		// Layout arena
		const size_t widget_meta_sz = ALIGN_8(sizeof(widget_meta)) * _widget_count;
		const size_t pos_props_sz	= ALIGN_8(sizeof(pos_props)) * _widget_count;
		const size_t size_props_sz	= ALIGN_8(sizeof(size_props)) * _widget_count;
		const size_t pos_result_sz	= ALIGN_8(sizeof(pos_result)) * _widget_count;
		const size_t size_result_sz = ALIGN_8(sizeof(size_result)) * _widget_count;
		const size_t user_data_sz	= ALIGN_8(sizeof(widget_user_data)) * _widget_count;
		const size_t scroll_sz		= ALIGN_8(sizeof(scroll_props)) * _widget_count;
		const size_t inp_col_sz		= ALIGN_8(sizeof(input_color_props)) * _widget_count;
		_layout_arena.capacity		= widget_meta_sz + pos_props_sz + size_props_sz + pos_result_sz + size_result_sz + user_data_sz + scroll_sz + inp_col_sz;
		_layout_arena.base_ptr		= ALIGNED_MALLOC(_layout_arena.capacity, 8);
		MEMSET(_layout_arena.base_ptr, 0, _layout_arena.capacity);

		size_t offset = 0;
		_metas		  = reinterpret_cast<widget_meta*>(_layout_arena.base_ptr);
		offset += widget_meta_sz;

		_pos_properties = reinterpret_cast<pos_props*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += pos_props_sz;

		_size_properties = reinterpret_cast<size_props*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += size_props_sz;

		_pos_results = reinterpret_cast<pos_result*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += pos_result_sz;

		_size_results = reinterpret_cast<size_result*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += size_result_sz;

		_user_datas = reinterpret_cast<widget_user_data*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += user_data_sz;

		_scroll_properties = reinterpret_cast<scroll_props*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += scroll_sz;

		_input_color_properties = reinterpret_cast<input_color_props*>(reinterpret_cast<unsigned char*>(_layout_arena.base_ptr) + offset);
		offset += inp_col_sz;

		// Gfx arena
		const size_t widget_gfx_sz		   = ALIGN_8(sizeof(widget_gfx)) * _widget_count;
		const size_t stroke_props_sz	   = ALIGN_8(sizeof(stroke_props)) * _widget_count;
		const size_t second_color_props_sz = ALIGN_8(sizeof(second_color_props)) * _widget_count;
		const size_t rounding_props_sz	   = ALIGN_8(sizeof(rounding_props)) * _widget_count;
		const size_t aa_props_sz		   = ALIGN_8(sizeof(aa_props)) * _widget_count;
		const size_t text_props_sz		   = ALIGN_8(sizeof(text_props)) * _widget_count;
		_gfx_arena.capacity				   = widget_gfx_sz + stroke_props_sz + second_color_props_sz + rounding_props_sz + aa_props_sz + text_props_sz;
		_gfx_arena.base_ptr				   = ALIGNED_MALLOC(_gfx_arena.capacity, 8);
		MEMSET(_gfx_arena.base_ptr, 0, _gfx_arena.capacity);

		for (unsigned int i = 0; i < _widget_count; i++)
		{
			_metas[i].parent					= NULL_WIDGET_ID;
			_scroll_properties[i].scroll_parent = NULL_WIDGET_ID;
		}

		_gfxs		   = reinterpret_cast<widget_gfx*>(_gfx_arena.base_ptr);
		_strokes	   = reinterpret_cast<stroke_props*>(reinterpret_cast<unsigned char*>(_gfx_arena.base_ptr) + widget_gfx_sz);
		_second_colors = reinterpret_cast<second_color_props*>(reinterpret_cast<unsigned char*>(_gfx_arena.base_ptr) + widget_gfx_sz + stroke_props_sz);
		_roundings	   = reinterpret_cast<rounding_props*>(reinterpret_cast<unsigned char*>(_gfx_arena.base_ptr) + widget_gfx_sz + stroke_props_sz + second_color_props_sz);
		_aa_props	   = reinterpret_cast<aa_props*>(reinterpret_cast<unsigned char*>(_gfx_arena.base_ptr) + widget_gfx_sz + stroke_props_sz + second_color_props_sz + rounding_props_sz);
		_texts		   = reinterpret_cast<text_props*>(reinterpret_cast<unsigned char*>(_gfx_arena.base_ptr) + widget_gfx_sz + stroke_props_sz + second_color_props_sz + rounding_props_sz + aa_props_sz);

		// Misc-rest
		const size_t hover_callbacks_sz = ALIGN_8(sizeof(hover_callback)) * _widget_count;
		const size_t mouse_callbacks_sz = ALIGN_8(sizeof(mouse_callback)) * _widget_count;
		const size_t key_callbacks_sz	= ALIGN_8(sizeof(key_callback)) * _widget_count;
		const size_t custom_passes_sz	= ALIGN_8(sizeof(custom_passes)) * _widget_count;
		_misc_arena.capacity			= hover_callbacks_sz + mouse_callbacks_sz + key_callbacks_sz + custom_passes_sz;
		_misc_arena.base_ptr			= ALIGNED_MALLOC(_misc_arena.capacity, 8);
		MEMSET(_misc_arena.base_ptr, 0, _misc_arena.capacity);

		_hover_callbacks = reinterpret_cast<hover_callback*>(_misc_arena.base_ptr);
		_mouse_callbacks = reinterpret_cast<mouse_callback*>(reinterpret_cast<unsigned char*>(_misc_arena.base_ptr) + hover_callbacks_sz);
		_key_callbacks	 = reinterpret_cast<key_callback*>(reinterpret_cast<unsigned char*>(_misc_arena.base_ptr) + hover_callbacks_sz + mouse_callbacks_sz);
		_custom_passes	 = reinterpret_cast<custom_passes*>(reinterpret_cast<unsigned char*>(_misc_arena.base_ptr) + hover_callbacks_sz + mouse_callbacks_sz + key_callbacks_sz);

		for (size_t i = 0; i < _widget_count; i++)
		{
			new (&_metas[i]) widget_meta{};
			new (&_pos_properties[i]) pos_props{};
			new (&_size_properties[i]) size_props{};
			new (&_pos_results[i]) pos_result{};
			new (&_size_results[i]) size_result{};
			new (&_gfxs[i]) widget_gfx{};
			new (&_strokes[i]) stroke_props{};
			new (&_second_colors[i]) second_color_props{};
			new (&_roundings[i]) rounding_props{};
			new (&_aa_props[i]) aa_props{};
			new (&_texts[i]) text_props{};
			new (&_hover_callbacks[i]) hover_callback{};
			new (&_mouse_callbacks[i]) mouse_callback{};
			new (&_key_callbacks[i]) key_callback{};
			new (&_custom_passes[i]) custom_passes{};
		}

		const size_t vertex_count = conf.vertex_buffer_sz / sizeof(vertex);
		const size_t index_count  = conf.index_buffer_sz / sizeof(index);
		_vertex_count_per_buffer  = static_cast<unsigned int>(vertex_count / conf.buffer_count);
		_index_count_per_buffer	  = static_cast<unsigned int>(index_count / conf.buffer_count);
		_buffer_count			  = conf.buffer_count;

		const size_t cache_vertex_count = conf.text_cache_vertex_buffer_sz / sizeof(vertex);
		const size_t cache_index_count	= conf.text_cache_index_buffer_sz / sizeof(index);

		_vertex_buffer			  = reinterpret_cast<vertex*>(MALLOC(sizeof(vertex) * vertex_count));
		_index_buffer			  = reinterpret_cast<index*>(MALLOC(sizeof(index) * index_count));
		_text_cache_vertex_buffer = reinterpret_cast<vertex*>(MALLOC(sizeof(vertex) * cache_vertex_count));
		_text_cache_index_buffer  = reinterpret_cast<index*>(MALLOC(sizeof(index) * cache_index_count));
		_text_cache_vertex_size	  = cache_vertex_count;
		_text_cache_index_size	  = cache_index_count;

		for (size_t i = 0; i < vertex_count; i++)
			new (&_vertex_buffer[i]) vertex();
		for (size_t i = 0; i < index_count; i++)
			new (&_index_buffer[i]) index();

		_total_sz = _layout_arena.capacity + _gfx_arena.capacity + _misc_arena.capacity + conf.vertex_buffer_sz + conf.index_buffer_sz + conf.text_cache_vertex_buffer_sz + conf.text_cache_index_buffer_sz;
		V_LOG("Vekt builder initialized with %d widgets. Total memory reserved: %zu bytes - %0.2f mb", _widget_count, _total_sz, static_cast<f32>(_total_sz) / 1000000.f);
		PUSH_ALLOCATION_SZ(_total_sz);

		_depth_first_widgets.reserve(1024);
		_depth_first_child_info.reserve(1024);
		_depth_first_fill_parents.reserve(1024);
		_depth_first_scrolls.reserve(1024);
		_depth_first_focusables.reserve(1024);
		_draw_order_first_hover_widgets.reserve(1024);
		_draw_order_first_mouse_widgets.reserve(1024);

		_root = allocate();
	}

	void builder::uninit()
	{
		PUSH_DEALLOCATION_SZ(_total_sz);

		deallocate(_root);

		for (size_t i = 0; i < _widget_count; i++)
		{
			_metas[i].~widget_meta();
			_pos_properties[i].~pos_props();
			_size_properties[i].~size_props();
			_pos_results[i].~pos_result();
			_size_results[i].~size_result();
			_gfxs[i].~widget_gfx();
			_strokes[i].~stroke_props();
			_second_colors[i].~second_color_props();
			_roundings[i].~rounding_props();
			_aa_props[i].~aa_props();
			_texts[i].~text_props();
			_hover_callbacks[i].~hover_callback();
			_mouse_callbacks[i].~mouse_callback();
			_key_callbacks[i].~key_callback();
			_custom_passes[i].~custom_passes();
		}

		ALIGNED_FREE(_layout_arena.base_ptr);
		ALIGNED_FREE(_gfx_arena.base_ptr);
		ALIGNED_FREE(_misc_arena.base_ptr);
		_layout_arena.base_ptr = nullptr;
		_gfx_arena.base_ptr	   = nullptr;
		_misc_arena.base_ptr   = nullptr;

		if (_vertex_buffer)
			FREE(_vertex_buffer);
		if (_index_buffer)
			FREE(_index_buffer);
		if (_text_cache_vertex_buffer)
			FREE(_text_cache_vertex_buffer);
		if (_text_cache_index_buffer)
			FREE(_text_cache_index_buffer);

		_vertex_buffer = nullptr;
		_index_buffer  = nullptr;
	}

	void builder::build_begin(const VEKT_VEC2& screen_size)
	{
		_draw_buffers.resize(0);
		_clip_stack.resize_explicit(0);
		_buffer_counter = 0;

		/* size & pos & draw */
		builder& bd = *this;

		pos_props& root_pos = _pos_properties[_root];
		root_pos.pos		= VEKT_VEC2();
		root_pos.flags |= pos_flags::pf_x_abs | pos_flags::pf_y_abs;

		size_props& root_size = _size_properties[_root];
		root_size.size		  = screen_size;
		root_size.flags |= size_flags::sf_x_abs | size_flags::sf_y_abs;

		calculate_sizes();
		calculate_positions();

		_clip_stack.resize_explicit(0);
		_clip_stack.push_back({{0.0f, 0.0f, screen_size.x, screen_size.y}, 0});
		calculate_draw();
	}

	void builder::build_end()
	{
		_clip_stack.pop_back();
		std::sort(_draw_buffers.begin(), _draw_buffers.end(), [](const draw_buffer& a, const draw_buffer& b) { return a.draw_order < b.draw_order; });
	}

	void builder::widget_add_child(id widget_id, id child_id)
	{
		widget_meta& meta		= _metas[widget_id];
		widget_meta& child_meta = _metas[child_id];

		if (child_meta.parent != NULL_WIDGET_ID)
			widget_remove_child(child_meta.parent, child_id);

		child_meta.parent = widget_id;
		meta.children.push_back(child_id);

		// build_hierarchy();
	}

	void builder::widget_remove_child(id widget_id, id child_id)
	{
		widget_meta& meta		= _metas[widget_id];
		widget_meta& child_meta = _metas[child_id];

		meta.children.remove(child_id);
		child_meta.parent = NULL_WIDGET_ID;

		// build_hierarchy();
	}

	void builder::widget_set_text(id w, const char* text, size_t default_cap)
	{
		text_props& tp = _texts[w];

#ifdef VEKT_STRING_CSTR

		if (tp.text == nullptr)
		{
			ASSERT(_on_allocate_text != nullptr);
			ASSERT(_on_deallocate_text != nullptr);
			tp.text			 = _on_allocate_text(_callback_user_data, default_cap == 0 ? (strlen(text) + 1) : default_cap);
			tp.text_capacity = default_cap == 0 ? (strlen(text) + 1) : default_cap;
		}

		ASSERT((strlen(text) + 1) <= tp.text_capacity);
		char* b = (char*)tp.text;
		SFG::char_util::append(b, b + tp.text_capacity, text);
#else
		tp.text = text;
#endif

		widget_update_text(w);
	}

	void builder::widget_append_text_start(id widget)
	{
		text_props& tp = _texts[widget];
		tp.append_ctr  = 0;
		char c		   = '\0';
		MEMCPY((void*)tp.text, &c, 1);
	}

	void builder::widget_append_text(id widget, f32 f, int precision, size_t default_cap)
	{
		text_props& tp = _texts[widget];

#ifdef VEKT_STRING_CSTR
		if (tp.text == nullptr)
		{
			ASSERT(_on_allocate_text != nullptr);
			ASSERT(_on_deallocate_text != nullptr);
			tp.text			 = _on_allocate_text(_callback_user_data, default_cap == 0 ? 64 : default_cap);
			tp.text_capacity = default_cap == 0 ? 64 : default_cap;
		}

		char* end		   = (char*)tp.text + tp.text_capacity;
		char* start_append = (char*)tp.text + tp.append_ctr;
		char* start		   = start_append;

		SFG::char_util::append_double(start_append, end, f, precision);
		tp.append_ctr += (start_append - start);
#else
		tp.text = std::to_string(f);
#endif
		widget_update_text(widget);
	}

	void builder::widget_append_text(id widget, unsigned int i, size_t default_cap)
	{
		text_props& tp = _texts[widget];

#ifdef VEKT_STRING_CSTR
		if (tp.text == nullptr)
		{
			ASSERT(_on_allocate_text != nullptr);
			ASSERT(_on_deallocate_text != nullptr);
			tp.text			 = _on_allocate_text(_callback_user_data, default_cap == 0 ? 64 : default_cap);
			tp.text_capacity = default_cap == 0 ? 64 : default_cap;
		}

		char* end		   = (char*)tp.text + tp.text_capacity;
		char* start_append = (char*)tp.text + tp.append_ctr;
		char* start		   = start_append;

		SFG::char_util::append_u32(start_append, end, i);
		tp.append_ctr += (start_append - start);

#else
		tp.text = std::to_string(i);
#endif
		widget_update_text(widget);
	}

	void builder::widget_append_text(id widget, const char* cstr, size_t default_cap)
	{
		text_props& tp = _texts[widget];

#ifdef VEKT_STRING_CSTR
		if (tp.text == nullptr)
		{
			ASSERT(_on_allocate_text != nullptr);
			ASSERT(_on_deallocate_text != nullptr);
			tp.text			 = _on_allocate_text(_callback_user_data, default_cap == 0 ? (strlen(cstr) + 1) : default_cap);
			tp.text_capacity = default_cap == 0 ? (strlen(cstr) + 1) : default_cap;
		}

		char* end		   = (char*)tp.text + tp.text_capacity;
		char* start_append = (char*)tp.text + tp.append_ctr;
		char* start		   = start_append;
		SFG::char_util::append(start_append, end, cstr);
		tp.append_ctr += (start_append - start);

#else
		tp.text = std::to_string(i);
#endif
		widget_update_text(widget);
	}

	VEKT_VEC4 builder::widget_get_clip(id widget_id) const
	{
		const size_result& sz = _size_results[widget_id];
		const pos_result&  ps = _pos_results[widget_id];
		return {ps.pos.x, ps.pos.y, sz.size.x, sz.size.y};
	}

	widget_gfx& builder::widget_get_gfx(id widget)
	{
		return _gfxs[widget];
	}
	stroke_props& builder::widget_get_stroke(id widget)
	{
		return _strokes[widget];
	}
	rounding_props& builder::widget_get_rounding(id widget)
	{
		return _roundings[widget];
	}
	aa_props& builder::widget_get_aa(id widget)
	{
		return _aa_props[widget];
	}
	second_color_props& builder::widget_get_second_color(id widget)
	{
		return _second_colors[widget];
	}
	input_color_props& builder::widget_get_input_colors(id widget)
	{
		return _input_color_properties[widget];
	}
	text_props& builder::widget_get_text(id widget)
	{
		return _texts[widget];
	}

	mouse_callback& builder::widget_get_mouse_callbacks(id widget)
	{
		return _mouse_callbacks[widget];
	}

	key_callback& builder::widget_get_key_callbacks(id widget)
	{
		return _key_callbacks[widget];
	}

	hover_callback& builder::widget_get_hover_callbacks(id widget)
	{
		return _hover_callbacks[widget];
	}

	widget_user_data& builder::widget_get_user_data(id widget)
	{
		return _user_datas[widget];
	}

	custom_passes& builder::widget_get_custom_pass(id widget)
	{
		return _custom_passes[widget];
	}

	const widget_meta& builder::widget_get_meta(id widget)
	{
		return _metas[widget];
	}

	id builder::widget_get_child(id widget, unsigned int index)
	{
		return _metas[widget].children[index];
	}

	void builder::widget_update_text(id widget)
	{
		widget_gfx& gfx = _gfxs[widget];
		gfx.flags &= ~gfx_is_rect;
		gfx.flags |= gfx_is_text;

		size_props& sz = _size_properties[widget];
		sz.flags	   = sf_x_abs | sf_y_abs;

		text_props& props = _texts[widget];
		sz.size			  = get_text_size(props);
	}

	void builder::widget_set_visible(id widget, bool is_visible)
	{
		if (is_visible)
		{
			_gfxs[widget].flags &= ~gfx_invisible;
			// _size_properties[widget].flags &= ~size_flags::sf_invisible;
			// _pos_properties[widget].flags &= ~pos_flags::pf_invisible;
		}
		else
		{
			_gfxs[widget].flags |= gfx_invisible;
			// _size_properties[widget].flags |= size_flags::sf_invisible;
			// _pos_properties[widget].flags |= pos_flags::pf_invisible;
		}
	}

	bool builder::widget_get_visible(id widget) const
	{
		return _gfxs[widget].flags & gfx_invisible;
	}

	id builder::allocate()
	{
		if (!_free_list.empty())
		{
			const id w = _free_list.get_back();
			_free_list.pop_back();
			return w;
		}

		const id idx = static_cast<id>(_widget_head);
		_widget_head++;
		ASSERT(_widget_head < _widget_count);

		return idx;
	}

	void builder::deallocate_impl(id w)
	{
		widget_meta& meta = _metas[w];

		text_props& tp = _texts[w];

		if (w == _pressed_widget)
			set_pressing(NULL_WIDGET_ID, _pressed_button, {});
		if (w == _focused_widget)
			set_focus(NULL_WIDGET_ID, false);

#ifdef VEKT_STRING_CSTR
		if (tp.text_capacity != 0 && tp.text)
		{
			ASSERT(_on_deallocate_text);
			_on_deallocate_text(_callback_user_data, tp.text);
			tp.text			 = nullptr;
			tp.text_capacity = 0;
		}
#endif
		for (id c : meta.children)
			deallocate_impl(c);

		_user_datas[w]			   = widget_user_data();
		_input_color_properties[w] = input_color_props();
		_scroll_properties[w]	   = scroll_props();
		_metas[w]				   = widget_meta{};
		_size_properties[w]		   = size_props{};
		_pos_properties[w]		   = pos_props{};
		_size_results[w]		   = size_result{};
		_pos_results[w]			   = pos_result{};
		_gfxs[w]				   = widget_gfx{};
		_strokes[w]				   = stroke_props{};
		_second_colors[w]		   = second_color_props{};
		_roundings[w]			   = rounding_props{};
		_aa_props[w]			   = aa_props{};
		_texts[w]				   = text_props{};
		_hover_callbacks[w]		   = hover_callback{};
		_mouse_callbacks[w]		   = mouse_callback{};
		_key_callbacks[w]		   = key_callback{};
		_custom_passes[w]		   = custom_passes{};

		_free_list.push_back(w);
	}

	void builder::deallocate(id w)
	{
		widget_meta& meta = _metas[w];

		if (meta.parent != NULL_WIDGET_ID)
		{
			widget_meta& parent_meta = _metas[meta.parent];
			parent_meta.children.remove(w);
		}

		deallocate_impl(w);

		// build_hierarchy();
	}

	void builder::clear_text_cache()
	{
		_text_cache.resize_explicit(0);
	}

	void builder::set_focus(id widget, bool from_nav)
	{
		if (widget == _focused_widget)
			return;

		const id prev_focused = _focused_widget;
		if (_focused_widget != NULL_WIDGET_ID)
		{
			hover_callback& hb = _hover_callbacks[_focused_widget];
			hb.is_focused	   = false;

			if (hb.on_focus_lost)
				hb.on_focus_lost(this, _focused_widget);
		}

		_focused_widget = widget;
		if (_focused_widget != NULL_WIDGET_ID)
		{
			hover_callback& hb = _hover_callbacks[_focused_widget];
			hb.is_focused	   = true;
			if (hb.on_focus_gained && prev_focused != _focused_widget)
				hb.on_focus_gained(this, _focused_widget, from_nav);
		}
	}

	void builder::set_pressing(id widget, unsigned int button, const VEKT_VEC2& press_pos)
	{
		if (widget == _pressed_widget)
			return;

		_pressed_button = button;

		if (_pressed_widget != NULL_WIDGET_ID)
		{
			_hover_callbacks[_pressed_widget].is_pressing = 0;
		}

		_pressed_widget = widget;
		if (_pressed_widget != NULL_WIDGET_ID)
		{
			_hover_callbacks[_pressed_widget].is_pressing = 1;
			_press_relative_pos							  = press_pos - _pos_results[_pressed_widget].pos;
		}

		// if (widget != NULL_WIDGET_ID)
		// {
		// 	mouse_callback& mb			  = _mouse_callbacks[widget];
		// 	const bool		receive_mouse = mb.on_context_mouse || mb.on_drag;
		// 	if (!receive_mouse)
		// 		return;
		// }
	}

	unsigned int builder::count_total_children(id widget_id) const
	{
		unsigned int	   count = 0;
		const widget_meta& meta	 = _metas[widget_id];
		for (id child : meta.children)
		{
			count++;
			count += count_total_children(child);
		}
		return count;
	}

	void builder::populate_hierarchy(id current_widget_id, unsigned int depth)
	{
		_depth_first_widgets.push_back(current_widget_id);
		_depth_first_child_info.push_back({current_widget_id, depth, count_total_children(current_widget_id)});

		widget_meta&	   current	   = _metas[current_widget_id];
		const unsigned int child_depth = ++depth;
		bool			   pushed	   = false;

		scroll_props& sc = _scroll_properties[current_widget_id];

		hover_callback& hb			  = _hover_callbacks[current_widget_id];
		mouse_callback& mb			  = _mouse_callbacks[current_widget_id];
		const bool		receive_mouse = mb.on_mouse || mb.on_drag;
		const bool		receive_hover = hb.on_hover_begin || hb.on_hover_end || hb.on_hover_move || (widget_get_gfx(current_widget_id).flags & gfx_flags::gfx_has_hover_color);
		const bool		receive_wheel = mb.on_mouse_wheel;
		const bool		is_hovered	  = hb.is_hovered;
		const bool		is_pressing	  = hb.is_pressing;

		if (!receive_mouse)
			hb.is_pressing = 0;

		if (!receive_hover)
			hb.is_hovered = 0;

		if (sc.scroll_parent != NULL_WIDGET_ID)
			_depth_first_scrolls.push_back(current_widget_id);

		if (receive_mouse)
			_draw_order_first_mouse_widgets.push_back(current_widget_id);

		if (receive_hover)
			_draw_order_first_hover_widgets.push_back(current_widget_id);

		if (receive_wheel)
			_depth_first_mouse_wheel_widgets.push_back(current_widget_id);

		if (current_widget_id == _pressed_widget && !receive_mouse)
			set_pressing(NULL_WIDGET_ID, _pressed_button, {});

		if (_gfxs[current_widget_id].flags & gfx_flags::gfx_focusable)
			_depth_first_focusables.push_back(current_widget_id);

		for (id child : current.children)
		{
			size_props& sz = _size_properties[child];
			if (!pushed && (sz.flags & size_flags::sf_x_fill || sz.flags & size_flags::sf_y_fill))
			{
				_depth_first_fill_parents.push_back(current_widget_id);
				pushed = false;
			}

			populate_hierarchy(child, child_depth);
		}
	}

	void builder::build_hierarchy()
	{
		_depth_first_mouse_wheel_widgets.resize_explicit(0);
		_depth_first_fill_parents.resize_explicit(0);
		_depth_first_scrolls.resize_explicit(0);
		_depth_first_widgets.resize_explicit(0);
		_depth_first_child_info.resize_explicit(0);
		_depth_first_focusables.resize_explicit(0);
		_draw_order_first_hover_widgets.resize_explicit(0);
		_draw_order_first_mouse_widgets.resize_explicit(0);

		populate_hierarchy(_root, 0);

		std::sort(_draw_order_first_hover_widgets.begin(), _draw_order_first_hover_widgets.end(), [this](vekt::id a, vekt::id b) -> bool { return widget_get_gfx(a).draw_order > widget_get_gfx(b).draw_order; });
		std::sort(_draw_order_first_mouse_widgets.begin(), _draw_order_first_mouse_widgets.end(), [this](vekt::id a, vekt::id b) -> bool { return widget_get_gfx(a).draw_order > widget_get_gfx(b).draw_order; });

		// restore hover states
		calculate_sizes();
		calculate_positions();
		on_mouse_move(_mouse_position, false);
	}

	void builder::calculate_sizes()
	{
		// top-down
		for (id widget : _depth_first_widgets)
		{
			const size_props& sz = _size_properties[widget];

			if (sz.flags & size_flags::sf_custom_pass)
			{
				custom_passes& passes = _custom_passes[widget];
				if (passes.custom_size_pass)
					passes.custom_size_pass(this, widget);
				continue;
			}

			const widget_meta& meta		  = _metas[widget];
			VEKT_VEC2		   final_size = VEKT_VEC2();
			size_result&	   res		  = _size_results[widget];
			if (sz.flags & size_flags::sf_x_abs)
				res.size.x = sz.size.x;
			if (sz.flags & size_flags::sf_y_abs)
				res.size.y = sz.size.y;
		}

		// bottom-up
		const int wsz = static_cast<int>(_depth_first_widgets.size());
		for (int i = wsz - 1; i >= 0; i--)
		{
			const id		   widget = _depth_first_widgets[i];
			const size_props&  sz	  = _size_properties[widget];
			const widget_meta& meta	  = _metas[widget];

			VEKT_VEC2 final_size = VEKT_VEC2();

			const bool touch_x = sz.flags & size_flags::sf_x_max_children || sz.flags & size_flags::sf_x_total_children;
			const bool touch_y = sz.flags & size_flags::sf_y_max_children || sz.flags & size_flags::sf_y_total_children;

			if (touch_x || touch_y)
			{
				for (id child : meta.children)
				{
					pos_props& child_pos_props = _pos_properties[child];
					if (child_pos_props.flags & pos_flags::pf_overlay)
						continue;

					size_props& child_size_props = _size_properties[child];

					const size_result& child_res = _size_results[child];

					if (sz.flags & size_flags::sf_x_max_children && !(child_size_props.flags & size_flags::sf_x_relative))
						final_size.x = math::max(child_res.size.x, final_size.x);
					else if (sz.flags & size_flags::sf_x_total_children && !(child_size_props.flags & size_flags::sf_x_relative))
						final_size.x += child_res.size.x + sz.spacing;

					if (sz.flags & size_flags::sf_y_max_children && !(child_size_props.flags & size_flags::sf_y_relative))
						final_size.y = math::max(child_res.size.y + sz.child_margins.top + sz.child_margins.bottom, final_size.y);
					else if (sz.flags & size_flags::sf_y_total_children && !(child_size_props.flags & size_flags::sf_y_relative))
						final_size.y += child_res.size.y + sz.spacing;
				}

				if (sz.flags & size_flags::sf_x_total_children && !math::equals(final_size.x, 0.0f, 0.001f))
					final_size.x -= sz.spacing;
				if (sz.flags & size_flags::sf_y_total_children && !math::equals(final_size.y, 0.0f, 0.001f))
					final_size.y -= sz.spacing;

				final_size.x += sz.child_margins.left + sz.child_margins.right;
				final_size.y += sz.child_margins.top + sz.child_margins.bottom;

				if (sz.flags & size_flags::sf_x_copy_y)
					final_size.x = final_size.y;
				else if (sz.flags & size_flags::sf_y_copy_x)
					final_size.y = final_size.x;

				size_result& res = _size_results[widget];
				if (touch_x)
					res.size.x = final_size.x;
				if (touch_y)
					res.size.y = final_size.y;
			}
		}

		// top-down
		for (id widget : _depth_first_widgets)
		{
			const size_props& sz = _size_properties[widget];
			if (sz.flags & size_flags::sf_custom_pass)
			{
				custom_passes& passes = _custom_passes[widget];
				if (passes.custom_size_pass)
					passes.custom_size_pass(this, widget);
				continue;
			}

			const widget_meta& meta = _metas[widget];

			const bool	 x_relative = sz.flags & size_flags::sf_x_relative;
			const bool	 y_relative = sz.flags & size_flags::sf_y_relative;
			size_result& res		= _size_results[widget];

			if (x_relative || y_relative)
			{
				const size_props&  parent_sz  = _size_properties[meta.parent];
				const size_result& parent_res = _size_results[meta.parent];
				if (x_relative)
					res.size.x = (parent_res.size.x - parent_sz.child_margins.left - parent_sz.child_margins.right) * sz.size.x;
				if (y_relative)
					res.size.y = (parent_res.size.y - parent_sz.child_margins.top - parent_sz.child_margins.bottom) * sz.size.y;
			}

			if (sz.flags & size_flags::sf_x_copy_y)
				res.size.x = res.size.y;
			else if (sz.flags & size_flags::sf_y_copy_x)
				res.size.y = res.size.x;
		}

		// top-down fill
		for (id widget : _depth_first_fill_parents)
		{
			const size_props& sz = _size_properties[widget];
			if (sz.flags & size_flags::sf_custom_pass)
			{
				custom_passes& passes = _custom_passes[widget];
				if (passes.custom_size_pass)
					passes.custom_size_pass(this, widget);
				continue;
			}

			const size_result& parent_result	= _size_results[widget];
			const pos_props&   parent_pos_props = _pos_properties[widget];
			const size_props&  parent_props		= _size_properties[widget];
			const widget_meta& parent_meta		= _metas[widget];

			const f32 available_size_default_x = parent_result.size.x - parent_props.child_margins.left - parent_props.child_margins.right;
			const f32 available_size_default_y = parent_result.size.y - parent_props.child_margins.top - parent_props.child_margins.bottom;

			if (parent_pos_props.flags & pos_flags::pf_child_pos_row)
			{
				f32			 total		   = 0.0f;
				unsigned int total_count   = 0;
				unsigned int waiting_count = 0;

				for (id c : parent_meta.children)
				{
					const pos_props& child_pos_props = _pos_properties[c];

					if (child_pos_props.flags & pos_flags::pf_overlay)
						continue;

					const size_props& child_props = _size_properties[c];

					if (child_props.flags & size_flags::sf_x_fill)
					{
						waiting_count++;
					}
					else
					{
						size_result& child_result = _size_results[c];
						total += child_result.size.x;
					}
					total_count++;
				}

				const f32 available_size = available_size_default_x - (total_count - 1) * parent_props.spacing - total;
				const f32 size_per_child = available_size / static_cast<f32>(waiting_count);

				for (id c : parent_meta.children)
				{
					const size_props& child_props  = _size_properties[c];
					size_result&	  child_result = _size_results[c];

					if (child_props.flags & size_flags::sf_x_fill)
						child_result.size.x = size_per_child;

					if (child_props.flags & size_flags::sf_y_fill)
						child_result.size.y = available_size_default_y;
				}
			}
			else if (parent_pos_props.flags & pos_flags::pf_child_pos_column)
			{
				f32			 total		   = 0.0f;
				unsigned int total_count   = 0;
				unsigned int waiting_count = 0;

				for (id c : parent_meta.children)
				{
					const pos_props& child_pos_props = _pos_properties[c];

					if (child_pos_props.flags & pos_flags::pf_overlay)
						continue;

					const size_props& child_props = _size_properties[c];
					if (child_props.flags & size_flags::sf_y_fill)
					{
						waiting_count++;
					}
					else
					{
						size_result& child_result = _size_results[c];
						total += child_result.size.y;
					}
					total_count++;
				}

				const f32 available_size = available_size_default_y - (total_count - 1) * parent_props.spacing - total;
				const f32 size_per_child = available_size / static_cast<f32>(waiting_count);

				for (id c : parent_meta.children)
				{
					const size_props& child_props  = _size_properties[c];
					size_result&	  child_result = _size_results[c];

					if (child_props.flags & size_flags::sf_y_fill)
						child_result.size.y = size_per_child;

					if (child_props.flags & size_flags::sf_x_fill)
						child_result.size.x = available_size_default_x;
				}
			}
			else
			{
				for (id c : parent_meta.children)
				{
					const size_props& child_props  = _size_properties[c];
					size_result&	  child_result = _size_results[c];

					if (child_props.flags & size_flags::sf_x_fill)
						child_result.size.x = available_size_default_x;

					if (child_props.flags & size_flags::sf_y_fill)
						child_result.size.x = available_size_default_y;
				}
			}
		}

		for (id widget : _depth_first_scrolls)
		{
			scroll_props& sc = _scroll_properties[widget];
			if (sc.scroll_parent == NULL_WIDGET_ID)
				continue;

			const size_props&  props	   = _size_properties[widget];
			const widget_meta& parent_meta = _metas[sc.scroll_parent];
			const size_props&  parent_size = _size_properties[sc.scroll_parent];

			f32			 total_y				  = 0.0f;
			unsigned int count					  = 0;
			const f32	 available_size_default_y = _size_results[sc.scroll_parent].size.y - parent_size.child_margins.top - parent_size.child_margins.bottom;

			for (id c : parent_meta.children)
			{
				const size_props& sz = _size_properties[c];
				if (c == widget)
					continue;

				if (_pos_properties[c].flags & pos_flags::pf_overlay)
					continue;

				total_y += _size_results[c].size.y;
				count++;
			}

			if (count == 0)
				break;

			const f32 total = total_y + parent_size.spacing * (count - 1);
			const f32 ratio = total / available_size_default_y;

			const widget_meta& meta = _metas[widget];

			_size_results[widget].size.y = ratio > 1.0f ? _size_results[meta.parent].size.y * (1.0f / ratio) : 0.0f;

			const f32  diff = total - available_size_default_y;
			pos_props& pp	= _pos_properties[sc.scroll_parent];

			if (pp.scroll_offset < -diff)
				pp.scroll_offset = -diff;

			if (pp.scroll_offset > 0)
				pp.scroll_offset = 0;
			sc._max_scroll = diff;
		}
	}

	void builder::calculate_positions()
	{
		u32 i = 0;
		for (id widget : _depth_first_widgets)
		{
			pos_props& pp = _pos_properties[widget];
			i++;
			if (pp.flags & pos_flags::pf_custom_pass)
			{
				custom_passes& passes = _custom_passes[widget];
				if (passes.custom_pos_pass)
					passes.custom_pos_pass(this, widget);
				continue;
			}

			const size_props&  sz		 = _size_properties[widget];
			const size_result& sr		 = _size_results[widget];
			const f32		   my_width	 = sr.size.x;
			const f32		   my_height = sr.size.y;

			// set my pos if abs needed.
			pos_result& pr = _pos_results[widget];
			if (pp.flags & pos_flags::pf_x_abs)
				pr.pos.x = pp.pos.x;
			if (pp.flags & pos_flags::pf_y_abs)
				pr.pos.y = pp.pos.y;

			const f32  my_pos_x = pr.pos.x;
			const f32  my_pos_y = pr.pos.y;
			const bool is_row	= pp.flags & pos_flags::pf_child_pos_row;
			const bool is_col	= pp.flags & pos_flags::pf_child_pos_column;

			widget_meta& meta = _metas[widget];

			f32 row_position = my_pos_x + sz.child_margins.left;
			f32 col_position = my_pos_y + sz.child_margins.top + pp.scroll_offset;

			for (id child : meta.children)
			{
				pos_props&	 child_pos_props = _pos_properties[child];
				pos_result&	 child_pr		 = _pos_results[child];
				size_result& child_sr		 = _size_results[child];

				if (child_pos_props.flags & pos_flags::pf_x_relative)
				{
					if (child_pos_props.flags & pos_flags::pf_x_anchor_end)
						child_pr.pos.x = (my_pos_x + my_width - sz.child_margins.right) - child_sr.size.x;
					else if (child_pos_props.flags & pos_flags::pf_x_anchor_center)
						child_pr.pos.x = (my_pos_x) + (my_width * child_pos_props.pos.x) - child_sr.size.x * 0.5f;
					else
						child_pr.pos.x = (my_pos_x + sz.child_margins.left) + (my_width * child_pos_props.pos.x);

					if (is_row && !(child_pos_props.flags & pos_flags::pf_overlay))
						row_position = math::max(row_position, child_pr.pos.x + child_sr.size.x + sz.spacing);
				}
				else if (!(child_pos_props.flags & pos_flags::pf_x_abs) && is_row)
				{
					child_pr.pos.x = row_position;
					row_position += child_sr.size.x + sz.spacing;
				}

				if (child_pos_props.flags & pos_flags::pf_y_relative)
				{
					if (child_pos_props.flags & pos_flags::pf_y_anchor_end)
						child_pr.pos.y = (my_pos_y + my_height - sz.child_margins.bottom) - child_sr.size.y;
					else if (child_pos_props.flags & pos_flags::pf_y_anchor_center)
						child_pr.pos.y = (my_pos_y) + (my_height * child_pos_props.pos.y) - child_sr.size.y * 0.5f;
					else
						child_pr.pos.y = (my_pos_y + sz.child_margins.top) + (my_height * child_pos_props.pos.y);

					if (is_col && !(child_pos_props.flags & pos_flags::pf_overlay))
						col_position = math::max(col_position, child_pr.pos.y + child_sr.size.y + sz.spacing);
				}
				else if (!(child_pos_props.flags & pos_flags::pf_y_abs) && is_col)
				{
					child_pr.pos.y = col_position;
					col_position += child_sr.size.y + sz.spacing;
				}
			}
		}

		for (id widget : _depth_first_scrolls)
		{
			scroll_props& sc = _scroll_properties[widget];
			if (sc.scroll_parent == NULL_WIDGET_ID)
				continue;

			widget_meta& meta = _metas[widget];

			pos_result&	 parent_pr = _pos_results[meta.parent];
			size_result& parent_sr = _size_results[meta.parent];
			size_result& sr		   = _size_results[widget];

			_pos_results[widget].pos.y = math::remap(sc.scroll_ratio, 0.0f, 1.0f, parent_pr.pos.y, parent_pr.pos.y + parent_sr.size.y - sr.size.y);
		}
	}

	void builder::calculate_draw()
	{
		direction color_direction = direction::horizontal;
		bool	  multi_color	  = false;

		const unsigned int sz = _depth_first_child_info.size();

		for (unsigned int i = 1; i < sz;)
		{
			const depth_first_child_info& info	 = _depth_first_child_info[i];
			const id					  widget = info.widget_id;

			// left-over clip stacks from previous widgets, always pop until we reach the depth of the current widget.
			while (_clip_stack.size() > 0 && _clip_stack.get_back().depth >= info.depth)
				_clip_stack.pop_back();

			/*
				Both invisible widgets and clipped widgets skip the dfo list by total children amount.
			*/
			widget_gfx& gfx = _gfxs[widget];

			if (gfx.flags & gfx_invisible)
			{
				i += info.owned_children + 1;
				continue;
			}

			const VEKT_VEC2& pos		  = _pos_results[widget].pos;
			const VEKT_VEC2& size		  = _size_results[widget].size;
			const VEKT_VEC4	 widget_clip  = VEKT_VEC4(pos.x, pos.y, size.x, size.y);
			const VEKT_VEC4& back		  = _clip_stack.get_back().rect;
			const VEKT_VEC4	 intersection = calculate_intersection(back, widget_clip);
			const bool		 has_clip	  = gfx.flags & gfx_flags::gfx_clip_children;
			if (intersection.z <= 0 || intersection.w <= 0)
			{
				i += info.owned_children + 1;
				continue;
			}

			VEKT_VEC4 start_color = gfx.color;
			VEKT_VEC4 end_color	  = VEKT_VEC4();

			multi_color = false;
			if (gfx.flags & gfx_has_second_color)
			{
				second_color_props& p = widget_get_second_color(widget);
				end_color			  = p.color;
				color_direction		  = p.direction;
				multi_color			  = true;
			}

			if (gfx.flags & gfx_has_hover_color)
			{
				hover_callback& hb = _hover_callbacks[widget];
				if (hb.is_hovered)
				{
					start_color = _input_color_properties[widget].hovered_color;
					multi_color = false;
				}
			}

			if (gfx.flags & gfx_has_press_color)
			{
				hover_callback& hb = _hover_callbacks[widget];
				if (hb.is_pressing)
				{
					start_color = _input_color_properties[widget].pressed_color;
					multi_color = false;
				}
			}

			const bool has_aa		= gfx.flags & gfx_has_aa;
			const bool has_outline	= gfx.flags & gfx_has_stroke;
			const bool has_rounding = gfx.flags & gfx_has_rounding;

			VEKT_VEC4 stroke_col = has_outline ? _strokes[widget].color : VEKT_VEC4();

			if (has_outline && (gfx.flags & gfx_focusable))
			{
				if (_hover_callbacks[widget].is_focused)
					stroke_col = _input_color_properties[widget].focus_color;
			}

			const rect_props props = {
				.gfx			 = gfx,
				.min			 = pos,
				.max			 = pos + size,
				.color_start	 = start_color,
				.color_end		 = end_color,
				.stroke_col		 = stroke_col,
				.color_direction = color_direction,
				.widget_id		 = widget,
				.multi_color	 = multi_color,
			};

			if (gfx.flags & gfx_is_rect)
			{
				if (has_aa && has_outline && has_rounding)
					add_filled_rect_aa_outline_rounding(props);
				else if (has_aa && has_outline && !has_rounding)
					add_filled_rect_aa_outline(props);
				else if (has_aa && !has_outline && !has_rounding)
					add_filled_rect_aa(props);
				else if (has_aa && !has_outline && has_rounding)
					add_filled_rect_aa_rounding(props);
				else if (has_outline && !has_aa && !has_rounding)
					add_filled_rect_outline(props);
				else if (has_outline && has_rounding && !has_aa)
					add_filled_rect_rounding_outline(props);
				else if (has_rounding && !has_aa && !has_outline)
					add_filled_rect_rounding(props);
				else
					add_filled_rect(props);
			}
			else if (gfx.flags & gfx_is_stroke)
			{
				if (has_aa && has_rounding)
					add_stroke_rect_aa_rounding(props);
				else if (has_aa && !has_rounding)
					add_stroke_rect_aa(props);
				else if (!has_aa && has_rounding)
					add_stroke_rect_rounding(props);
				else
					add_stroke_rect(props);
			}
			else if (gfx.flags & gfx_is_text)
			{
				add_text(_texts[widget], start_color, pos, size, gfx.draw_order, gfx.user_data);
			}
			else if (gfx.flags & gfx_is_text_cached)
			{
				add_text_cached(_texts[widget], start_color, pos, size, gfx.draw_order, gfx.user_data);
			}

			if (has_clip)
				_clip_stack.push_back({widget_clip, info.depth});

			if (gfx.flags & gfx_flags::gfx_custom_pass)
			{
				custom_passes& passes = _custom_passes[widget];
				if (passes.custom_draw_pass)
					passes.custom_draw_pass(this, widget);
			}

			i++;
		}
	}

	void builder::widget_set_size(id widget_id, const VEKT_VEC2& size, helper_size_type helper_x, helper_size_type helper_y)
	{
		size_props& props = _size_properties[widget_id];
		props.size		  = size;
		props.flags		  = 0;

		switch (helper_x)
		{
		case helper_size_type::absolute:
			props.flags |= size_flags::sf_x_abs;
			break;
		case helper_size_type::relative:
			props.flags |= size_flags::sf_x_relative;
			break;
		case helper_size_type::fill:
			props.flags |= size_flags::sf_x_fill;
			break;
		case helper_size_type::max_children:
			props.flags |= size_flags::sf_x_max_children;
			break;
		case helper_size_type::total_children:
			props.flags |= size_flags::sf_x_total_children;
			break;
		case helper_size_type::copy_other:
			props.flags |= size_flags::sf_x_copy_y;
			break;
		default:
			break;
		}

		switch (helper_y)
		{
		case helper_size_type::absolute:
			props.flags |= size_flags::sf_y_abs;
			break;
		case helper_size_type::relative:
			props.flags |= size_flags::sf_y_relative;
			break;
		case helper_size_type::fill:
			props.flags |= size_flags::sf_y_fill;
			break;
		case helper_size_type::max_children:
			props.flags |= size_flags::sf_y_max_children;
			break;
		case helper_size_type::total_children:
			props.flags |= size_flags::sf_y_total_children;
			break;
		case helper_size_type::copy_other:
			props.flags |= size_flags::sf_y_copy_x;
			break;
		default:
			break;
		}
	}

	void builder::widget_set_size_abs(id widget_id, const VEKT_VEC2& size)
	{
		size_props& props = _size_properties[widget_id];
		props.size		  = size;
		props.flags |= size_flags::sf_x_abs;
		props.flags |= size_flags::sf_y_abs;
	}

	void builder::widget_set_pos(id widget_id, const VEKT_VEC2& pos, helper_pos_type helper_x, helper_pos_type helper_y, helper_anchor_type anchor_x, helper_anchor_type anchor_y)
	{
		pos_props& props = _pos_properties[widget_id];
		props.pos		 = pos;
		props.flags &= ~pos_flags::pf_x_relative;
		props.flags &= ~pos_flags::pf_y_relative;
		props.flags &= ~pos_flags::pf_x_abs;
		props.flags &= ~pos_flags::pf_y_abs;
		props.flags &= ~pos_flags::pf_x_anchor_center;
		props.flags &= ~pos_flags::pf_y_anchor_center;
		props.flags &= ~pos_flags::pf_x_anchor_end;
		props.flags &= ~pos_flags::pf_y_anchor_end;

		switch (helper_x)
		{
		case helper_pos_type::relative:
			props.flags |= pos_flags::pf_x_relative;
			break;
		case helper_pos_type::absolute:
			props.flags |= pos_flags::pf_x_abs;
			break;
		default:
			break;
		}

		switch (anchor_x)
		{
		case helper_anchor_type::center:
			props.flags |= pos_flags::pf_x_anchor_center;
			break;
		case helper_anchor_type::end:
			props.flags |= pos_flags::pf_x_anchor_end;
			break;
		default:
			break;
		}

		switch (helper_y)
		{
		case helper_pos_type::relative:
			props.flags |= pos_flags::pf_y_relative;
			break;
		case helper_pos_type::absolute:
			props.flags |= pos_flags::pf_y_abs;
			break;
		default:
			break;
		}

		switch (anchor_y)
		{
		case helper_anchor_type::center:
			props.flags |= pos_flags::pf_y_anchor_center;
			break;
		case helper_anchor_type::end:
			props.flags |= pos_flags::pf_y_anchor_end;
			break;
		default:
			break;
		}
	}

	void builder::widget_set_pos_abs(id widget_id, const VEKT_VEC2& pos)
	{
		pos_props& props = _pos_properties[widget_id];
		props.pos		 = pos;
		props.flags |= pos_flags::pf_x_abs;
		props.flags |= pos_flags::pf_y_abs;
	}

	const VEKT_VEC2& builder::widget_get_size(id widget_id) const
	{
		return _size_results[widget_id].size;
	}

	const VEKT_VEC2& builder::widget_get_pos(id widget_id) const
	{
		return _pos_results[widget_id].pos;
	}

	size_props& builder::widget_get_size_props(id widget_id)
	{
		return _size_properties[widget_id];
	}

	scroll_props& builder::widget_get_scroll_props(id widget_id)
	{
		return _scroll_properties[widget_id];
	}

	pos_props& builder::widget_get_pos_props(id widget_id)
	{
		return _pos_properties[widget_id];
	}

	void builder::widget_set_scroll_offset(id widget_id, f32 offset)
	{
		_pos_properties[widget_id].scroll_offset = offset;
	}

	void builder::on_mouse_move(const VEKT_VEC2& mouse, bool send_events)
	{
		const VEKT_VEC2 delta = mouse - _mouse_position;
		_mouse_position		  = mouse;

		bool first_hover = false;

		for (id widget : _draw_order_first_hover_widgets)
		{
			hover_callback& hover_state = _hover_callbacks[widget];
			const VEKT_VEC4 clip		= widget_get_clip(widget);
			const bool		hovered		= !first_hover && clip.is_point_inside(mouse.x, mouse.y);

			if (hovered && !first_hover)
				first_hover = true;

			if (send_events && hover_state.on_hover_end && !hovered && hover_state.is_hovered)
				hover_state.on_hover_end(this, widget);

			if (send_events && hover_state.on_hover_begin && hovered && !hover_state.is_hovered)
				hover_state.on_hover_begin(this, widget);

			if (hovered && hover_state.on_hover_move)
				hover_state.on_hover_move(this, widget);

			hover_state.is_hovered = hovered;
		}

		if (_pressed_widget != NULL_WIDGET_ID)
		{
			scroll_props& sc	   = _scroll_properties[_pressed_widget];
			size_result&  thumb_sr = _size_results[_pressed_widget];

			if (sc.scroll_parent != NULL_WIDGET_ID)
			{
				pos_result&	 track_pr = _pos_results[sc.scroll_parent];
				size_result& track_sr = _size_results[sc.scroll_parent];

				const f32 track_top	   = track_pr.pos.y;
				const f32 track_height = track_sr.size.y;
				const f32 thumb_h	   = thumb_sr.size.y;
				const f32 travel	   = math::max(0.0f, track_height - thumb_h);
				const f32 grab_y	   = _press_relative_pos.y;
				f32		  thumb_top	   = mouse.y - grab_y;
				thumb_top			   = math::clamp(thumb_top, track_top, track_top + travel);

				const f32  ratio	 = (travel > 0.0f) ? ((thumb_top - track_top) / travel) : 0.0f;
				pos_props& parent_pp = _pos_properties[sc.scroll_parent];

				parent_pp.scroll_offset = -ratio * sc._max_scroll;
				sc.scroll_ratio			= ratio;
			}

			mouse_callback& mc = _mouse_callbacks[_pressed_widget];
			if (mc.on_drag && send_events)
				mc.on_drag(this, _pressed_widget, _mouse_position.x, _mouse_position.y, delta.x, delta.y, _pressed_button);
		}
	}

	input_event_result builder::on_mouse_event(const mouse_event& ev)
	{
		bool handled = false;

		if (ev.type == input_event_type::released)
			set_pressing(NULL_WIDGET_ID, ev.button, {});
		else if (ev.type == input_event_type::pressed)
		{
			set_focus(NULL_WIDGET_ID, false);
			set_pressing(NULL_WIDGET_ID, ev.button, ev.position);
		}

		if (ev.type == input_event_type::pressed)
		{
			for (id sc : _depth_first_scrolls)
			{
				const VEKT_VEC4 clip	= widget_get_clip(sc);
				const bool		hovered = clip.is_point_inside(_mouse_position.x, _mouse_position.y);
				if (hovered)
				{
					set_pressing(sc, ev.button, ev.position);
					return input_event_result::handled;
				}
			}
		}

		for (id widget : _draw_order_first_mouse_widgets)
		{
			const VEKT_VEC4 clip	= widget_get_clip(widget);
			const bool		hovered = clip.is_point_inside(ev.position.x, ev.position.y);
			if (!hovered)
				continue;

			mouse_callback& cb = _mouse_callbacks[widget];
			if (!cb.on_mouse && !cb.on_drag)
				continue;

			if (ev.type == input_event_type::pressed)
			{
				set_focus(widget, false);
				set_pressing(widget, ev.button, ev.position);
			}

			if (cb.on_mouse)
			{
				const input_event_result res = cb.on_mouse(this, widget, ev, input_event_phase::tunneling);
				if (res == input_event_result::handled)
				{

					return input_event_result::handled;
				}
			}
		}

		return input_event_result::not_handled;
	}

	input_event_result builder::on_mouse_wheel_event(const mouse_wheel_event& ev)
	{
		for (id sc : _depth_first_scrolls)
		{
			scroll_props& sp = _scroll_properties[sc];
			const id	  p	 = sp.scroll_parent;
			if (p == NULL_WIDGET_ID)
				continue;

			const VEKT_VEC4 clip	= widget_get_clip(p);
			const bool		hovered = clip.is_point_inside(_mouse_position.x, _mouse_position.y);
			if (hovered)
			{
				_pos_properties[p].scroll_offset -= ev.amount;
				sp.scroll_ratio = _pos_properties[p].scroll_offset / -sp._max_scroll;
			}
		}

		for (id widget : _depth_first_mouse_wheel_widgets)
		{
			hover_callback& hb = _hover_callbacks[widget];
			if (!hb.is_hovered)
				continue;

			mouse_callback&			 mc	 = _mouse_callbacks[widget];
			const input_event_result res = mc.on_mouse_wheel(this, widget, ev);
			if (res == input_event_result::handled)
				return input_event_result::handled;
		}

		return input_event_result::not_handled;
	}

	input_event_result builder::on_key_event(const key_event& ev)
	{
		if (_focused_widget == NULL_WIDGET_ID)
			return input_event_result::not_handled;

		key_callback& ks = _key_callbacks[_focused_widget];

		if (!ks.on_key)
			return input_event_result::not_handled;

		return ks.on_key(this, _focused_widget, ev);
	}

	void builder::next_focus()
	{
		if (_focused_widget == NULL_WIDGET_ID)
			return;

		unsigned int index = 0;
		for (id w : _depth_first_focusables)
		{
			if (w == _focused_widget)
			{
				if (index == _depth_first_focusables.size() - 1 && !_depth_first_focusables.empty())
				{
					set_focus(_depth_first_focusables[0], true);
					return;
				}
				else if (!_depth_first_focusables.empty())
				{
					set_focus(_depth_first_focusables[index + 1], true);
					return;
				}
			}
			index++;
		}
	}

	void builder::prev_focus()
	{
		if (_focused_widget == NULL_WIDGET_ID)
			return;

		unsigned int index = 0;
		for (id w : _depth_first_focusables)
		{
			if (w == _focused_widget)
			{
				if (index == 0 && !_depth_first_focusables.empty())
				{
					set_focus(_depth_first_focusables[_depth_first_focusables.size() - 1], true);
					return;
				}
				else if (!_depth_first_focusables.empty())
				{
					set_focus(_depth_first_focusables[index - 1], true);
					return;
				}
			}
			index++;
		}
	}

	void builder::add_line(const line_props& props)
	{
		// Build a quad around the segment using thickness
		const VEKT_VEC2 dir = (props.p1 - props.p0).normalized();
		const VEKT_VEC2 nrm(-dir.y, dir.x);
		const f32		half_t = props.thickness * 0.5f;
		const VEKT_VEC2 off	   = nrm * half_t;

		VEKT_VEC2 q0 = props.p0 - off;
		VEKT_VEC2 q1 = props.p0 + off;
		VEKT_VEC2 q2 = props.p1 + off;
		VEKT_VEC2 q3 = props.p1 - off;

		// Simple fix: swap q0 and q2
		_reuse_outer_path.resize_explicit(0);
		_reuse_outer_path.push_back(q2);
		_reuse_outer_path.push_back(q1);
		_reuse_outer_path.push_back(q0);
		_reuse_outer_path.push_back(q3);

		VEKT_VEC2 bb_min = _reuse_outer_path[0];
		VEKT_VEC2 bb_max = _reuse_outer_path[0];
		for (unsigned int i = 1; i < _reuse_outer_path.size(); ++i)
		{
			const VEKT_VEC2& p = _reuse_outer_path[i];
			bb_min.x		   = math::min(bb_min.x, p.x);
			bb_min.y		   = math::min(bb_min.y, p.y);
			bb_max.x		   = math::max(bb_max.x, p.x);
			bb_max.y		   = math::max(bb_max.y, p.y);
		}

		draw_buffer*	   db	 = get_draw_buffer(props.draw_order, props.user_data);
		const unsigned int start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.color, bb_min, bb_max);
		add_filled_rect(db, start);
	}

	void builder::add_line_aa(const line_aa_props& props)
	{
		const VEKT_VEC2 dir = (props.p1 - props.p0).normalized();
		const VEKT_VEC2 nrm(-dir.y, dir.x);
		const f32		half_t = props.thickness * 0.5f;
		const VEKT_VEC2 off	   = nrm * half_t;

		VEKT_VEC2 q0 = props.p0 - off;
		VEKT_VEC2 q1 = props.p0 + off;
		VEKT_VEC2 q2 = props.p1 + off;
		VEKT_VEC2 q3 = props.p1 - off;

		_reuse_outer_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);
		_reuse_outer_path.push_back(q2);
		_reuse_outer_path.push_back(q1);
		_reuse_outer_path.push_back(q0);
		_reuse_outer_path.push_back(q3);

		VEKT_VEC2 bb_min = _reuse_outer_path[0];
		VEKT_VEC2 bb_max = _reuse_outer_path[0];
		for (unsigned int i = 1; i < _reuse_outer_path.size(); ++i)
		{
			const VEKT_VEC2& p = _reuse_outer_path[i];
			bb_min.x		   = math::min(bb_min.x, p.x);
			bb_min.y		   = math::min(bb_min.y, p.y);
			bb_max.x		   = math::max(bb_max.x, p.x);
			bb_max.y		   = math::max(bb_max.y, p.y);
		}

		// Expand outward for AA ring
		generate_offset_rect(_reuse_aa_outer_path, _reuse_outer_path, -static_cast<f32>(props.aa_thickness));

		draw_buffer*	   db		 = get_draw_buffer(props.draw_order, props.user_data);
		const unsigned int out_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.color, bb_min, bb_max);
		add_filled_rect(db, out_start);

		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, out_start, 0.0f, bb_min, bb_max);
		add_strip(db, out_aa_start, out_start, _reuse_aa_outer_path.size(), false);
	}

	void builder::generate_circle_path(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& center, f32 radius, unsigned int segments)
	{
		if (segments < 3)
			segments = 3;
		out_path.resize_explicit(0);
		for (unsigned int i = 0; i < segments; ++i)
		{
			const f32		ang	  = DEG_2_RAD * (360.0f * (static_cast<f32>(i) / static_cast<f32>(segments)));
			const VEKT_VEC2 point = center + VEKT_VEC2(math::sin(ang), -math::cos(ang)) * radius;
			out_path.push_back(point);
		}
	}

	void builder::add_circle(const circle_props& props)
	{
		_reuse_outer_path.resize_explicit(0);
		_reuse_inner_path.resize_explicit(0);

		generate_circle_path(_reuse_outer_path, props.center, props.radius, props.segments);

		const VEKT_VEC2 bb_min = props.center - VEKT_VEC2(props.radius, props.radius);
		const VEKT_VEC2 bb_max = props.center + VEKT_VEC2(props.radius, props.radius);

		draw_buffer* db = get_draw_buffer(props.draw_order, props.user_data);

		if (props.filled)
		{
			const unsigned int out_start = db->vertex_count;
			add_vertices(db, _reuse_outer_path, props.color, bb_min, bb_max);
			const unsigned int central_start = db->vertex_count;
			add_central_vertex(db, props.color, bb_min, bb_max);
			add_filled_rect_central(db, out_start, central_start, _reuse_outer_path.size());
		}
		else
		{
			const f32 t = math::min(props.thickness, props.radius);
			generate_offset_rect(_reuse_inner_path, _reuse_outer_path, t);
			const unsigned int out_start = db->vertex_count;
			add_vertices(db, _reuse_outer_path, props.color, bb_min, bb_max);
			const unsigned int in_start = db->vertex_count;
			add_vertices(db, _reuse_inner_path, props.color, bb_min, bb_max);
			add_strip(db, out_start, in_start, _reuse_outer_path.size(), false);
		}
	}

	void builder::add_sphere(const sphere_props& props)
	{
		circle_props cp;
		cp.center	  = props.center;
		cp.radius	  = props.radius;
		cp.color	  = props.color;
		cp.segments	  = props.segments;
		cp.filled	  = true;
		cp.draw_order = props.draw_order;
		cp.user_data  = props.user_data;
		add_circle(cp);
	}

	void builder::add_filled_rect(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		_reuse_outer_path.resize_explicit(0);
		generate_sharp_rect(_reuse_outer_path, props.min, props.max);

		const unsigned int out_start = db->vertex_count;

		if (props.multi_color)
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
		else
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);

		add_filled_rect(db, out_start);
	}

	void builder::add_filled_rect_aa(const rect_props& props)
	{
		_reuse_outer_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		aa_props& p = _aa_props[props.widget_id];

		generate_sharp_rect(_reuse_outer_path, props.min, props.max);
		generate_offset_rect(_reuse_aa_outer_path, _reuse_outer_path, -static_cast<f32>(p.thickness));

		draw_buffer*	   db		 = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		const unsigned int out_start = db->vertex_count;

		if (props.multi_color)
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
		else
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);

		add_filled_rect(db, out_start);

		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, out_start, 0.0f, props.min, props.max);
		add_strip(db, out_aa_start, out_start, _reuse_aa_outer_path.size(), false);
	}

	void builder::add_filled_rect_outline(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);

		stroke_props& out_p = _strokes[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_outline_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		generate_sharp_rect(_reuse_outer_path, props.min, props.max);

		generate_offset_rect(_reuse_outline_path, _reuse_outer_path, -static_cast<f32>(out_p.thickness));

		const unsigned int out_start = db->vertex_count;

		if (props.multi_color)
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
		else
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);

		add_filled_rect(db, out_start);

		unsigned int outline_start = 0;
		// add original vertices
		const unsigned int copy_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.stroke_col, props.min, props.max);

		outline_start = db->vertex_count;
		add_vertices(db, _reuse_outline_path, props.stroke_col, props.min, props.max);
		add_strip(db, outline_start, copy_start, _reuse_outline_path.size(), false);
	}

	void builder::add_filled_rect_rounding(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);

		rounding_props& rp = _roundings[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_outline_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		const bool has_rounding = rp.rounding > 0.0f;
		generate_rounded_rect(_reuse_outer_path, props.min, props.max, rp.rounding, rp.segments);

		const unsigned int out_start	 = db->vertex_count;
		const unsigned int central_start = out_start + _reuse_outer_path.size();

		if (props.multi_color)
		{
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
			add_central_vertex_multicolor(db, props.color_start, props.color_end, props.min, props.max);
		}
		else
		{
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
			add_central_vertex(db, props.color_start, props.min, props.max);
		}

		add_filled_rect_central(db, out_start, central_start, _reuse_outer_path.size());
	}

	void builder::add_filled_rect_aa_outline(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);

		aa_props&	  aa_p	= _aa_props[props.widget_id];
		stroke_props& out_p = _strokes[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_outline_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		generate_sharp_rect(_reuse_outer_path, props.min, props.max);
		generate_offset_rect(_reuse_outline_path, _reuse_outer_path, -static_cast<f32>(out_p.thickness));

		generate_offset_rect(_reuse_aa_outer_path, _reuse_outline_path, -static_cast<f32>(aa_p.thickness));

		const unsigned int out_start = db->vertex_count;

		if (props.multi_color)
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
		else
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
		add_filled_rect(db, out_start);

		unsigned int outline_start = 0;

		// add original vertices
		const unsigned int copy_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.stroke_col, props.min, props.max);

		outline_start = db->vertex_count;
		add_vertices(db, _reuse_outline_path, props.stroke_col, props.min, props.max);
		add_strip(db, outline_start, copy_start, _reuse_outline_path.size(), false);

		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, outline_start, 0.0f, props.min, props.max);
		add_strip(db, out_aa_start, outline_start, _reuse_aa_outer_path.size(), false);
	}

	void builder::add_filled_rect_aa_rounding(const rect_props& props)
	{
		draw_buffer*	db	 = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		aa_props&		aa_p = _aa_props[props.widget_id];
		rounding_props& rp	 = _roundings[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_outline_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		generate_rounded_rect(_reuse_outer_path, props.min, props.max, rp.rounding, rp.segments);

		generate_offset_rect(_reuse_aa_outer_path, _reuse_outer_path, -static_cast<f32>(aa_p.thickness));

		const unsigned int out_start	 = db->vertex_count;
		const unsigned int central_start = out_start + _reuse_outer_path.size();

		if (props.multi_color)
		{
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
			add_central_vertex_multicolor(db, props.color_start, props.color_end, props.min, props.max);
		}
		else
		{
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
			add_central_vertex(db, props.color_start, props.min, props.max);
		}

		add_filled_rect_central(db, out_start, central_start, _reuse_outer_path.size());

		unsigned int outline_start = 0;

		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, out_start, 0.0f, props.min, props.max);

		add_strip(db, out_aa_start, out_start, _reuse_aa_outer_path.size(), false);
	}

	void builder::add_filled_rect_rounding_outline(const rect_props& props)
	{
		draw_buffer*	db	  = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		stroke_props&	out_p = _strokes[props.widget_id];
		rounding_props& rp	  = _roundings[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_outline_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		generate_rounded_rect(_reuse_outer_path, props.min, props.max, rp.rounding, rp.segments);
		generate_offset_rect(_reuse_outline_path, _reuse_outer_path, -static_cast<f32>(out_p.thickness));

		const unsigned int out_start	 = db->vertex_count;
		const unsigned int central_start = out_start + _reuse_outer_path.size();

		if (props.multi_color)
		{
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
			add_central_vertex_multicolor(db, props.color_start, props.color_end, props.min, props.max);
		}
		else
		{
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
			add_central_vertex(db, props.color_start, props.min, props.max);
		}

		add_filled_rect_central(db, out_start, central_start, _reuse_outer_path.size());
		unsigned int outline_start = 0;

		// add original vertices
		const unsigned int copy_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.stroke_col, props.min, props.max);

		outline_start = db->vertex_count;
		add_vertices(db, _reuse_outline_path, props.stroke_col, props.min, props.max);
		add_strip(db, outline_start, copy_start, _reuse_outline_path.size(), false);
	}

	void builder::add_filled_rect_aa_outline_rounding(const rect_props& props)
	{
		draw_buffer*	db	  = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		aa_props&		aa_p  = _aa_props[props.widget_id];
		stroke_props&	out_p = _strokes[props.widget_id];
		rounding_props& rp	  = _roundings[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_outline_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);

		generate_rounded_rect(_reuse_outer_path, props.min, props.max, rp.rounding, rp.segments);

		generate_offset_rect(_reuse_outline_path, _reuse_outer_path, -static_cast<f32>(out_p.thickness));

		generate_offset_rect(_reuse_aa_outer_path, _reuse_outline_path, -static_cast<f32>(aa_p.thickness));

		const unsigned int out_start	 = db->vertex_count;
		const unsigned int central_start = out_start + _reuse_outer_path.size();

		if (props.multi_color)
		{
			add_vertices_multicolor(db, _reuse_outer_path, props.color_start, props.color_end, props.color_direction, props.min, props.max);
			add_central_vertex_multicolor(db, props.color_start, props.color_end, props.min, props.max);
		}
		else
		{
			add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
			add_central_vertex(db, props.color_start, props.min, props.max);
		}

		add_filled_rect_central(db, out_start, central_start, _reuse_outer_path.size());

		unsigned int outline_start = 0;

		// add original vertices
		const unsigned int copy_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.stroke_col, props.min, props.max);

		outline_start = db->vertex_count;
		add_vertices(db, _reuse_outline_path, props.stroke_col, props.min, props.max);
		add_strip(db, outline_start, copy_start, _reuse_outline_path.size(), false);

		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, outline_start, 0.0f, props.min, props.max);
		add_strip(db, out_aa_start, outline_start, _reuse_aa_outer_path.size(), false);
	}

	void builder::add_stroke_rect(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);

		stroke_props& out_p = _strokes[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_inner_path.resize_explicit(0);

		generate_sharp_rect(_reuse_outer_path, props.min, props.max);
		generate_offset_rect_4points(_reuse_inner_path, props.min, props.max, static_cast<f32>(out_p.thickness));

		// Original stroke
		const unsigned int out_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
		const unsigned int in_start = db->vertex_count;
		add_vertices(db, _reuse_inner_path, props.color_start, props.min, props.max);
		add_strip(db, out_start, in_start, _reuse_outer_path.size(), false);
	}

	void builder::add_stroke_rect_aa(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		_reuse_outer_path.resize_explicit(0);
		_reuse_inner_path.resize_explicit(0);
		_reuse_aa_outer_path.resize_explicit(0);
		_reuse_aa_inner_path.resize_explicit(0);

		stroke_props& out_p = _strokes[props.widget_id];
		aa_props&	  aa_p	= _aa_props[props.widget_id];

		generate_sharp_rect(_reuse_outer_path, props.min, props.max);
		generate_offset_rect(_reuse_inner_path, _reuse_outer_path, static_cast<f32>(out_p.thickness));

		generate_offset_rect(_reuse_aa_outer_path, _reuse_outer_path, -static_cast<f32>(aa_p.thickness));
		if (!_reuse_inner_path.empty())
		{
			generate_offset_rect(_reuse_aa_inner_path, _reuse_inner_path, static_cast<f32>(aa_p.thickness));
		}

		// Original stroke
		const unsigned int out_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
		const unsigned int in_start = db->vertex_count;
		add_vertices(db, _reuse_inner_path, props.color_start, props.min, props.max);
		add_strip(db, out_start, in_start, _reuse_outer_path.size(), false);
		// outer aa
		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, out_start, 0.0f, props.min, props.max);
		add_strip(db, out_aa_start, out_start, _reuse_aa_outer_path.size(), false);

		// inner aa
		const unsigned int in_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_inner_path, in_start, 0.0f, props.min, props.max);
		add_strip(db, in_start, in_aa_start, _reuse_aa_inner_path.size(), false);
	}

	void builder::add_stroke_rect_rounding(const rect_props& props)
	{
		draw_buffer* db = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		_reuse_outer_path.resize_explicit(0);
		_reuse_inner_path.resize_explicit(0);
		stroke_props&	out_p = _strokes[props.widget_id];
		rounding_props& rp	  = _roundings[props.widget_id];

		generate_rounded_rect(_reuse_outer_path, props.min, props.max, rp.rounding, rp.segments);
		generate_rounded_rect(_reuse_inner_path, props.min + VEKT_VEC2(out_p.thickness, out_p.thickness), props.max - VEKT_VEC2(out_p.thickness, out_p.thickness), rp.rounding, rp.segments);

		_reuse_aa_outer_path.resize_explicit(0);
		_reuse_aa_inner_path.resize_explicit(0);

		// Original stroke
		const unsigned int out_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
		const unsigned int in_start = db->vertex_count;
		add_vertices(db, _reuse_inner_path, props.color_start, props.min, props.max);
		add_strip(db, out_start, in_start, _reuse_outer_path.size(), false);
	}

	void builder::add_stroke_rect_aa_rounding(const rect_props& props)
	{
		draw_buffer*	db	  = get_draw_buffer(props.gfx.draw_order, props.gfx.user_data);
		stroke_props&	out_p = _strokes[props.widget_id];
		rounding_props& rp	  = _roundings[props.widget_id];
		aa_props&		aa_p  = _aa_props[props.widget_id];

		_reuse_outer_path.resize_explicit(0);
		_reuse_inner_path.resize_explicit(0);

		_reuse_aa_outer_path.resize_explicit(0);
		_reuse_aa_inner_path.resize_explicit(0);

		generate_rounded_rect(_reuse_outer_path, props.min, props.max, rp.rounding, rp.segments);
		generate_rounded_rect(_reuse_inner_path, props.min + VEKT_VEC2(out_p.thickness, out_p.thickness), props.max - VEKT_VEC2(out_p.thickness, out_p.thickness), rp.rounding, rp.segments);

		generate_offset_rect(_reuse_aa_outer_path, _reuse_outer_path, -static_cast<f32>(aa_p.thickness));
		if (!_reuse_inner_path.empty())
		{
			generate_offset_rect(_reuse_aa_inner_path, _reuse_inner_path, static_cast<f32>(aa_p.thickness));
		}

		// Original stroke
		const unsigned int out_start = db->vertex_count;
		add_vertices(db, _reuse_outer_path, props.color_start, props.min, props.max);
		const unsigned int in_start = db->vertex_count;
		add_vertices(db, _reuse_inner_path, props.color_start, props.min, props.max);
		add_strip(db, out_start, in_start, _reuse_outer_path.size(), false);

		// outer aa
		const unsigned int out_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_outer_path, out_start, 0.0f, props.min, props.max);
		add_strip(db, out_aa_start, out_start, _reuse_aa_outer_path.size(), false);

		// inner aa
		const unsigned int in_aa_start = db->vertex_count;
		add_vertices_aa(db, _reuse_aa_inner_path, in_start, 0.0f, props.min, props.max);
		add_strip(db, in_start, in_aa_start, _reuse_aa_inner_path.size(), false);
	}

	void builder::add_text(const text_props& text, const VEKT_VEC4& color, const VEKT_VEC2& position, const VEKT_VEC2& size, unsigned int draw_order, void* user_data, bool flip_uv)
	{
		if (text.font == nullptr)
		{
			V_ERR("vekt::builder::add_text() -> No font is set!");
			return;
		}

		if (text.text == nullptr)
			return;

#ifdef VEKT_STRING_CSTR
		const unsigned int char_count = static_cast<unsigned int>(strlen(text.text));
#else
		const unsigned int char_count = static_cast<unsigned int>(text.text.size());
#endif
		if (char_count == 0)
			return;

		draw_buffer* db = get_draw_buffer(draw_order, user_data, text.font);

		const f32 pixel_scale = text.font->_scale;
		const f32 subpixel	  = text.font->type == font_type::lcd ? 3.0f : 1.0f;

		const unsigned int start_vertices_idx = db->vertex_count;
		const unsigned int start_indices_idx  = db->index_count;

		unsigned int vtx_counter = 0;

		VEKT_VEC2 pen = position;

		vertex* vertices = db->add_get_vertex(char_count * 4);
		index*	indices	 = db->add_get_index(char_count * 6);

		unsigned int current_char = 0;

		const f32 scale	  = text.scale * pixel_scale;
		const f32 spacing = static_cast<f32>(text.spacing) * scale;

		auto draw_char = [&](const glyph& g, unsigned long c, unsigned long previous_char) {
			if (previous_char != 0)
			{
				pen.x += static_cast<f32>(text.font->glyph_info[previous_char].kern_advance[c]) * scale;
			}

			const f32 quad_left	  = pen.x + g.x_offset / subpixel * text.scale;
			const f32 quad_top	  = pen.y + g.y_offset * text.scale;
			const f32 quad_right  = quad_left + g.width * text.scale;
			const f32 quad_bottom = quad_top + g.height * text.scale;

			vertex& v0 = vertices[current_char * 4];
			vertex& v1 = vertices[current_char * 4 + 1];
			vertex& v2 = vertices[current_char * 4 + 2];
			vertex& v3 = vertices[current_char * 4 + 3];

			const f32		uv_x = g.uv_x, uv_y = g.uv_y, uv_w = g.uv_w, uv_h = g.uv_h;
			const VEKT_VEC2 uv0(uv_x, uv_y);
			const VEKT_VEC2 uv1(uv_x + uv_w, uv_y);
			const VEKT_VEC2 uv2(uv_x + uv_w, uv_y + uv_h);
			const VEKT_VEC2 uv3(uv_x, uv_y + uv_h);

			v0.pos = {quad_left, quad_top};
			v1.pos = {quad_right, quad_top};
			v2.pos = {quad_right, quad_bottom};
			v3.pos = {quad_left, quad_bottom};

			v0.color = color;
			v1.color = color;
			v2.color = color;
			v3.color = color;

			v0.uv = flip_uv ? VEKT_VEC2(uv0.x, uv3.y) : uv0;
			v1.uv = flip_uv ? VEKT_VEC2(uv1.x, uv2.y) : uv1;
			v2.uv = flip_uv ? VEKT_VEC2(uv2.x, uv1.y) : uv2;
			v3.uv = flip_uv ? VEKT_VEC2(uv3.x, uv0.y) : uv3;

			indices[current_char * 6]	  = start_vertices_idx + vtx_counter;
			indices[current_char * 6 + 1] = start_vertices_idx + vtx_counter + 1;
			indices[current_char * 6 + 2] = start_vertices_idx + vtx_counter + 3;

			indices[current_char * 6 + 3] = start_vertices_idx + vtx_counter + 1;
			indices[current_char * 6 + 4] = start_vertices_idx + vtx_counter + 2;
			indices[current_char * 6 + 5] = start_vertices_idx + vtx_counter + 3;

			vtx_counter += 4;

			pen.x += g.advance_x * scale + spacing;
			current_char++;
		};

#ifdef VEKT_STRING_CSTR
		const char* cstr = text.text;
#else
		const char* cstr = text.text.c_str();
#endif
		const uint8_t* c;
		pen.y += static_cast<f32>(text.font->ascent) * scale;

		unsigned long previous_char = 0;
		for (c = (uint8_t*)cstr; *c; c++)
		{
			auto		 character = *c;
			const glyph& ch		   = text.font->glyph_info[character];
			draw_char(ch, character, previous_char);
			previous_char = character;
		}
	}

	void builder::add_text_cached(const text_props& text, const VEKT_VEC4& color, const VEKT_VEC2& position, const VEKT_VEC2& size, unsigned int draw_order, void* user_data)
	{
		if (text.font == nullptr)
		{
			V_ERR("vekt::builder::add_text() -> No font is set!");
			return;
		}

		draw_buffer*   db	= get_draw_buffer(draw_order, user_data, text.font);
		const uint64_t hash = text.hash == 0 ? text_cache::hash_text_props(text, color) : text.hash;
		auto		   it	= _text_cache.find([hash](const text_cache& cache) -> bool { return cache.hash == hash; });
		if (it != _text_cache.end())
		{
			const unsigned int start_vtx = db->vertex_count;

			const unsigned int idx_count = it->vtx_count / 2 * 3;
			vertex*			   vertices	 = db->add_get_vertex(it->vtx_count);
			index*			   indices	 = db->add_get_index(idx_count);
			MEMCPY(vertices, &_text_cache_vertex_buffer[it->vtx_start], it->vtx_count * sizeof(vertex));
			MEMCPY(indices, &_text_cache_index_buffer[it->idx_start], idx_count * sizeof(index));

			for (unsigned int i = 0; i < it->vtx_count; i++)
			{
				vertices[i].pos.x += position.x;
				vertices[i].pos.y += position.y;
			}

			for (unsigned int i = 0; i < idx_count; i++)
			{
				indices[i] += start_vtx;
			}

			return;
		}

		const f32 pixel_scale = text.font->_scale;
		const f32 subpixel	  = text.font->type == font_type::lcd ? 3.0f : 1.0f;

		const unsigned int start_vertices_idx = db->vertex_count;
		const unsigned int start_indices_idx  = db->index_count;

#ifdef VEKT_STRING_CSTR
		const unsigned int char_count = static_cast<unsigned int>(strlen(text.text));
#else
		const unsigned int char_count = static_cast<unsigned int>(text.text.size());
#endif
		unsigned int vtx_counter = 0;

		VEKT_VEC2 pen = VEKT_VEC2();

		vertex* vertices = db->add_get_vertex(char_count * 4);
		index*	indices	 = db->add_get_index(char_count * 6);

		unsigned int current_char = 0;

		const f32 scale	  = text.scale * pixel_scale;
		const f32 spacing = static_cast<f32>(text.spacing) * scale;

		auto draw_char = [&](const glyph& g, unsigned long c, unsigned long previous_char) {
			if (previous_char != 0)
			{
				pen.x += static_cast<f32>(text.font->glyph_info[previous_char].kern_advance[c]) * scale;
			}

			const f32 quad_left	  = pen.x + g.x_offset / subpixel * text.scale;
			const f32 quad_top	  = pen.y + g.y_offset * text.scale;
			const f32 quad_right  = quad_left + g.width * text.scale;
			const f32 quad_bottom = quad_top + g.height * text.scale;

			vertex& v0 = vertices[current_char * 4];
			vertex& v1 = vertices[current_char * 4 + 1];
			vertex& v2 = vertices[current_char * 4 + 2];
			vertex& v3 = vertices[current_char * 4 + 3];

			const f32		uv_x = g.uv_x, uv_y = g.uv_y, uv_w = g.uv_w, uv_h = g.uv_h;
			const VEKT_VEC2 uv0(uv_x, uv_y);
			const VEKT_VEC2 uv1(uv_x + uv_w, uv_y);
			const VEKT_VEC2 uv2(uv_x + uv_w, uv_y + uv_h);
			const VEKT_VEC2 uv3(uv_x, uv_y + uv_h);

			v0.pos = {quad_left, quad_top};
			v1.pos = {quad_right, quad_top};
			v2.pos = {quad_right, quad_bottom};
			v3.pos = {quad_left, quad_bottom};

			v0.color = color;
			v1.color = color;
			v2.color = color;
			v3.color = color;

			v0.uv = uv0;
			v1.uv = uv1;
			v2.uv = uv2;
			v3.uv = uv3;

			indices[current_char * 6]	  = vtx_counter;
			indices[current_char * 6 + 1] = vtx_counter + 1;
			indices[current_char * 6 + 2] = vtx_counter + 3;

			indices[current_char * 6 + 3] = vtx_counter + 1;
			indices[current_char * 6 + 4] = vtx_counter + 2;
			indices[current_char * 6 + 5] = vtx_counter + 3;

			vtx_counter += 4;

			pen.x += g.advance_x * scale + spacing;
			current_char++;
		};

#ifdef VEKT_STRING_CSTR
		const char* cstr = text.text;
#else
		const char* cstr = text.text.c_str();
#endif
		const uint8_t* c;
		pen.y += static_cast<f32>(text.font->ascent) * scale;

		unsigned long previous_char = 0;
		for (c = (uint8_t*)cstr; *c; c++)
		{
			auto		 character = *c;
			const glyph& ch		   = text.font->glyph_info[character];
			draw_char(ch, character, previous_char);
			previous_char = character;
		}

		const unsigned int idx_count = vtx_counter / 2 * 3;

		if (_text_cache_vertex_count + vtx_counter < _text_cache_vertex_size && _text_cache_index_count + idx_count < _text_cache_index_size)
		{
			text_cache cache;
			cache.hash		= hash;
			cache.vtx_count = vtx_counter;
			cache.vtx_start = _text_cache_vertex_count;
			cache.idx_start = _text_cache_index_count;
			_text_cache.push_back(cache);
			MEMCPY(&_text_cache_vertex_buffer[_text_cache_vertex_count], vertices, vtx_counter * sizeof(vertex));
			MEMCPY(&_text_cache_index_buffer[_text_cache_index_count], indices, idx_count * sizeof(index));
			_text_cache_vertex_count += vtx_counter;
			_text_cache_index_count += idx_count;
		}

		for (unsigned int i = 0; i < vtx_counter; i++)
		{
			vertices[i].pos.x += position.x;
			vertices[i].pos.y += position.y;
		}

		for (unsigned int i = 0; i < idx_count; i++)
		{
			indices[i] += start_vertices_idx;
		}
	}

	unsigned int builder::widget_get_character_index(id widget, f32 x_diff)
	{
		text_props& tp = widget_get_text(widget);
		if (tp.font == nullptr)
			return 0;

		if (tp.text == nullptr)
			return 0;

		const font* fnt			= tp.font;
		const f32	pixel_scale = fnt->_scale;

		f32 total_x = 0.0f;

		const f32 used_scale = tp.scale;
#ifdef VEKT_STRING_CSTR
		const char* str = tp.text;
#else
		const char* str = tp.text.c_str();
#endif
		const f32	 spacing = static_cast<f32>(tp.spacing) * used_scale;
		const f32	 scale	 = pixel_scale * used_scale;
		unsigned int p		 = 0;

		for (size_t i = 0; str[i]; ++i)
		{
			const uint8_t c0 = static_cast<uint8_t>(str[i]);
			const glyph&  g0 = fnt->glyph_info[c0];

			if (total_x > x_diff)
				return static_cast<unsigned int>(i);

			total_x += g0.advance_x * scale;

			if (str[i + 1])
			{
				const uint8_t c1 = static_cast<uint8_t>(str[i + 1]);
				total_x += g0.kern_advance[c1] * scale;
			}

			total_x += spacing;
			p++;
		}

		return p;
	}

	f32 builder::widget_get_character_offset(id widget, unsigned int index)
	{
		text_props& tp = widget_get_text(widget);
		if (tp.font == nullptr)
			return 0;

		if (tp.text == nullptr)
			return 0;

		const font* fnt			= tp.font;
		const f32	pixel_scale = fnt->_scale;

		f32 total_x = 0.0f;

		const f32 used_scale = tp.scale;
#ifdef VEKT_STRING_CSTR
		const char* str = tp.text;
#else
		const char* str = tp.text.c_str();
#endif
		const f32 spacing = static_cast<f32>(tp.spacing) * used_scale;
		const f32 scale	  = pixel_scale * used_scale;
		f32		  f		  = 0.0f;

		for (size_t i = 0; str[i]; ++i)
		{
			const uint8_t c0 = static_cast<uint8_t>(str[i]);
			const glyph&  g0 = fnt->glyph_info[c0];

			if (i == index)
				return f;
			total_x += g0.advance_x * scale;

			if (str[i + 1])
			{
				const uint8_t c1 = static_cast<uint8_t>(str[i + 1]);
				total_x += g0.kern_advance[c1] * scale;
			}

			total_x += spacing;
			f = total_x;
		}

		return f;
	}

	VEKT_VEC2 builder::get_text_size(const text_props& text, const VEKT_VEC2& parent_size)
	{
		if (text.font == nullptr)
		{
			V_ERR("vekt::builder::get_text_size() -> No font is set!");
			return VEKT_VEC2();
		}

		if (text.text == nullptr)
			return VEKT_VEC2();

		const font* fnt			= text.font;
		const f32	pixel_scale = fnt->_scale;

		f32 total_x = 0.0f;
		f32 max_y	= 0.0f;

		// if (!math::equals(text._parent_relative_scale, 0.0f, 0.001f) && (!math::equals(parent_size.x, 0.0f, 0.001f) || !math::equals(parent_size.y, 0.0f, 0.001f)))
		// {
		// 	const f32 min	  = math::min(parent_size.x, parent_size.y) * text._parent_relative_scale;
		// 	const f32 ratio = min / static_cast<f32>(fnt->size);
		// 	text.scale		  = ratio;
		// }

		const f32 used_scale = text.scale;
#ifdef VEKT_STRING_CSTR
		const char* str = text.text;
#else
		const char* str = text.text.c_str();
#endif
		const f32 spacing = static_cast<f32>(text.spacing) * used_scale;
		const f32 scale	  = pixel_scale * used_scale;

		for (size_t i = 0; str[i]; ++i)
		{
			const uint8_t c0 = static_cast<uint8_t>(str[i]);
			const glyph&  g0 = fnt->glyph_info[c0];

			total_x += g0.advance_x * scale;

			if (str[i + 1])
			{
				const uint8_t c1 = static_cast<uint8_t>(str[i + 1]);
				total_x += g0.kern_advance[c1] * scale;
			}

			total_x += spacing;
		}

		max_y = (static_cast<f32>(fnt->ascent) - static_cast<f32>(fnt->descent)) * scale;
		return VEKT_VEC2(total_x - spacing, max_y);
	}

	void builder::add_strip(draw_buffer* db, unsigned int outer_start, unsigned int inner_start, unsigned int size, bool add_ccw)
	{
		index* idx = db->add_get_index(size * 6);

		for (unsigned int i = 0; i < size; i++)
		{
			const unsigned int p1_curr = outer_start + i;
			const unsigned int p1_next = outer_start + (i + 1) % size;
			const unsigned int p2_curr = inner_start + i;
			const unsigned int p2_next = inner_start + (i + 1) % size;
			const unsigned int base	   = i * 6;
			idx[base]				   = p1_curr;

			if (add_ccw)
			{
				idx[base + 1] = p2_curr;
				idx[base + 2] = p1_next;
			}
			else
			{
				idx[base + 1] = p1_next;
				idx[base + 2] = p2_curr;
			}

			idx[base + 3] = p1_next;
			if (add_ccw)
			{
				idx[base + 4] = p2_curr;
				idx[base + 5] = p2_next;
			}
			else
			{
				idx[base + 4] = p2_next;
				idx[base + 5] = p2_curr;
			}
		}
	}

	void builder::add_filled_rect(draw_buffer* db, unsigned int start)
	{
		index* idx = db->add_get_index(6);
		idx[0]	   = start;
		idx[1]	   = start + 1;
		idx[2]	   = start + 3;
		idx[3]	   = start + 1;
		idx[4]	   = start + 2;
		idx[5]	   = start + 3;
	}

	void builder::add_filled_rect_central(draw_buffer* db, unsigned int start, unsigned int central_start, unsigned int size)
	{
		for (unsigned int i = 0; i < size; i++)
		{
			db->add_index(central_start);
			db->add_index(start + i);
			db->add_index(start + ((i + 1) % (size)));
		}
	}

	void builder::add_vertices_aa(draw_buffer* db, const vector<VEKT_VEC2>& path, unsigned int original_vertices_idx, f32 alpha, const VEKT_VEC2& min, const VEKT_VEC2& max)
	{
		const unsigned int start_vtx_idx = db->vertex_count;

		for (unsigned int i = 0; i < path.size(); i++)
		{
			vertex& vtx = db->add_get_vertex();
			vtx.pos		= path[i];
			vtx.color	= db->vertex_start[original_vertices_idx + i].color;
			vtx.color.w = alpha;
			vtx.uv.x	= math::remap(vtx.pos.x, min.x, max.x, 0.0f, 1.0f);
			vtx.uv.y	= math::remap(vtx.pos.y, min.y, max.y, 0.0f, 1.0f);
		}
	}

	void builder::generate_offset_rect_4points(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& min, const VEKT_VEC2& max, f32 amount)
	{
		out_path.resize_explicit(4);
		out_path[0] = {min.x + amount, min.y + amount}; // Top-Left
		out_path[1] = {max.x - amount, min.y + amount}; // Top-Right
		out_path[2] = {max.x - amount, max.y - amount}; // Bottom-Right
		out_path[3] = {min.x + amount, max.y - amount}; // Bottom-Left
	}

	void builder::generate_offset_rect(vector<VEKT_VEC2>& out_path, const vector<VEKT_VEC2>& base_path, f32 distance)
	{
		if (base_path.size() < 2)
			return;
		out_path.resize_explicit(base_path.size());

		const size_t num_points = base_path.size();

		for (size_t i = 0; i < num_points; ++i)
		{
			// Get the current, previous, and next points
			const VEKT_VEC2& p_curr = base_path[i];
			const VEKT_VEC2& p_prev = base_path[(i + num_points - 1) % num_points];
			const VEKT_VEC2& p_next = base_path[(i + 1) % num_points];

			const VEKT_VEC2 tangent1	 = (p_curr - p_prev).normalized();
			const VEKT_VEC2 tangent2	 = (p_next - p_curr).normalized();
			const VEKT_VEC2 normal1		 = {-tangent1.y, tangent1.x};
			const VEKT_VEC2 normal2		 = {-tangent2.y, tangent2.x};
			const VEKT_VEC2 miter_vector = (normal1 + normal2).normalized();

			// Calculate the offset vertex
			const VEKT_VEC2 path = p_curr + miter_vector * distance;
			out_path[i]			 = path;
		}
	}

	void builder::add_vertices(draw_buffer* db, const vector<VEKT_VEC2>& path, const VEKT_VEC4& color, const VEKT_VEC2& min, const VEKT_VEC2& max)
	{
		vertex*	  vertices	  = db->add_get_vertex(path.size());
		const f32 inv_x_range = 1.0f / (max.x - min.x);
		const f32 inv_y_range = 1.0f / (max.y - min.y);

		for (unsigned int i = 0; i < path.size(); i++)
		{
			vertex& vtx = vertices[i];
			vtx.pos		= path[i];
			vtx.color	= color;
			vtx.uv.x	= (vtx.pos.x - min.x) * inv_x_range;
			vtx.uv.y	= (vtx.pos.y - min.y) * inv_y_range;
		}
	}

	void builder::add_vertices_multicolor(draw_buffer* db, const vector<VEKT_VEC2>& path, const VEKT_VEC4& color_start, const VEKT_VEC4& color_end, direction direction, const VEKT_VEC2& min, const VEKT_VEC2& max)
	{
		const unsigned int start_vtx_idx = db->vertex_count;

		vertex* vertices = db->add_get_vertex(path.size());

		const f32 inv_x_range = 1.0f / (max.x - min.x);
		const f32 inv_y_range = 1.0f / (max.y - min.y);

		const f32 inv_color_remap_x_range = 1.0f / (max.x - min.x);
		const f32 inv_color_remap_y_range = 1.0f / (max.y - min.y);

		const VEKT_VEC4 color_diff = color_end - color_start;

		for (unsigned int i = 0; i < path.size(); i++)
		{
			vertex& vtx = vertices[i];
			vtx.pos		= path[i];

			f32 ratio;
			if (direction == direction::horizontal)
				ratio = (vtx.pos.x - min.x) * inv_color_remap_x_range;
			else
				ratio = (vtx.pos.y - min.y) * inv_color_remap_y_range;

			vtx.color.x = color_start.x + color_diff.x * ratio;
			vtx.color.y = color_start.y + color_diff.y * ratio;
			vtx.color.z = color_start.z + color_diff.z * ratio;
			vtx.color.w = color_start.w + color_diff.w * ratio;

			// const f32 ratio = direction == direction::horizontal ? math::remap(vtx.pos.x, min.x, max.x, 0.0f, 1.0f) : math::remap(vtx.pos.y, min.y, max.y, 0.0f, 1.0f);
			// vtx.color.x		  = math::lerp(color_start.x, color_end.x, ratio);
			// vtx.color.y		  = math::lerp(color_start.y, color_end.y, ratio);
			// vtx.color.z		  = math::lerp(color_start.z, color_end.z, ratio);
			// vtx.color.w		  = math::lerp(color_start.w, color_end.w, ratio);
			vtx.uv.x = (vtx.pos.x - min.x) * inv_x_range;
			vtx.uv.y = (vtx.pos.y - min.y) * inv_y_range;
		}
	}

	void builder::add_central_vertex(draw_buffer* db, const VEKT_VEC4& color, const VEKT_VEC2& min, const VEKT_VEC2& max)
	{
		vertex& vtx = db->add_get_vertex();
		vtx.pos		= (min + max) * 0.5f;
		vtx.color	= color;
		vtx.uv		= VEKT_VEC2(0.5f, 0.5f);
	}

	void builder::add_central_vertex_multicolor(draw_buffer* db, const VEKT_VEC4& color_start, const VEKT_VEC4& color_end, const VEKT_VEC2& min, const VEKT_VEC2& max)
	{
		vertex& vtx = db->add_get_vertex();
		vtx.pos		= (min + max) * 0.5f;
		vtx.color	= (color_start + color_end) * 0.5f;
		vtx.uv		= VEKT_VEC2(0.5f, 0.5f);
	}

	void builder::generate_rounded_rect(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& min, const VEKT_VEC2& max, f32 r, int segments)
	{
		r = math::min(r, math::min((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f)); // Clamp radius

		if (segments == 0)
			segments = 10;

		segments = math::min(math::max(1, segments), 90);

		// top left
		{
			const VEKT_VEC2 center = VEKT_VEC2(min.x + r, min.y + r);
			for (int i = 0; i <= segments; ++i)
			{

				const f32		target_angle  = DEG_2_RAD * (270.0f + (90.0f / segments) * i);
				const VEKT_VEC2 point_on_unit = VEKT_VEC2(math::sin(target_angle), -math::cos(target_angle)) * r;
				const VEKT_VEC2 point		  = center + point_on_unit;
				out_path.push_back(point);
			}
		}
		// top right
		{
			const VEKT_VEC2 center = VEKT_VEC2(max.x - r, min.y + r);
			for (int i = 0; i <= segments; ++i)
			{

				const f32		target_angle  = DEG_2_RAD * ((90.0f / segments) * i);
				const VEKT_VEC2 point_on_unit = VEKT_VEC2(math::sin(target_angle), -math::cos(target_angle)) * r;
				const VEKT_VEC2 point		  = center + point_on_unit;
				out_path.push_back(point);
			}
		}

		// bottom right
		{
			const VEKT_VEC2 center = VEKT_VEC2(max.x - r, max.y - r);
			for (int i = 0; i <= segments; ++i)
			{

				const f32		target_angle  = DEG_2_RAD * (90.0f + (90.0f / segments) * i);
				const VEKT_VEC2 point_on_unit = VEKT_VEC2(math::sin(target_angle), -math::cos(target_angle)) * r;
				const VEKT_VEC2 point		  = center + point_on_unit;
				out_path.push_back(point);
			}
		}

		// bottom left
		{
			const VEKT_VEC2 center = VEKT_VEC2(min.x + r, max.y - r);
			for (int i = 0; i <= segments; ++i)
			{

				const f32		target_angle  = DEG_2_RAD * (180.0f + (90.0f / segments) * i);
				const VEKT_VEC2 point_on_unit = VEKT_VEC2(math::sin(target_angle), -math::cos(target_angle)) * r;
				const VEKT_VEC2 point		  = center + point_on_unit;
				out_path.push_back(point);
			}
		}
	}

	void builder::generate_sharp_rect(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& min, const VEKT_VEC2& max)
	{
		const unsigned int sz = out_path.size();
		out_path.resize_explicit(sz + 4);
		out_path[sz]	 = {min.x, min.y}; // Top-left
		out_path[sz + 1] = {max.x, min.y}; // Top-right
		out_path[sz + 2] = {max.x, max.y}; // Bottom-right
		out_path[sz + 3] = {min.x, max.y}; // Bottom-left
	}

	draw_buffer* builder::get_draw_buffer(unsigned int draw_order, void* user_data, font* fnt)
	{
		const VEKT_VEC4& clip = get_current_clip();

		for (draw_buffer& db : _draw_buffers)
		{
			const unsigned int fnt_id = fnt ? fnt->_font_id : NULL_WIDGET_ID;
			if (db.clip.equals(clip, 0.9f) && db.draw_order == draw_order && db.user_data == user_data && db.font_id == fnt_id)
			{
				return &db;
			}
		}

		ASSERT(_buffer_counter < _buffer_count);

		draw_buffer db	 = {};
		db.clip			 = clip;
		db.draw_order	 = draw_order;
		db.user_data	 = user_data;
		db.vertex_start	 = _vertex_buffer + _buffer_counter * _vertex_count_per_buffer;
		db.index_start	 = _index_buffer + _buffer_counter * _index_count_per_buffer;
		db.font_id		 = fnt ? fnt->_font_id : NULL_WIDGET_ID;
		db.atlas_id		 = fnt ? fnt->_atlas->get_id() : NULL_WIDGET_ID;
		db.font_type	 = fnt ? fnt->type : font_type::normal;
		db._max_vertices = _vertex_count_per_buffer;
		db._max_indices	 = _index_count_per_buffer;

		_buffer_counter++;
		_draw_buffers.push_back(db);
		return &_draw_buffers.get_back();
	}

	VEKT_VEC4 builder::calculate_intersection(const VEKT_VEC4& r1, const VEKT_VEC4& r2) const
	{
		const f32 x		 = math::max(r1.x, r2.x);
		const f32 y		 = math::max(r1.y, r2.y);
		const f32 right	 = math::min(r1.x + r1.z, r2.x + r2.z);
		const f32 bottom = math::min(r1.y + r1.w, r2.y + r2.w);

		if (right < x || bottom < y)
		{
			return VEKT_VEC4();
		}
		return {x, y, right - x, bottom - y};
	}

	void builder::widget_add_debug_wrap(id widget)
	{
		id wrapper = allocate();
		widget_set_pos(wrapper, VEKT_VEC2());
		widget_set_size(wrapper, VEKT_VEC2(1.0f, 1.0f));

		widget_gfx& r = widget_get_gfx(wrapper);
		r.flags |= gfx_is_stroke;
		_strokes[wrapper].thickness = 1;
		r.color						= VEKT_VEC4(1, 0, 0, 1);
		widget_add_child(widget, wrapper);
	}

	////////////////////////////////////////////////////////////////////////////////
	// :: ATLAS IMPL
	////////////////////////////////////////////////////////////////////////////////

	atlas::atlas(unsigned int width, unsigned int height, bool is_lcd, unsigned int id)
	{
		_id		= id;
		_width	= width;
		_height = height;
		_is_lcd = is_lcd;

		_available_slices.push_back(new atlas::slice(0, _height));
		_data_size		= width * height * (is_lcd ? 3 : 1);
		const size_t sz = static_cast<size_t>(_data_size);
		_data			= reinterpret_cast<unsigned char*>(MALLOC(sz));
		PUSH_ALLOCATION_SZ(_data_size);

		_ASSERT(_data != 0);
		if (_data != 0)
			memset(_data, 0, sz);
	}

	atlas::~atlas()
	{
		for (slice* slc : _available_slices)
			delete slc;
		_available_slices.clear();

		PUSH_DEALLOCATION_SZ(_data_size);
		FREE(_data);
	}

	bool atlas::add_font(font* font)
	{
		if (font->_atlas_required_height > _height)
			return false;

		unsigned int best_slice_diff = _height;
		slice*		 best_slice		 = nullptr;

		for (slice* slc : _available_slices)
		{
			if (slc->height < font->_atlas_required_height)
				continue;

			const unsigned int diff = slc->height - font->_atlas_required_height;
			if (diff < best_slice_diff)
			{
				best_slice_diff = diff;
				best_slice		= slc;
			}
		}

		if (best_slice == nullptr)
			return false;

		font->_atlas	 = this;
		font->_atlas_pos = best_slice->pos;
		font->_font_id	 = static_cast<unsigned int>(_fonts.size());
		_fonts.push_back(font);

		best_slice->pos += font->_atlas_required_height;
		best_slice->height -= font->_atlas_required_height;

		if (best_slice->height == 0)
		{
			_available_slices.remove(best_slice);
			delete best_slice;
		}

		return true;
	}

	void atlas::remove_font(font* fnt)
	{
		slice* slc = new slice(fnt->_atlas_pos, fnt->_atlas_pos);
		_available_slices.push_back(slc);
		_fonts.remove(fnt);
	}

	void font_manager::find_atlas(font* fnt)
	{
		for (atlas* atl : _atlases)
		{
			if (atl->get_is_lcd() != (fnt->type == font_type::lcd))
				continue;
			if (atl->add_font(fnt))
			{
				return;
			}
		}

		atlas* atl = new atlas(config.atlas_width, config.atlas_height, fnt->type == font_type::lcd, static_cast<unsigned int>(_atlases.size()));
		_atlases.push_back(atl);
		if (_atlas_created_cb)
			_atlas_created_cb(atl, _callback_user_data);
		const bool ok = atl->add_font(fnt);
		ASSERT(ok);
	}

	font* font_manager::load_font(unsigned char* data, unsigned int data_size, unsigned int size, unsigned int range0, unsigned int range1, font_type type, int sdf_padding, int sdf_edge, f32 sdf_distance)
	{

		stbtt_fontinfo stb_font;
		stbtt_InitFont(&stb_font, data, stbtt_GetFontOffsetForIndex(data, 0));

		font* fnt	= new font();
		fnt->_scale = stbtt_ScaleForMappingEmToPixels(&stb_font, static_cast<f32>(size));
		fnt->type	= type;

		stbtt_GetFontVMetrics(&stb_font, &fnt->ascent, &fnt->descent, &fnt->line_gap);
		fnt->size = size;

		int		  total_width = 0;
		int		  max_height  = 0;
		const int x_padding	  = 2;

		for (int i = range0; i < range1; i++)
		{
			glyph& glyph_info = fnt->glyph_info[i];

			if (type == font_type::sdf)
			{
				int x_off, y_off;
				glyph_info.sdf_data = stbtt_GetCodepointSDF(&stb_font, fnt->_scale, i, sdf_padding, sdf_edge, sdf_distance, &glyph_info.width, &glyph_info.height, &x_off, &y_off);
				glyph_info.x_offset = static_cast<f32>(x_off);
				glyph_info.y_offset = static_cast<f32>(y_off);
			}
			else if (type == font_type::lcd)
			{
				int ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetCodepointBitmapBoxSubpixel(&stb_font, i, fnt->_scale * 3, fnt->_scale, 1.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
				glyph_info.width	= ix1 - ix0;
				glyph_info.height	= iy1 - iy0;
				glyph_info.x_offset = static_cast<f32>(ix0);
				glyph_info.y_offset = static_cast<f32>(iy0);
			}
			else
			{
				int ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetCodepointBitmapBox(&stb_font, i, fnt->_scale, fnt->_scale, &ix0, &iy0, &ix1, &iy1);
				glyph_info.width	= ix1 - ix0;
				glyph_info.height	= iy1 - iy0;
				glyph_info.x_offset = static_cast<f32>(ix0);
				glyph_info.y_offset = static_cast<f32>(iy0);
			}

			if (glyph_info.width >= 1)
				total_width += glyph_info.width + x_padding;
			max_height = static_cast<int>(math::max(max_height, glyph_info.height));
			stbtt_GetCodepointHMetrics(&stb_font, i, &glyph_info.advance_x, &glyph_info.left_bearing);

			for (int j = 0; j < 128; j++)
				glyph_info.kern_advance[j] = stbtt_GetCodepointKernAdvance(&stb_font, i, j);
		}

		const int required_rows		= static_cast<int>(math::ceilf(static_cast<f32>(total_width) / static_cast<f32>(config.atlas_width)));
		const int required_height	= max_height;
		fnt->_atlas_required_height = required_rows * required_height;
		find_atlas(fnt);

		if (fnt->_atlas == nullptr)
		{
			delete fnt;
			V_ERR("vekt::font_manager::load_font -> Failed finding an atlas for the font!");
			return nullptr;
		}

		fnt->_font_id = static_cast<unsigned int>(_fonts.size());
		_fonts.push_back(fnt);

		int current_atlas_pen_x = 0;
		int current_atlas_pen_y = 0;

		for (int i = range0; i < range1; i++)
		{
			glyph& glyph_info = fnt->glyph_info[i];

			if (glyph_info.width <= 0 || glyph_info.height <= 0)
			{
				glyph_info.atlas_x = 0;
				glyph_info.atlas_y = 0;
				continue;
			}

			const int w = glyph_info.width;
			const int h = glyph_info.height;
			if (current_atlas_pen_x + w > static_cast<int>(config.atlas_width))
			{
				current_atlas_pen_x = 0;
				current_atlas_pen_y += max_height;
			}

			if (current_atlas_pen_y + h > static_cast<int>(fnt->_atlas_required_height))
			{
				ASSERT(false);
			}

			glyph_info.atlas_x = current_atlas_pen_x;
			glyph_info.atlas_y = fnt->_atlas_pos + current_atlas_pen_y;
			glyph_info.uv_x	   = static_cast<f32>(glyph_info.atlas_x) / static_cast<f32>(fnt->_atlas->get_width());
			glyph_info.uv_y	   = static_cast<f32>(glyph_info.atlas_y) / static_cast<f32>(fnt->_atlas->get_height());
			glyph_info.uv_w	   = static_cast<f32>(w) / static_cast<f32>(fnt->_atlas->get_width());
			glyph_info.uv_h	   = static_cast<f32>(h) / static_cast<f32>(fnt->_atlas->get_height());

			const unsigned int pixel_size	  = fnt->type == font_type::lcd ? 3 : 1;
			unsigned char*	   dest_pixel_ptr = fnt->_atlas->get_data() + (glyph_info.atlas_y * fnt->_atlas->get_width() * pixel_size) + glyph_info.atlas_x * pixel_size;

			if (type == font_type::sdf)
			{
				int atlas_stride = fnt->_atlas->get_width(); // assuming 1 byte per pixel
				for (int row = 0; row < h; ++row)
				{
					std::memcpy(dest_pixel_ptr + row * atlas_stride, glyph_info.sdf_data + row * w, w);
				}
			}
			else if (type == font_type::lcd)
			{
				stbtt_MakeCodepointBitmapSubpixel(&stb_font, dest_pixel_ptr, w, h, fnt->_atlas->get_width() * 3, fnt->_scale * 3, fnt->_scale, 1.0f, 0.0f, i);
			}
			else
			{
				stbtt_MakeCodepointBitmap(&stb_font,
										  dest_pixel_ptr,
										  w,						// Output bitmap width
										  h,						// Output bitmap height
										  fnt->_atlas->get_width(), // Atlas stride/pitch
										  fnt->_scale,				// Horizontal scale
										  fnt->_scale,				// Vertical scale
										  i);						// Codepoint
			}

			current_atlas_pen_x += w + x_padding;
		}

		if (_atlas_updated_cb)
			_atlas_updated_cb(fnt->_atlas, _callback_user_data);
		return fnt;
	}

	font* font_manager::load_font_from_file(const char* filename, unsigned int size, unsigned int range_start, unsigned int range_end, font_type type, int sdf_padding, int sdf_edge, f32 sdf_distance)
	{
		if (range_start >= range_end)
		{
			V_ERR("vekt::font_manager::load_font -> range_start needs to be smaller than range_end! %s", filename);
			return nullptr;
		}

		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			V_ERR("vekt::font_manager::load_font -> Failed opening font file! %s", filename);
			return nullptr;
		}

		std::streamsize file_size = file.tellg();
		file.seekg(0, std::ios::beg);

		vector<unsigned char> ttf_buffer;
		ttf_buffer.resize(static_cast<unsigned int>(file_size));
		if (!file.read(reinterpret_cast<char*>(ttf_buffer.data()), file_size))
		{
			V_ERR("vekt::font_manager::load_font -> Failed reading font buffer! %s", filename);
			return nullptr;
		}
		return load_font(ttf_buffer.data(), ttf_buffer.size(), size, range_start, range_end, type, sdf_padding, sdf_edge, sdf_distance);
	}

	void font_manager::unload_font(font* fnt)
	{
		fnt->_atlas->remove_font(fnt);

		if (fnt->_atlas->empty())
		{
			_atlases.remove(fnt->_atlas);

			const unsigned int sz = static_cast<unsigned int>(_atlases.size());
			for (unsigned int i = 0; i < sz; i++)
				_atlases[i]->set_id(i);

			if (_atlas_destroyed_cb)
				_atlas_destroyed_cb(fnt->_atlas, _callback_user_data);

			delete fnt->_atlas;
		}

		_fonts.remove(fnt);
		delete fnt;

		const unsigned int sz = static_cast<unsigned int>(_fonts.size());
		for (unsigned int i = 0; i < sz; i++)
			_fonts[i]->_font_id = i;
	}

	void font_manager::init()
	{
	}

	void font_manager::uninit()
	{
		for (atlas* atl : _atlases)
		{
			if (_atlas_destroyed_cb)
				_atlas_destroyed_cb(atl, _callback_user_data);
			delete atl;
		}

		for (font* fnt : _fonts)
			delete fnt;

		_atlases.clear();
		_fonts.clear();
	}

	font::~font()
	{
		for (unsigned int i = 0; i < 128; i++)
		{
			glyph& g = glyph_info[i];

			if (g.sdf_data)
				stbtt_FreeSDF(g.sdf_data, nullptr);
		}
	}

	void snapshot::init(size_t vertex_size, size_t index_size)
	{
		const size_t vertex_count = vertex_size / sizeof(vertex);
		const size_t index_count  = index_size / sizeof(index);
		vertices				  = new vekt::vertex[vertex_count];
		indices					  = new vekt::index[index_count];
		_max_vertices			  = vertex_count;
		_max_indices			  = index_count;
		draw_buffers.reserve(256);
	}

	void snapshot::uninit()
	{
		_max_vertices = 0;
		_max_indices  = 0;
		delete[] vertices;
		delete[] indices;
		vertices = nullptr;
		indices	 = nullptr;
	}

	void snapshot::copy(const vector<vekt::draw_buffer>& copy_source)
	{
		u32 vtx_offset = 0;
		u32 idx_offset = 0;
		draw_buffers.resize(0);

		for (const draw_buffer& db : copy_source)
		{
			draw_buffers.push_back(db);
			draw_buffer& last = draw_buffers.get_back();

			ASSERT(vtx_offset + db.vertex_count <= _max_vertices);
			ASSERT(idx_offset + db.index_count <= _max_indices);

			vertex* dest_vtx  = vertices + vtx_offset;
			index*	dest_idx  = indices + idx_offset;
			last.vertex_start = dest_vtx;
			last.index_start  = dest_idx;
			vtx_offset += db.vertex_count;
			idx_offset += db.index_count;

			MEMCPY(dest_vtx, db.vertex_start, sizeof(vertex) * db.vertex_count);
			MEMCPY(dest_idx, db.index_start, sizeof(index) * db.index_count);
		}
	}
}
