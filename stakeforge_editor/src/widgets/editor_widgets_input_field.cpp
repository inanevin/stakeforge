// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_input_field.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sfg
{
	namespace
	{
		constexpr f32 INPUT_TEXT_WIDTH_FACTOR = 0.6f;
		constexpr f32 INPUT_CARET_WIDTH		  = 1.0f;
		constexpr f32 INPUT_CARET_HEIGHT	  = 0.8f;
		constexpr f32 INPUT_CARET_BLINK_TIME  = 0.9f;

		bool is_ctrl_down()
		{
			return process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
		}

		bool is_press_or_repeat(ui::key_action_e action)
		{
			return action == ui::key_action_e::press || action == ui::key_action_e::repeat;
		}

		u32 raster_px_for(f32 point_size, f32 ui_scale, f32 dpi_scale)
		{
			const f32 scale = ui_scale > 0.0f ? ui_scale : 1.0f;
			return static_cast<u32>(math::max(1.0f, point_size * scale * dpi_scale + 0.5f));
		}
	}

	void editor_input_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_input_field_config_t& config)
	{
		_ui			  = &ui;
		_config		  = config;
		_number_value = config.number_value;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "input_field");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;
		ui.set_pre_layout_tick(_root, on_pre_layout_tick, this);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_focusable | ui::wf_clip_children;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.child_margins	 = config.type == editor_input_field_type_e::number_slider ? vec4f_t{} : vec4f_t{0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

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
		slider_in.flags			   = ui::wf_overlay;
		slider_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		slider_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		slider_in.size_value	   = {1.0f, 1.0f};
		paint.set_custom(_slider, draw_slider, this);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "input_field_label");
		tree.attach(_root, _label);
		tree.draw_order(_label) = tree.draw_order_const(_root) + 2;

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = ui::wf_overlay;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		paint.set_text(_label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_overlay = ui.allocate_widget();
		ui.set_widget_debug_name(_overlay, "input_field_overlay");
		tree.attach(_root, _overlay);
		tree.draw_order(_overlay) = tree.draw_order_const(_root) + 3;

		ui::layout_in_t& overlay_in = tree.in(_overlay);
		overlay_in.flags			= ui::wf_overlay;
		overlay_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		overlay_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		overlay_in.size_value		= {1.0f, 1.0f};
		paint.set_custom(_overlay, draw_overlay, this);

		if (is_number_type())
		{
			if (config.type == editor_input_field_type_e::number_slider)
				_number_value = math::clamp(_number_value, config.min_value, config.max_value);
			if (config.integer)
				_number_value = static_cast<f32>(static_cast<i32>(_number_value + (_number_value >= 0.0f ? 0.5f : -0.5f)));
			format_number();
		}
		else
		{
			set_text_raw(config.text_value != nullptr ? config.text_value : "");
		}
		refresh_text();
	}

	void editor_input_field_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui				  = nullptr;
		_root			  = NULL_WIDGET;
		_slider			  = NULL_WIDGET;
		_label			  = NULL_WIDGET;
		_overlay		  = NULL_WIDGET;
		_config			  = {};
		_text[0]		  = '\0';
		_text_advances[0] = 0.0f;
		_text_len		  = 0;
		_caret			  = 0;
		_selection_anchor = 0;
		_number_value	  = 0.0f;
		_blink_seconds	  = 0.0f;
	}

	void editor_input_field_t::set_text(const char* value)
	{
		set_text_raw(value);
		refresh_text();
	}

	void editor_input_field_t::set_number(f32 value)
	{
		_number_value = value;
		if (_config.type == editor_input_field_type_e::number_slider)
			_number_value = math::clamp(_number_value, _config.min_value, _config.max_value);
		if (_config.integer)
			_number_value = static_cast<f32>(static_cast<i32>(_number_value + (_number_value >= 0.0f ? 0.5f : -0.5f)));
		format_number();
		refresh_text();
	}

	void editor_input_field_t::refresh_text()
	{
		rebuild_text_advances();

		const editor_theme_t& theme		  = editor_theme_t::get();
		const bool			  placeholder = _text_len == 0 && _config.placeholder != nullptr;
		const char*			  text		  = placeholder ? _config.placeholder : _text;
		_ui->set_widget_text(_label, text != nullptr ? text : "");

		ui::paint_def_t& label_paint = _ui->get_paint().def(_label);
		label_paint.text.color		 = placeholder ? theme.color_text2 : theme.color_text0;

		ui::layout_in_t& label_in = _ui->get_tree().in(_label);
		if (_config.type == editor_input_field_type_e::number_slider)
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

	void editor_input_field_t::notify_changed()
	{
		if (is_number_type())
		{
			if (_config.on_number_changed != nullptr)
				_config.on_number_changed(_number_value, _config.user_data);
		}
		else if (_config.on_text_changed != nullptr)
		{
			_config.on_text_changed(_text, _config.user_data);
		}
	}

	void editor_input_field_t::commit_number_text()
	{
		if (!is_number_type())
			return;

		char* end	= nullptr;
		f32	  value = static_cast<f32>(std::strtod(_text, &end));
		if (end == _text)
			value = 0.0f;
		if (_config.type == editor_input_field_type_e::number_slider)
			value = math::clamp(value, _config.min_value, _config.max_value);
		if (_config.integer)
			value = static_cast<f32>(static_cast<i32>(value + (value >= 0.0f ? 0.5f : -0.5f)));
		_number_value = value;
		format_number();
		refresh_text();
		notify_changed();
	}

	void editor_input_field_t::set_text_raw(const char* value)
	{
		const char* src = value != nullptr ? value : "";
		_text_len		= static_cast<u32>(math::min(static_cast<size_t>(TEXT_CAPACITY - 1), std::strlen(src)));
		std::memcpy(_text, src, _text_len);
		_text[_text_len]  = '\0';
		_caret			  = _text_len;
		_selection_anchor = _caret;
	}

	void editor_input_field_t::format_number()
	{
		if (_config.integer)
			std::snprintf(_text, TEXT_CAPACITY, "%d", static_cast<i32>(_number_value));
		else
			std::snprintf(_text, TEXT_CAPACITY, "%.3f", static_cast<double>(_number_value));
		_text_len		  = static_cast<u32>(std::strlen(_text));
		_caret			  = _text_len;
		_selection_anchor = _caret;
	}

	void editor_input_field_t::insert_char(char c)
	{
		if (!accepts_char(c) || _text_len + 1 >= TEXT_CAPACITY)
			return;
		erase_selection();
		if (_text_len + 1 >= TEXT_CAPACITY)
			return;
		std::memmove(_text + _caret + 1, _text + _caret, _text_len - _caret + 1);
		_text[_caret] = c;
		_text_len++;
		set_caret(_caret + 1);
		if (is_number_type())
		{
			update_number_from_text();
			refresh_text();
			notify_changed();
		}
		else
		{
			refresh_text();
			notify_changed();
		}
	}

	void editor_input_field_t::insert_text(const char* text)
	{
		if (text == nullptr)
			return;
		for (const char* c = text; *c != '\0'; ++c)
			insert_char(*c);
	}

	void editor_input_field_t::erase_selection()
	{
		if (!has_selection())
			return;
		erase_range(selection_min(), selection_max());
	}

	void editor_input_field_t::erase_range(u32 start, u32 end)
	{
		if (start >= end || start > _text_len || end > _text_len)
			return;
		std::memmove(_text + start, _text + end, _text_len - end + 1);
		_text_len -= end - start;
		set_caret(start);
	}

	void editor_input_field_t::set_caret(u32 index)
	{
		_caret			  = math::min(index, _text_len);
		_selection_anchor = _caret;
		reset_caret_blink();
	}

	void editor_input_field_t::select_all()
	{
		_selection_anchor = 0;
		_caret			  = _text_len;
		reset_caret_blink();
	}

	void editor_input_field_t::update_drag_selection(const vec2f_t& pos)
	{
		_caret = index_from_pos(pos);
		reset_caret_blink();
	}

	void editor_input_field_t::apply_number_delta(f32 delta_x)
	{
		if (!is_number_type())
			return;
		set_number(_number_value + delta_x * _config.increment);
		notify_changed();
	}

	void editor_input_field_t::update_number_from_text()
	{
		if (!is_number_type())
			return;
		char* end	= nullptr;
		f32	  value = static_cast<f32>(std::strtod(_text, &end));
		if (end == _text)
			value = 0.0f;
		if (_config.type == editor_input_field_type_e::number_slider)
			value = math::clamp(value, _config.min_value, _config.max_value);
		if (_config.integer)
			value = static_cast<f32>(static_cast<i32>(value + (value >= 0.0f ? 0.5f : -0.5f)));
		_number_value = value;
	}

	u32 editor_input_field_t::index_from_pos(const vec2f_t& pos) const
	{
		if (_text_len == 0)
			return 0;

		const ui::layout_out_t& label_out = _ui->get_tree().out(_label);
		const f32				x		  = pos.x - label_out.pos.x;
		for (u32 i = 0; i < _text_len; ++i)
		{
			const f32 midpoint = (_text_advances[i] + _text_advances[i + 1]) * 0.5f;
			if (x < midpoint)
				return i;
		}
		return _text_len;
	}

	void editor_input_field_t::rebuild_text_advances()
	{
		_text_advances[0] = 0.0f;
		if (_text_len == 0)
			return;

		const editor_theme_t&	   theme	  = editor_theme_t::get();
		const font_runtime_t*	   font		  = resource_manager_t::get().find_runtime<font_runtime_t>(theme.font_default);
		const f32				   size		  = theme.text_default_px_size;
		const u32				   px		  = raster_px_for(size, _ui->get_ui_scale(), _ui->get_dpi_scale());
		const f32				   draw_scale = size / static_cast<f32>(px);
		const ui::vg_text_style_t& text_style = _ui->get_paint().def(_label).text;
		u32						   prev		  = 0;
		f32						   pen		  = 0.0f;

		if (font == nullptr || font->face == nullptr)
		{
			for (u32 i = 0; i < _text_len; ++i)
				_text_advances[i + 1] = static_cast<f32>(i + 1) * theme.text_default_px_size * INPUT_TEXT_WIDTH_FACTOR;
			return;
		}

		ui::glyph_atlas_t& atlas = resource_manager_t::get().get_glyph_atlas();
		for (u32 i = 0; i < _text_len; ++i)
		{
			const u32 c = static_cast<u8>(_text[i]);
			if (prev != 0)
				pen += atlas.get_kern_advance(font, prev, c, px) * draw_scale;

			const ui::glyph_entry_t* g = atlas.request_glyph(font, c, px, text_style.raster_mode);
			pen += g->advance_x * draw_scale + static_cast<f32>(text_style.spacing);
			_text_advances[i + 1] = pen;
			prev				  = c;
		}
	}

	f32 editor_input_field_t::text_width(u32 len) const
	{
		return _text_advances[math::min(len, _text_len)];
	}

	void editor_input_field_t::reset_caret_blink()
	{
		_blink_seconds = 0.0f;
	}

	u32 editor_input_field_t::selection_min() const
	{
		return math::min(_caret, _selection_anchor);
	}

	u32 editor_input_field_t::selection_max() const
	{
		return math::max(_caret, _selection_anchor);
	}

	bool editor_input_field_t::has_selection() const
	{
		return _caret != _selection_anchor;
	}

	bool editor_input_field_t::is_number_type() const
	{
		return _config.type == editor_input_field_type_e::number || _config.type == editor_input_field_type_e::number_slider;
	}

	bool editor_input_field_t::accepts_char(char c) const
	{
		if (!is_number_type())
			return c >= 32;
		if (c >= '0' && c <= '9')
			return true;
		const u32 insert = has_selection() ? selection_min() : _caret;
		const u32 sel0	 = has_selection() ? selection_min() : _caret;
		const u32 sel1	 = has_selection() ? selection_max() : _caret;
		if (c == '-' || c == '+')
		{
			if (insert != 0)
				return false;
			for (u32 i = 0; i < _text_len; ++i)
			{
				if (i >= sel0 && i < sel1)
					continue;
				if (_text[i] == '-' || _text[i] == '+')
					return false;
			}
			return true;
		}
		if (!_config.integer && (c == '.' || c == ','))
		{
			for (u32 i = 0; i < _text_len; ++i)
			{
				if (i >= sel0 && i < sel1)
					continue;
				if (_text[i] == '.' || _text[i] == ',')
					return false;
			}
			return true;
		}
		return false;
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

	void editor_input_field_t::on_focus_gain(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_input_field_t*>(user_data)->reset_caret_blink();
	}

	void editor_input_field_t::on_drag(ui::input_router_t& router, ui::widget_id_t, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		if (router.is_pressed(ui::mouse_button_e::middle) == field._root)
			field.apply_number_delta(pos.x - (pos.x - delta.x));
		else if (router.is_pressed(ui::mouse_button_e::left) == field._root)
			field.update_drag_selection(pos);
	}

	void editor_input_field_t::on_focus_lose(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_input_field_t*>(user_data)->commit_number_text();
	}

	void editor_input_field_t::on_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (!is_press_or_repeat(ev.action))
			return;

		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
		const bool			  ctrl	= is_ctrl_down();

		if (ev.key == static_cast<u16>(input_code::key_left))
		{
			field.set_caret(field.selection_min() > 0 ? field.selection_min() - 1 : 0);
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_right))
		{
			field.set_caret(field.selection_max() < field._text_len ? field.selection_max() + 1 : field._text_len);
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_a) && ctrl)
		{
			field.select_all();
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_c) && ctrl)
		{
			if (field.has_selection())
			{
				char	  clip[TEXT_CAPACITY] = {};
				const u32 start				  = field.selection_min();
				const u32 count				  = field.selection_max() - start;
				std::memcpy(clip, field._text + start, count);
				clip[count] = '\0';
				process::push_clipboard(clip);
			}
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_x) && ctrl)
		{
			if (field.has_selection())
			{
				char	  clip[TEXT_CAPACITY] = {};
				const u32 start				  = field.selection_min();
				const u32 count				  = field.selection_max() - start;
				std::memcpy(clip, field._text + start, count);
				clip[count] = '\0';
				process::push_clipboard(clip);
				field.erase_selection();
				field.update_number_from_text();
				field.refresh_text();
				field.notify_changed();
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
			if (field.has_selection())
				field.erase_selection();
			else if (field._caret > 0)
				field.erase_range(field._caret - 1, field._caret);
			field.update_number_from_text();
			field.refresh_text();
			field.notify_changed();
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_delete))
		{
			if (field.has_selection())
				field.erase_selection();
			else if (field._caret < field._text_len)
				field.erase_range(field._caret, field._caret + 1);
			field.update_number_from_text();
			field.refresh_text();
			field.notify_changed();
			return;
		}
		if (ev.key == static_cast<u16>(input_code::key_return))
		{
			field.commit_number_text();
			return;
		}

		const char c	= process::get_character_from_key(ev.key);
		const u16  mask = process::get_character_mask_from_key(ev.key, c);
		if ((mask & character_mask::printable) != 0)
			field.insert_char(c == ',' ? '.' : c);
	}

	void editor_input_field_t::on_pre_layout_tick(ui::ui_context&, ui::widget_id_t, f32 dt_seconds, void* user_data)
	{
		editor_input_field_t& field = *static_cast<editor_input_field_t*>(user_data);
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
		if (field._config.type != editor_input_field_type_e::number_slider)
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

		if (field.has_selection())
		{
			const f32			x0	 = label_out.pos.x + field.text_width(field.selection_min());
			const f32			x1	 = label_out.pos.x + field.text_width(field.selection_max());
			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_accent1_dim;
			rect.fill_color_b		 = theme.color_accent1_dim;
			canvas.add_rect({x0, y0}, {x1, y1}, rect, state, draw_order);
		}

		if (field._blink_seconds >= INPUT_CARET_BLINK_TIME * 0.5f)
			return;

		const f32			x	 = label_out.pos.x + field.text_width(field._caret);
		ui::vg_line_paint_t line = {};
		line.color				 = theme.color_text0;
		line.thickness			 = INPUT_CARET_WIDTH;
		canvas.add_line({x, y0}, {x, y1}, line, state, draw_order + 1);
	}
}
