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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/input/input_mappings.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr f32 INPUT_TEXT_WIDTH_FACTOR = 0.6f;
		constexpr f32 INPUT_CARET_WIDTH		  = 1.0f;
		constexpr f32 INPUT_CARET_HEIGHT	  = 0.8f;
		constexpr f32 INPUT_CARET_BLINK_TIME  = 0.9f;
	}

	void editor_input_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_input_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "input_field");
		tree.attach(parent, _root);
		ui.set_pre_layout_tick(_root, on_pre_layout_tick, this);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags |= ui::wf_input | ui::wf_focusable;

		root_in.child_clip_mode = ui::clip_mode_e::cpu_rect;
		root_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		= ui::axis_mode_e::fixed;
		root_in.size_value		= {1.0f, theme.item_height};
		root_in.child_margins	= vec4f_t{0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.outline_color		 = theme.color_outline_light;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;
		paint.set_rect(_root, rect);
		paint.set_focus_color(_root, theme.color_accent0);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_press;
		listener.on_release			   = on_release;
		listener.on_double_click	   = on_double_click;
		listener.on_hover_enter		   = on_hover_enter;
		listener.on_hover_exit		   = on_hover_exit;
		listener.on_focus_gain		   = on_focus_gain;
		listener.on_drag			   = on_drag;
		listener.on_focus_lose		   = on_focus_lose;
		listener.on_key				   = on_key;
		ui.get_input().set_listener(_root, listener);

		_slider = ui.allocate_widget();
		ui.set_widget_debug_name(_slider, "input_field_slider");
		tree.attach(_root, _slider);
		tree.draw_order(_slider) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& slider_in = tree.in(_slider);
		slider_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		slider_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		slider_in.size_value	   = {1.0f, 1.0f};
		paint.set_custom(_slider, draw_slider, this);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "input_field_label");
		tree.attach(_root, _label);
		tree.draw_order(_label) = tree.draw_order_const(_root) + 2;

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		paint.set_text(_label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_overlay = ui.allocate_widget();
		ui.set_widget_debug_name(_overlay, "input_field_overlay");
		tree.attach(_root, _overlay);
		tree.draw_order(_overlay) = tree.draw_order_const(_root) + 3;

		ui::layout_in_t& overlay_in = tree.in(_overlay);
		overlay_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		overlay_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		overlay_in.size_value		= {1.0f, 1.0f};
		paint.set_custom(_overlay, draw_overlay, this);

		update_field_data(config.field);
	}

	void editor_input_field_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui		 = nullptr;
		_root	 = NULL_WIDGET;
		_slider	 = NULL_WIDGET;
		_label	 = NULL_WIDGET;
		_overlay = NULL_WIDGET;
		_fields.resize(0);
		_config					= {};
		_text[0]				= '\0';
		_text_advances[0]		= 0.0f;
		_text_len				= 0;
		_caret					= 0;
		_selection_anchor		= 0;
		_number_value			= 0.0f;
		_blink_seconds			= 0.0f;
		_text_advance_ui_scale	= 0.0f;
		_text_advance_dpi_scale = 0.0f;
		_mixed					= false;
		_edit_active			= false;
		_edit_dirty				= false;
	}

	void editor_input_field_t::select_all()
	{
		_selection_anchor = 0;
		_caret			  = _text_len;
		reset_caret_blink();
	}

	void editor_input_field_t::set_visible(bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		ui::layout_in_t&   in	= tree.in(_root);
		if (visible)
			in.flags |= ui::wf_visible | ui::wf_input | ui::wf_focusable;
		else
			in.flags = 0;
	}

	void editor_input_field_t::update_field_data(editor_input_field_field_t field)
	{
		SFG_ASSERT(field.fields.size > 0);
		SFG_ASSERT(field.fields.data != nullptr);
		for (size_t i = 0; i < field.fields.size; ++i)
			SFG_ASSERT(field.fields.data[i] != nullptr);

		if (field.fields.data != _fields.data())
			_fields.assign(field.fields.data, field.fields.data + field.fields.size);
		field.fields  = {.data = _fields.data(), .size = _fields.size()};
		_config.field = field;
		_mixed		  = false;
		_edit_active  = false;
		_edit_dirty	  = false;

		ui::layout_in_t& root_in = _ui->get_tree().in(_root);
		root_in.child_margins =
			field.is_slider ? vec4f_t{editor_theme_t::get().outline_thickness * 2, 0.0f, editor_theme_t::get().outline_thickness * 2, 0.0f} : vec4f_t{0.0f, editor_theme_t::get().margin_horizontal, 0.0f, editor_theme_t::get().margin_horizontal};

		switch (field.type)
		{
		case editor_input_field_field_type_e::string: {
			const string_t& value = *reinterpret_cast<const string_t*>(field.fields.data[0]);
			for (size_t i = 1; i < field.fields.size; ++i)
			{
				if (*reinterpret_cast<const string_t*>(field.fields.data[i]) != value)
				{
					_mixed = true;
					break;
				}
			}
			set_text_raw(_mixed ? "" : value.c_str());
			break;
		}
		case editor_input_field_field_type_e::char_array: {
			SFG_ASSERT(field.field_size > 0);
			const char* value = reinterpret_cast<const char*>(field.fields.data[0]);
			for (size_t i = 1; i < field.fields.size; ++i)
			{
				if (std::strncmp(reinterpret_cast<const char*>(field.fields.data[i]), value, field.field_size) != 0)
				{
					_mixed = true;
					break;
				}
			}
			set_text_raw(_mixed ? "" : value);
			break;
		}
		case editor_input_field_field_type_e::pod_number: {
			SFG_ASSERT(field.field_size > 0);
			const u8* value = field.fields.data[0];
			for (size_t i = 1; i < field.fields.size; ++i)
			{
				if (std::memcmp(field.fields.data[i], value, field.field_size) != 0)
				{
					_mixed = true;
					break;
				}
			}
			_number_value = read_pod_number(value);
			if (field.is_slider)
				_number_value = math::clamp(_number_value, _config.min_value, _config.max_value);
			if (_mixed)
				set_text_raw("");
			else
				format_number();
			break;
		}
		}

		refresh_text();
	}

	void editor_input_field_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_input_field_t::set_text(const char* text)
	{
		_mixed = false;
		set_text_raw(text);
		if (update_number_from_text())
			modify_field();
		refresh_text();
		submit_edit();
	}

	void editor_input_field_t::refresh_text()
	{
		rebuild_text_advances();

		const editor_theme_t& theme		  = editor_theme_t::get();
		const bool			  placeholder = _mixed || (_text_len == 0 && _config.placeholder != nullptr);
		const char*			  text		  = _mixed ? "Mixed" : (placeholder ? _config.placeholder : _text);
		_ui->set_widget_text(_label, text != nullptr ? text : "");

		ui::paint_def_t& label_paint = _ui->get_paint().def(_label);
		label_paint.text.color		 = _mixed ? theme.color_accent_warn : (placeholder ? (std::strcmp(_config.placeholder, "Mixed") == 0 ? theme.color_accent_warn : theme.color_text2) : theme.color_text0);

		ui::layout_in_t& label_in = _ui->get_tree().in(_label);
		if (_config.field.is_slider)
		{
			label_in.pos_mode_x	 = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.x = 0.5f;
			label_in.anchor_x	 = ui::anchor_e::center;
		}
		else
		{
			label_in.pos_mode_x	 = ui::pos_mode_e::offset_in_parent;
			label_in.pos_value.x = 0.0f;
			label_in.anchor_x	 = ui::anchor_e::start;
		}
	}

	void editor_input_field_t::commit_number_text()
	{
		if (_config.field.type != editor_input_field_field_type_e::pod_number)
			return;

		if (!_edit_dirty)
		{
			format_number();
			refresh_text();
			return;
		}

		char* end	= nullptr;
		f32	  value = static_cast<f32>(std::strtod(_text, &end));
		if (end == _text)
			value = 0.0f;
		if (_config.field.is_slider)
			value = math::clamp(value, _config.min_value, _config.max_value);
		if (_config.increment >= 1.0f)
			value = static_cast<f32>(static_cast<i64>(value + (value >= 0.0f ? 0.5f : -0.5f)));
		if (value == _number_value)
		{
			format_number();
			refresh_text();
			return;
		}
		_number_value = value;
		format_number();
		modify_field();
		refresh_text();
	}

	bool editor_input_field_t::update_number_from_text()
	{
		if (_config.field.type != editor_input_field_field_type_e::pod_number)
			return true;
		char* end	= nullptr;
		f32	  value = static_cast<f32>(std::strtod(_text, &end));
		if (end == _text)
			value = 0.0f;
		if (_config.field.is_slider)
			value = math::clamp(value, _config.min_value, _config.max_value);
		if (_config.increment >= 1.0f)
			value = static_cast<f32>(static_cast<i64>(value + (value >= 0.0f ? 0.5f : -0.5f)));
		if (value == _number_value)
			return false;
		_number_value = value;
		return true;
	}

	void editor_input_field_t::begin_edit()
	{
		if (_edit_active)
			return;
		_edit_active = true;
		if (_config.callbacks.edit_begin != nullptr)
			_config.callbacks.edit_begin(_config.callbacks.user_data);
	}

	void editor_input_field_t::submit_edit()
	{
		if (!_edit_dirty)
		{
			_edit_active = false;
			return;
		}
		if (_config.callbacks.edit_submitted != nullptr)
			_config.callbacks.edit_submitted(_config.callbacks.user_data);
		_edit_active = false;
		_edit_dirty	 = false;
	}

	void editor_input_field_t::modify_field()
	{
		SFG_ASSERT(_config.field.fields.size > 0);
		SFG_ASSERT(_config.field.fields.data != nullptr);

		if (!has_field_value_changed())
			return;

		begin_edit();

		switch (_config.field.type)
		{
		case editor_input_field_field_type_e::string:
			for (size_t i = 0; i < _config.field.fields.size; ++i)
				*reinterpret_cast<string_t*>(_config.field.fields.data[i]) = _text;
			break;
		case editor_input_field_field_type_e::char_array: {
			SFG_ASSERT(_config.field.field_size > 0);
			const size_t copy_size = math::min(static_cast<size_t>(_text_len), _config.field.field_size - 1);
			for (size_t i = 0; i < _config.field.fields.size; ++i)
			{
				SFG_MEMCPY(_config.field.fields.data[i], _text, copy_size);
				reinterpret_cast<char*>(_config.field.fields.data[i])[copy_size] = '\0';
			}
			break;
		}
		case editor_input_field_field_type_e::pod_number:
			for (size_t i = 0; i < _config.field.fields.size; ++i)
				write_pod_number(_config.field.fields.data[i], _number_value);
			break;
		}

		_edit_dirty = true;
		if (_config.callbacks.edited != nullptr)
			_config.callbacks.edited(_config.callbacks.user_data);
	}

	bool editor_input_field_t::has_field_value_changed() const
	{
		switch (_config.field.type)
		{
		case editor_input_field_field_type_e::string:
			for (size_t i = 0; i < _config.field.fields.size; ++i)
			{
				if (std::strcmp(reinterpret_cast<const string_t*>(_config.field.fields.data[i])->c_str(), _text) != 0)
					return true;
			}
			return false;
		case editor_input_field_field_type_e::char_array:
			SFG_ASSERT(_config.field.field_size > 0);
			for (size_t i = 0; i < _config.field.fields.size; ++i)
			{
				if (std::strcmp(reinterpret_cast<const char*>(_config.field.fields.data[i]), _text) != 0)
					return true;
			}
			return false;
		case editor_input_field_field_type_e::pod_number:
			for (size_t i = 0; i < _config.field.fields.size; ++i)
			{
				if (read_pod_number(_config.field.fields.data[i]) != _number_value)
					return true;
			}
			return false;
		}
		return false;
	}

	void editor_input_field_t::set_text_raw(const char* value)
	{
		const char* src = value != nullptr ? value : "";
		_text_len		= static_cast<u32>(math::min(static_cast<size_t>(TEXT_CAPACITY - 1), std::strlen(src)));
		SFG_MEMCPY(_text, src, _text_len);
		_text[_text_len]  = '\0';
		_caret			  = _text_len;
		_selection_anchor = _caret;
	}

	void editor_input_field_t::format_number()
	{
		if (_config.increment >= 1.0f)
			std::snprintf(_text, TEXT_CAPACITY, "%lld", static_cast<long long>(_number_value));
		else
			std::snprintf(_text, TEXT_CAPACITY, "%.3f", static_cast<double>(_number_value));
		_text_len		  = static_cast<u32>(std::strlen(_text));
		_caret			  = _text_len;
		_selection_anchor = _caret;
	}

	void editor_input_field_t::insert_char(char c)
	{
		_mixed = false;
		if (!insert_char_data(c))
			return;
		if (update_number_from_text())
			modify_field();
		refresh_text();
	}

	void editor_input_field_t::insert_text(const char* text)
	{
		SFG_ASSERT(text != nullptr);
		_mixed	 = false;
		bool any = false;
		for (const char* c = text; *c != '\0'; ++c)
			any |= insert_char_data(*c);
		if (!any)
			return;
		if (update_number_from_text())
			modify_field();
		refresh_text();
	}

	bool editor_input_field_t::insert_char_data(char c)
	{
		if (!accepts_char(c) || _text_len + 1 >= TEXT_CAPACITY)
			return false;
		erase_selection();
		SFG_MEMMOVE(_text + _caret + 1, _text + _caret, _text_len - _caret + 1);
		_text[_caret] = c;
		_text_len++;
		set_caret(_caret + 1);
		return true;
	}

	void editor_input_field_t::erase_selection()
	{
		if (_caret == _selection_anchor)
			return;
		erase_range(math::min(_caret, _selection_anchor), math::max(_caret, _selection_anchor));
	}

	void editor_input_field_t::erase_range(u32 start, u32 end)
	{
		if (start >= end || start > _text_len || end > _text_len)
			return;
		SFG_MEMMOVE(_text + start, _text + end, _text_len - end + 1);
		_text_len -= end - start;
		set_caret(start);
	}

	void editor_input_field_t::set_caret(u32 index)
	{
		_caret			  = math::min(index, _text_len);
		_selection_anchor = _caret;
		reset_caret_blink();
	}

	void editor_input_field_t::update_drag_selection(const vec2f_t& pos)
	{
		_caret = index_from_pos(pos);
		reset_caret_blink();
	}

	void editor_input_field_t::apply_number_delta(f32 delta_x)
	{
		if (_config.field.type != editor_input_field_field_type_e::pod_number)
			return;
		_mixed	  = false;
		f32 value = _number_value + delta_x * _config.increment;
		if (_config.field.is_slider)
			value = math::clamp(value, _config.min_value, _config.max_value);
		if (_config.increment >= 1.0f)
			value = static_cast<f32>(static_cast<i64>(value + (value >= 0.0f ? 0.5f : -0.5f)));
		if (value == _number_value)
			return;
		_number_value = value;
		format_number();
		modify_field();
		refresh_text();
	}

	void editor_input_field_t::apply_slider_position(const vec2f_t& pos)
	{
		if (_config.field.type != editor_input_field_field_type_e::pod_number || !_config.field.is_slider)
			return;
		const ui::layout_out_t& out	  = _ui->get_tree().out(_root);
		const f32				t	  = out.size.x > 0.0f ? math::clamp((pos.x - out.pos.x) / out.size.x, 0.0f, 1.0f) : 0.0f;
		const f32				value = _config.min_value + (_config.max_value - _config.min_value) * t;
		if (value == _number_value)
			return;
		_mixed		  = false;
		_number_value = value;
		format_number();
		modify_field();
		refresh_text();
	}

	void editor_input_field_t::rebuild_text_advances()
	{
		_text_advances[0]		= 0.0f;
		const f32 ui_scale		= ui::get_valid_scale(_ui->get_ui_scale());
		const f32 dpi_scale		= ui::get_valid_scale(_ui->get_dpi_scale());
		_text_advance_ui_scale	= ui_scale;
		_text_advance_dpi_scale = dpi_scale;
		if (_text_len == 0)
			return;

		const editor_theme_t&	   theme	  = editor_theme_t::get();
		const font_runtime_t*	   font		  = resource_manager_t::get().find_runtime<font_runtime_t>(theme.font_default);
		const ui::vg_text_style_t& text_style = _ui->get_paint().def(_label).text;

		if (font == nullptr || font->face == nullptr)
		{
			for (u32 i = 0; i < _text_len; ++i)
				_text_advances[i + 1] = static_cast<f32>(i + 1) * theme.text_default_px_size * ui_scale * INPUT_TEXT_WIDTH_FACTOR;
			return;
		}

		ui::vg_text_paint_t text_paint = {};
		text_paint.font				   = font;
		text_paint.color			   = text_style.color;
		text_paint.size_px			   = text_style.point_size * ui_scale;
		text_paint.raster_px		   = ui::get_text_raster_px(text_paint.size_px, dpi_scale);
		text_paint.spacing			   = static_cast<f32>(text_style.spacing) * ui_scale;
		text_paint.raster_mode		   = text_style.raster_mode;
		text_paint.flip_uv			   = text_style.flip_uv;

		ui::glyph_atlas_t& atlas	  = resource_manager_t::get().get_glyph_atlas();
		const u32		   px		  = ui::get_text_paint_raster_px(text_paint);
		const f32		   draw_scale = ui::get_text_paint_draw_scale(text_paint, px);
		u32				   prev		  = 0;
		f32				   pen		  = 0.0f;
		f32				   min_x	  = 0.0f;
		f32				   max_x	  = 0.0f;
		bool			   valid	  = false;
		for (u32 i = 0; i < _text_len; ++i)
		{
			const u32 c = static_cast<u8>(_text[i]);
			if (prev != 0)
				pen += atlas.get_kern_advance(font, prev, c, px) * draw_scale;

			const ui::glyph_entry_t* g = atlas.request_glyph(font, c, px, text_style.raster_mode);
			if (g->width > 0 && g->height > 0)
			{
				const f32 quad_left	 = pen + g->left_bearing * draw_scale;
				const f32 quad_right = quad_left + static_cast<f32>(g->width) * draw_scale;
				min_x				 = valid ? math::min(min_x, quad_left) : quad_left;
				max_x				 = valid ? math::max(max_x, quad_right) : quad_right;
				_text_advances[i]	 = quad_left;
				valid				 = true;
			}
			else
			{
				_text_advances[i] = pen;
			}
			pen += g->advance_x * draw_scale + text_paint.spacing;
			prev = c;
		}
		const f32 advance_x = pen - text_paint.spacing;
		if (valid)
		{
			for (u32 i = 0; i < _text_len; ++i)
				_text_advances[i] -= min_x;
			_text_advances[_text_len] = math::max(max_x, advance_x) - min_x;
		}
		else
		{
			_text_advances[_text_len] = math::max(0.0f, advance_x);
		}
	}

	void editor_input_field_t::reset_caret_blink()
	{
		_blink_seconds = 0.0f;
	}

	u32 editor_input_field_t::index_from_pos(const vec2f_t& pos) const
	{
		if (_text_len == 0)
			return 0;

		const ui::layout_out_t& label_out = _ui->get_tree().out(_label);
		const f32				x		  = pos.x - math::round(label_out.pos.x);
		for (u32 i = 0; i < _text_len; ++i)
		{
			const f32 midpoint = (_text_advances[i] + _text_advances[i + 1]) * 0.5f;
			if (x < midpoint)
				return i;
		}
		return _text_len;
	}

	f32 editor_input_field_t::text_width(u32 len) const
	{
		return _text_advances[math::min(len, _text_len)];
	}

	bool editor_input_field_t::accepts_char(char c) const
	{
		if (_config.field.type != editor_input_field_field_type_e::pod_number)
			return c >= 32;
		if (c >= '0' && c <= '9')
			return true;
		const bool selected = _caret != _selection_anchor;
		const u32  sel_min	= selected ? math::min(_caret, _selection_anchor) : _caret;
		const u32  sel_max	= selected ? math::max(_caret, _selection_anchor) : _caret;
		if (c == '-' || c == '+')
		{
			if (sel_min != 0)
				return false;
			for (u32 i = 0; i < _text_len; ++i)
			{
				if (i >= sel_min && i < sel_max)
					continue;
				if (_text[i] == '-' || _text[i] == '+')
					return false;
			}
			return true;
		}
		if (_config.increment < 1.0f && (c == '.' || c == ','))
		{
			for (u32 i = 0; i < _text_len; ++i)
			{
				if (i >= sel_min && i < sel_max)
					continue;
				if (_text[i] == '.' || _text[i] == ',')
					return false;
			}
			return true;
		}
		return false;
	}

	f32 editor_input_field_t::read_pod_number(const u8* data) const
	{
		if (_config.increment >= 1.0f)
		{
			if (_config.field.field_size == sizeof(u64))
				return _config.min_value >= 0.0f ? static_cast<f32>(*reinterpret_cast<const u64*>(data)) : static_cast<f32>(*reinterpret_cast<const i64*>(data));
			if (_config.field.field_size == sizeof(u32))
				return _config.min_value >= 0.0f ? static_cast<f32>(*reinterpret_cast<const u32*>(data)) : static_cast<f32>(*reinterpret_cast<const i32*>(data));
			if (_config.field.field_size == sizeof(u16))
				return _config.min_value >= 0.0f ? static_cast<f32>(*reinterpret_cast<const u16*>(data)) : static_cast<f32>(*reinterpret_cast<const i16*>(data));
			if (_config.field.field_size == sizeof(u8))
				return _config.min_value >= 0.0f ? static_cast<f32>(*reinterpret_cast<const u8*>(data)) : static_cast<f32>(*reinterpret_cast<const i8*>(data));
		}
		if (_config.field.field_size == sizeof(f32))
			return *reinterpret_cast<const f32*>(data);
		if (_config.field.field_size == sizeof(u64))
			return static_cast<f32>(*reinterpret_cast<const u64*>(data));
		return 0.0f;
	}

	void editor_input_field_t::write_pod_number(u8* data, f32 value)
	{
		if (_config.increment >= 1.0f)
		{
			if (_config.field.field_size == sizeof(u64))
			{
				if (_config.min_value >= 0.0f)
					*reinterpret_cast<u64*>(data) = static_cast<u64>(math::max(value, 0.0f));
				else
					*reinterpret_cast<i64*>(data) = static_cast<i64>(value);
				return;
			}
			if (_config.field.field_size == sizeof(u32))
			{
				if (_config.min_value >= 0.0f)
					*reinterpret_cast<u32*>(data) = static_cast<u32>(math::max(value, 0.0f));
				else
					*reinterpret_cast<i32*>(data) = static_cast<i32>(value);
				return;
			}
			if (_config.field.field_size == sizeof(u16))
			{
				if (_config.min_value >= 0.0f)
					*reinterpret_cast<u16*>(data) = static_cast<u16>(math::clamp(value, 0.0f, 65535.0f));
				else
					*reinterpret_cast<i16*>(data) = static_cast<i16>(math::clamp(value, -32768.0f, 32767.0f));
				return;
			}
			if (_config.field.field_size == sizeof(u8))
			{
				if (_config.min_value >= 0.0f)
					*reinterpret_cast<u8*>(data) = static_cast<u8>(math::clamp(value, 0.0f, 255.0f));
				else
					*reinterpret_cast<i8*>(data) = static_cast<i8>(math::clamp(value, -128.0f, 127.0f));
				return;
			}
		}
		if (_config.field.field_size == sizeof(f32))
			*reinterpret_cast<f32*>(data) = value;
	}

	void editor_input_field_t::on_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		if (btn == ui::mouse_button_e::left)
		{
			field._caret			= field.index_from_pos(pos);
			field._selection_anchor = field._caret;
			field.reset_caret_blink();
		}
		else if (btn == ui::mouse_button_e::middle)
			field.reset_caret_blink();
	}

	void editor_input_field_t::on_release(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::middle)
			return;

		static_cast<editor_input_field_t*>(user_data)->submit_edit();
	}

	void editor_input_field_t::on_double_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn == ui::mouse_button_e::left)
			static_cast<editor_input_field_t*>(user_data)->select_all();
	}

	void editor_input_field_t::on_hover_enter(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void*)
	{
		process::set_cursor_state(window_cursor_state_e::caret);
	}

	void editor_input_field_t::on_hover_exit(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void*)
	{
		process::set_cursor_state(window_cursor_state_e::arrow);
	}

	void editor_input_field_t::on_focus_gain(ui::input_router_t&, ui::widget_id_t, bool via_navigation, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		field.reset_caret_blink();
		field._edit_active = false;
		field._edit_dirty  = false;

		if (via_navigation)
			field.select_all();
	}

	void editor_input_field_t::on_drag(ui::input_router_t& router, ui::widget_id_t, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		if (router.is_pressed(ui::mouse_button_e::middle) == field._root)
		{
			if (field._config.field.is_slider)
				field.apply_slider_position(pos);
			else
				field.apply_number_delta(pos.x - (pos.x - delta.x));
		}
		else if (router.is_pressed(ui::mouse_button_e::left) == field._root)
			field.update_drag_selection(pos);
	}

	void editor_input_field_t::on_focus_lose(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		field.commit_number_text();
		field.submit_edit();
	}

	void editor_input_field_t::on_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press && ev.action != ui::key_action_e::repeat)
			return;

		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		const bool			  ctrl	= process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

		if (ev.key == static_cast<u16>(input_code::key_left))
		{
			const u32 selection_min = math::min(field._caret, field._selection_anchor);
			field.set_caret(selection_min > 0 ? selection_min - 1 : 0);
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_right))
		{
			const u32 selection_max = math::max(field._caret, field._selection_anchor);
			field.set_caret(selection_max < field._text_len ? selection_max + 1 : field._text_len);
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_a) && ctrl)
		{
			field.select_all();
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_c) && ctrl)
		{
			if (field._caret != field._selection_anchor)
			{
				char	  clip[TEXT_CAPACITY] = {};
				const u32 start				  = math::min(field._caret, field._selection_anchor);
				const u32 count				  = math::max(field._caret, field._selection_anchor) - start;
				SFG_MEMCPY(clip, field._text + start, count);
				clip[count] = '\0';
				process::push_clipboard(clip);
			}
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_x) && ctrl)
		{
			if (field._caret != field._selection_anchor)
			{
				char	  clip[TEXT_CAPACITY] = {};
				const u32 start				  = math::min(field._caret, field._selection_anchor);
				const u32 count				  = math::max(field._caret, field._selection_anchor) - start;
				SFG_MEMCPY(clip, field._text + start, count);
				clip[count] = '\0';
				process::push_clipboard(clip);
				field.erase_selection();
				if (field.update_number_from_text())
					field.modify_field();
				field.refresh_text();
			}
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_v) && ctrl)
		{
			const string_t clip = process::get_clipboard();
			field.insert_text(clip.c_str());
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_backspace))
		{
			field._mixed = false;
			if (field._caret != field._selection_anchor)
				field.erase_selection();
			else if (field._caret > 0)
				field.erase_range(field._caret - 1, field._caret);
			if (field.update_number_from_text())
				field.modify_field();
			field.refresh_text();
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_delete))
		{
			field._mixed = false;
			if (field._caret != field._selection_anchor)
				field.erase_selection();
			else if (field._caret < field._text_len)
				field.erase_range(field._caret, field._caret + 1);
			if (field.update_number_from_text())
				field.modify_field();
			field.refresh_text();
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_return))
		{
			field.commit_number_text();
			field.submit_edit();
			return;
		}

		const char c	= process::get_character_from_key(ev.key);
		const u16  mask = process::get_character_mask_from_key(ev.key, c);
		if ((mask & character_mask::printable) != 0)
			field.insert_char(c == ',' ? '.' : c);
	}

	void editor_input_field_t::on_pre_layout_tick(ui::ui_context&, ui::widget_id_t, f32 dt_seconds, void* user_data)
	{
		editor_input_field_t& field		= *static_cast<editor_input_field_t*>(user_data);
		const f32			  ui_scale	= ui::get_valid_scale(field._ui->get_ui_scale());
		const f32			  dpi_scale = ui::get_valid_scale(field._ui->get_dpi_scale());
		if (field._text_advance_ui_scale != ui_scale || field._text_advance_dpi_scale != dpi_scale)
			field.rebuild_text_advances();

		if (field._ui->get_input().get_focused() == field._root)
		{
			field._blink_seconds += dt_seconds;
			if (field._blink_seconds >= INPUT_CARET_BLINK_TIME)
				field._blink_seconds -= INPUT_CARET_BLINK_TIME;
		}
		else
		{
			field._blink_seconds = 0.0f;
		}
	}

	void editor_input_field_t::draw_slider(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		if (!field._config.field.is_slider)
			return;

		const f32 range = field._config.max_value - field._config.min_value;
		if (range <= 0.0f)
			return;

		const editor_theme_t&	theme = editor_theme_t::get();
		const ui::layout_out_t& out	  = field._ui->get_tree().out(id);
		const f32				t	  = math::clamp((field._number_value - field._config.min_value) / range, 0.0f, 1.0f);
		ui::vg_rect_paint_t		rect  = {};
		rect.fill_color_a			  = theme.color_accent0;
		rect.fill_color_b			  = theme.color_accent0_dim;
		rect.gradient				  = ui::vg_gradient_e::horizontal;
		rect.rounding				  = theme.item_rounding;
		rect.rounding_segs			  = 4;
		rect.aa_thickness			  = theme.aa_thickness;
		ui::ui_render_state_t state	  = {};
		state.pipeline				  = paint.get_pipelines().default_pipeline;
		canvas.add_rect({out.pos.x, out.pos.y}, {out.pos.x + out.size.x * t, out.pos.y + out.size.y}, rect, state, field._ui->get_tree().draw_order_const(id));
	}

	void editor_input_field_t::draw_overlay(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		if (field._ui->get_input().get_focused() != field._root)
			return;

		const editor_theme_t&	theme	   = editor_theme_t::get();
		const ui::layout_out_t& root_out   = field._ui->get_tree().out(field._root);
		const ui::layout_out_t& label_out  = field._ui->get_tree().out(field._label);
		const f32				y0		   = root_out.pos.y + root_out.size.y * (1.0f - INPUT_CARET_HEIGHT) * 0.5f;
		const f32				y1		   = root_out.pos.y + root_out.size.y * (1.0f + INPUT_CARET_HEIGHT) * 0.5f;
		const u32				draw_order = field._ui->get_tree().draw_order_const(id);
		ui::ui_render_state_t	state	   = {};
		state.pipeline					   = paint.get_pipelines().default_pipeline;

		if (field._caret != field._selection_anchor)
		{
			const f32			text_x = math::round(label_out.pos.x);
			const f32			x0	   = text_x + field.text_width(math::min(field._caret, field._selection_anchor));
			const f32			x1	   = text_x + field.text_width(math::max(field._caret, field._selection_anchor));
			ui::vg_rect_paint_t rect   = {};
			rect.fill_color_a		   = theme.color_accent1_dim;
			rect.fill_color_b		   = theme.color_accent1_dim;
			canvas.add_rect({x0, y0}, {x1, y1}, rect, state, draw_order);
		}

		if (field._blink_seconds >= INPUT_CARET_BLINK_TIME * 0.5f)
			return;

		const f32			x	 = math::round(label_out.pos.x) + field.text_width(field._caret);
		ui::vg_line_paint_t line = {};
		line.color				 = theme.color_text0;
		line.thickness			 = math::max(1.0f, INPUT_CARET_WIDTH * ui::get_valid_scale(field._ui->get_ui_scale()));
		canvas.add_line({x, y0}, {x, y1}, line, state, draw_order + 1);
	}
}
