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

#include "editor_gui_builder.hpp "
#include "io/log.hpp"
#include "io/assert.hpp"
#include "io/file_system.hpp"

#include "data/string_util.hpp"
#include "data/vector_util.hpp"
#include "data/char_util.hpp"
#include "math/color.hpp"
#include "math/math.hpp"
#include "memory/text_allocator.hpp"
#include "memory/memory.hpp"
#include "common/system_info.hpp"
#include "input/input_mappings.hpp"
#include "gui/vekt.hpp"

#include "gui/icon_defs.hpp"

#include "editor/editor.hpp"
#include "editor/editor_theme.hpp"
#include "editor/editor_settings.hpp"

#include "app/app.hpp"
#include "world/world.hpp"

#include "reflection/reflection.hpp"
#include "platform/window.hpp"
#include "platform/process.hpp"

#undef min
#undef max

namespace SFG
{
	using namespace vekt;

	// -----------------------------------------------------------------------------
	// gui text field
	// -----------------------------------------------------------------------------

	namespace
	{
		static inline unsigned int umin(unsigned int a, unsigned int b)
		{
			return a < b ? a : b;
		}
		static inline unsigned int umax(unsigned int a, unsigned int b)
		{
			return a > b ? a : b;
		}
	}

	unsigned int gui_builder::gui_text_field::selection_min() const
	{
		return umin(caret_pos, caret_end_pos);
	}
	unsigned int gui_builder::gui_text_field::selection_max() const
	{
		return umax(caret_pos, caret_end_pos);
	}
	void gui_builder::gui_text_field::collapse_caret_to(unsigned int p)
	{
		p			  = umin(p, buffer_size);
		caret_pos	  = p;
		caret_end_pos = p;
	}

	void gui_builder::gui_text_field::delete_range(unsigned int from, unsigned int to)
	{
		if (from > to)
		{
			unsigned int tmp = from;
			from			 = to;
			to				 = tmp;
		}
		from = umin(from, buffer_size);
		to	 = umin(to, buffer_size);
		if (from == to)
			return;

		const unsigned int tail_count = (buffer_size - to) + 1;
		SFG_MEMMOVE((char*)buffer + from, buffer + to, tail_count);

		buffer_size -= (to - from);
		collapse_caret_to(from);
	}
	bool gui_builder::gui_text_field::delete_selection_if_any()
	{
		const unsigned int mn = selection_min();
		const unsigned int mx = selection_max();
		if (mn == mx)
			return false;
		delete_range(mn, mx);
		return true;
	}
	void gui_builder::gui_text_field::insert_string_at_caret(const char* s, unsigned int len)
	{
		if (!s || len == 0)
			return;

		delete_selection_if_any();

		const unsigned int available = (buffer_capacity > 0) ? (buffer_capacity - 1) : 0;
		if (buffer_size >= available)
			return;

		const unsigned int can_add = umin(len, available - buffer_size);
		if (can_add == 0)
			return;

		const unsigned int pos = umin(caret_pos, buffer_size);

		const unsigned int tail_count = (buffer_size - pos) + 1;
		SFG_MEMMOVE((char*)buffer + pos + can_add, buffer + pos, tail_count);
		SFG_MEMCPY((char*)buffer + pos, s, can_add);

		buffer_size += can_add;
		collapse_caret_to(pos + can_add);
	}
	void gui_builder::gui_text_field::insert_char_at_caret(char c)
	{
		insert_string_at_caret(&c, 1);
	}

	namespace
	{
		void on_hover_begin_hand_c(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::hand);
		}

		void on_hover_end_hand_c(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::arrow);
		}

		void on_hover_begin_text_field(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::caret);
		}

		void on_hover_end_text_field(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::arrow);
		}

	}

	vekt::input_event_result gui_builder::on_text_field_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type == vekt::input_event_type::released)
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		if (ev.type == vekt::input_event_type::repeated)
		{
			it->caret_pos	  = 0;
			it->caret_end_pos = it->buffer_size;
			return vekt::input_event_result::handled;
		}

		const vekt::id text_widget = it->text_widget;

		if (it->buffer_size == 0)
		{
			it->caret_end_pos = 0;
		}
		else
		{
			const vector2 pos		= b->widget_get_pos(text_widget);
			const vector2 text_size = b->widget_get_size(text_widget);
			const f32	  x_diff	= ev.position.x - pos.x;

			it->caret_pos = math::clamp(b->widget_get_character_index(it->text_widget, x_diff), (u32)0, it->buffer_size);
		}
		it->caret_end_pos = it->caret_pos;
		return vekt::input_event_result::handled;
	}

	void gui_builder::on_text_field_drag(vekt::builder* b, vekt::id widget, f32 mp_x, f32 mp_y, f32 delta_x, f32 delta_y, unsigned int button)
	{
		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		if (button == input_code::mouse_middle)
		{
			if (it->is_slider)
			{
				const vector2 pos	= b->widget_get_pos(it->widget);
				const vector2 size	= b->widget_get_size(it->widget);
				const f32	  ratio = math::clamp(math::remap(mp_x, pos.x, pos.x + size.x, 0.0f, 1.0f), 0.0f, 1.0f);
				it->value			= it->min + (it->max - it->min) * ratio;
				it->value			= math::clamp(it->value, it->min, it->max);

				if (it->is_slider == 2)
					it->value = static_cast<u32>(it->value);

				size_props& sz = b->widget_get_size_props(it->sliding_widget);
				sz.size.x	   = ratio;
			}
			else
			{
				it->value += math::clamp(delta_x, -1.0f, 1.0f) * it->value_increment;
			}

			gb->text_field_edit_complete(*it);

			if (it->type == gui_text_field_type::text_only)
				gb->invoke_reflection(it->widget, (void*)it->buffer, it->sub_index);
			else if (it->type == gui_text_field_type::number)
				gb->invoke_reflection(it->widget, &it->value, it->sub_index);

			if (gb->callbacks.on_input_field_changed)
				gb->callbacks.on_input_field_changed(gb->callbacks.callback_ud, gb->_builder, widget, it->buffer, it->value);

			return;
		}

		if (button != input_code::mouse_0)
			return;

		if (it->buffer_size == 0)
		{
			it->caret_end_pos = 0;
			return;
		}
		const vekt::id text_widget = it->text_widget;

		const vector2 pos		= b->widget_get_pos(text_widget);
		const vector2 text_size = b->widget_get_size(text_widget);
		const f32	  x_diff	= mp_x - pos.x;

		if (x_diff < 0)
		{
			it->caret_end_pos = 0;
			return;
		}
		it->caret_end_pos = math::clamp(b->widget_get_character_index(it->text_widget, x_diff), (u32)0, it->buffer_size);
	}

	void gui_builder::on_text_field_focus_lost(vekt::builder* b, vekt::id widget)
	{
		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		if (it == gb->_text_fields.end())
			return;
		gb->text_field_edit_complete(*it);
	}

	void gui_builder::on_text_field_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav)
	{

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		if (from_nav)
		{
			it->caret_pos	  = 0;
			it->caret_end_pos = it->buffer_size;
		}
	}

	void gui_builder::on_context_item_hover_begin(vekt::builder* b, vekt::id widget)
	{
		gui_builder* gb					= static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		b->widget_get_gfx(widget).color = editor_theme::get().col_highlight_transparent;
	}

	void gui_builder::on_context_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_gfx(widget).color = {};
	}

	vekt::input_event_result gui_builder::on_toggle_button_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		gui_builder* self = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it	  = vector_util::find_if(self->_toggle_buttons, [widget](const gui_toggle_button& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != self->_toggle_buttons.end());

		if (ev.button == mouse_0 && ev.type == vekt::input_event_type::released)
		{
			self->set_toggle_button_state(widget, !it->toggled);

			if (self->callbacks.on_toggle_button)
				self->callbacks.on_toggle_button(self->callbacks.callback_ud, b, widget, it->toggled);
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	void gui_builder::on_text_field_draw(vekt::builder* b, vekt::id widget)
	{
		hover_callback& hb = b->widget_get_hover_callbacks(widget);
		if (!hb.is_focused)
			return;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		const gui_text_field& tf = *it;

		const vector2 pos		= b->widget_get_pos(tf.text_widget) + vector2(0.0f, 0.0f);
		const vector2 text_size = b->widget_get_size(tf.text_widget);

		const unsigned int min		  = math::min(tf.caret_pos, tf.caret_end_pos);
		const unsigned int max		  = math::max(tf.caret_pos, tf.caret_end_pos);
		const f32		   min_offset = b->widget_get_character_offset(tf.text_widget, min);
		const f32		   max_offset = b->widget_get_character_offset(tf.text_widget, max);

		const widget_gfx gfx = {
			.draw_order = b->widget_get_gfx(tf.text_widget).draw_order,
			.flags		= gfx_flags::gfx_is_rect,
		};

		static f32 timer = 0.0f;

		if (timer > 500)
		{
			// caret

			b->add_filled_rect({
				.gfx			 = gfx,
				.min			 = pos + vector2(min_offset, 0.0f),
				.max			 = pos + vector2(min_offset + 1.0f * editor_theme::UI_SCALING, text_size.y),
				.color_start	 = editor_theme::get().col_text_dim,
				.color_end		 = editor_theme::get().col_text_dim,
				.color_direction = vekt::direction::horizontal,
				.widget_id		 = widget,
				.multi_color	 = false,
			});

			if (timer > 1500)
				timer = 0.0f;
		}

		timer += g_frame_info.main_thread_time_milli.load();

		if (min == max)
			return;

		// highlight
		b->add_filled_rect({
			.gfx			 = gfx,
			.min			 = pos + vector2(min_offset, 0.0f),
			.max			 = pos + vector2(max_offset, text_size.y),
			.color_start	 = editor_theme::get().col_highlight_transparent,
			.color_end		 = editor_theme::get().col_highlight_transparent,
			.color_direction = vekt::direction::horizontal,
			.widget_id		 = widget,
			.multi_color	 = false,
		});
	}

	vekt::input_event_result gui_builder::on_text_field_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev)
	{
		if (ev.type != vekt::input_event_type::pressed || (ev.type == vekt::input_event_type::repeated && ev.key == input_code::key_backspace))
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		gui_text_field& tf	   = *it;
		char*			buffer = (char*)tf.buffer;

		const unsigned int capacity = tf.buffer_capacity;
		SFG_ASSERT(capacity != 0);

		// Clamp caret positions into [0, tf.buffer_size]
		tf.caret_pos	 = umin(tf.caret_pos, tf.buffer_size);
		tf.caret_end_pos = umin(tf.caret_end_pos, tf.buffer_size);

		const bool ctrl = window::is_key_down(input_code::key_lctrl);

		bool text_changed = false;

		if (ev.key == input_code::key_left)
		{
			const unsigned int mn = tf.selection_min();
			if (mn > 0)
				tf.collapse_caret_to(mn - 1);
			else
				tf.collapse_caret_to(0);
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_right)
		{
			const unsigned int mx = tf.selection_max();
			if (mx < tf.buffer_size)
				tf.collapse_caret_to(mx + 1);
			else
				tf.collapse_caret_to(tf.buffer_size);
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_return)
		{
			gb->text_field_edit_complete(tf);
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_backspace)
		{
			if (!tf.delete_selection_if_any())
			{
				if (tf.caret_pos > 0)
				{
					const unsigned int p = umin(tf.caret_pos, tf.buffer_size);
					tf.delete_range(p - 1, p);
				}
			}
			text_changed = true;
		}
		else if (ev.key == input_code::key_a && ctrl)
		{
			tf.caret_pos	 = 0;
			tf.caret_end_pos = tf.buffer_size;
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_x && ctrl)
		{
			const unsigned int mn = tf.selection_min();
			const unsigned int mx = tf.selection_max();
			if (mn != mx)
			{
				const string slice(buffer + mn, buffer + mx);
				process::push_clipboard(slice.c_str());

				if (!tf.delete_selection_if_any())
				{
					if (tf.caret_pos > 0)
					{
						const unsigned int p = umin(tf.caret_pos, tf.buffer_size);
						tf.delete_range(p - 1, p);
					}
				}
				text_changed = true;
			}
		}
		else if (ev.key == input_code::key_c && ctrl)
		{
			const unsigned int mn = tf.selection_min();
			const unsigned int mx = tf.selection_max();
			if (mn != mx)
			{
				const string slice(buffer + mn, buffer + mx);
				process::push_clipboard(slice.c_str());
			}
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_v && ctrl)
		{
			const string clip = process::get_clipboard();
			tf.insert_string_at_caret(clip.c_str(), (unsigned int)clip.size());
			text_changed = true;

			return vekt::input_event_result::handled;
		}
		else
		{
			const char c	= process::get_character_from_key(static_cast<u32>(ev.key));
			const u16  mask = process::get_character_mask_from_key(static_cast<u32>(ev.key), c);
			if (!(mask & character_mask::printable))
				return vekt::input_event_result::not_handled;

			tf.insert_char_at_caret(c);
			text_changed = true;
		}

		if (text_changed)
		{
			if (capacity > 0)
			{
				const unsigned int safe_end = umin(tf.buffer_size, capacity - 1);
				buffer[safe_end]			= '\0';
				tf.buffer_size				= safe_end;
			}

			if (tf.type == gui_text_field_type::number)
			{
				if (tf.decimals == 0)
				{
					int val;
					if (string_util::to_int(tf.buffer, val))
						tf.value = val;
					else
						tf.value = 0.0f;
				}
				else
				{
					f32 val	  = 0.0f;
					u32 out_d = 0;

					char_util::replace_all((char*)tf.buffer, ',', '.');
					if (string_util::to_float(tf.buffer, val, out_d))
					{
						tf.value = val;
					}
					else
						tf.value = 0.0f;
				}
			}

			if (gb->callbacks.on_input_field_changed)
				gb->callbacks.on_input_field_changed(gb->callbacks.callback_ud, gb->_builder, widget, tf.buffer, tf.value);
			b->widget_update_text(tf.text_widget);

			if (tf.is_slider)
			{
				tf.value	   = math::clamp(tf.value, tf.min, tf.max);
				size_props& sz = b->widget_get_size_props(tf.sliding_widget);
				sz.size.x	   = math::remap(tf.value, tf.min, tf.max, 0.0f, 1.0f);
			}

			if (it->type == gui_text_field_type::text_only)
				gb->invoke_reflection(it->widget, (void*)it->buffer, it->sub_index);
			else if (it->type == gui_text_field_type::number)
			{
				gb->invoke_reflection(it->widget, &it->value, it->sub_index);
			}
		}

		return vekt::input_event_result::handled;
	}

	vekt::input_event_result gui_builder::on_checkbox_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type != vekt::input_event_type::pressed || ev.button != static_cast<u16>(input_code::mouse_0))
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_checkboxes, [widget](const gui_checkbox& cb) -> bool { return cb.widget == widget; });
		SFG_ASSERT(it != gb->_checkboxes.end());

		it->state ^= 1;
		b->widget_set_visible(it->text_widget, it->state);

		bool data = it->state;
		gb->invoke_reflection(it->widget, &data);
		if (gb->callbacks.on_checkbox_changed)
			gb->callbacks.on_checkbox_changed(gb->callbacks.callback_ud, b, widget, it->state);

		return vekt::input_event_result::handled;
	}

	vekt::input_event_result gui_builder::on_resource_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type != vekt::input_event_type::pressed || ev.button != static_cast<u16>(input_code::mouse_0))
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);

		auto it = vector_util::find_if(gb->_resources, [widget](const gui_resource& r) -> bool { return r.widget == widget; });
		SFG_ASSERT(it != gb->_resources.end());

		if (it->extension == nullptr || strlen(it->extension) == 0)
			return vekt::input_event_result::handled;

		string file = process::select_file("select", it->extension);
		file_system::fix_path(file);

		if (!editor_settings::get().is_in_work_directory(file))
		{
			SFG_ERR("resource selected must be inside project directory!");
			return vekt::input_event_result::handled;
		}

		resource_manager& rm	   = editor::get().get_app().get_world().get_resource_manager();
		const string	  relative = editor_settings::get().get_relative(file);
		const string_id	  hash	   = TO_SID(relative);
		resource_handle	  h		   = rm.get_resource_handle_by_hash_if_exists(it->type, hash);

		if (h.is_null())
		{
			rm.load_resources({relative}, false, editor_settings::get().working_dir.c_str());
			h = rm.get_resource_handle_by_hash(it->type, hash);
		}
		gb->invoke_reflection(it->widget, &h);

		if (gb->callbacks.on_resource_changed)
			gb->callbacks.on_resource_changed(gb->callbacks.callback_ud, b, widget, file);
		b->widget_set_text(it->text_widget, file.c_str());

		return vekt::input_event_result::handled;
	}

	// -----------------------------------------------------------------------------
	// big layout
	// -----------------------------------------------------------------------------

	void gui_builder::init(vekt::builder* b)
	{
		_builder = b;
		_text_fields.reserve(256);
		_checkboxes.reserve(256);
		_resources.reserve(256);
		_dropdowns.reserve(128);
		_reflected.reserve(256);
		_control_buttons.reserve(128);
		_toggle_buttons.reserve(128);

		const id w = _builder->allocate();

		// gfx
		_builder->widget_get_gfx(w).color = editor_theme::get().col_root;
		_builder->widget_get_gfx(w).flags = gfx_flags::gfx_is_rect;

		// positioning
		vekt::size_props& sz = _builder->widget_get_size_props(w);
		sz.flags			 = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_relative;
		sz.size.x			 = 1.0f;
		sz.size.y			 = 1.0f;

		// sizes
		_builder->widget_get_size_props(w).spacing		 = editor_theme::get().root_spacing;
		_builder->widget_get_size_props(w).child_margins = {editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin};
		pos_props& pp									 = _builder->widget_get_pos_props(w);
		pp.flags										 = pos_flags::pf_child_pos_column;
		_root											 = w;
	}

	void gui_builder::uninit()
	{
		for (const gui_resource& r : _resources)
		{
			editor::get().get_text_allocator().deallocate(r.extension);
		}

		for (const gui_dropdown& d : _dropdowns)
		{
			for (const char* it : d.items)
				editor::get().get_text_allocator().deallocate(it);
		}

		_resources.resize(0);
		_dropdowns.resize(0);
		_checkboxes.resize(0);
		_text_fields.resize(0);
		_reflected.resize(0);
		_control_buttons.resize(0);
		_toggle_buttons.resize(0);

		_builder->deallocate(_root);
		_root = NULL_WIDGET_ID;
	}

	void gui_builder::invoke_reflection(vekt::id widget, void* data_ptr, unsigned int sub_index)
	{
		auto ref = vector_util::find_if(_reflected, [widget](const reflected_property& rf) -> bool { return widget == rf.widget; });
		if (ref != _reflected.end())
		{
			meta& m = reflection::get().resolve(ref->type);

			// update underlying value
			const reflected_field_type ft  = ref->field->_type;
			void*					   ptr = ref->field->value(ref->obj).cast_ptr<void>();

			if (ft == reflected_field_type::rf_vector2 || ft == reflected_field_type::rf_vector3 || ft == reflected_field_type::rf_vector4 || ft == reflected_field_type::rf_color)
				SFG_MEMCPY((u8*)ptr + sizeof(f32) * sub_index, data_ptr, sizeof(f32));
			else if (ft == reflected_field_type::rf_vector2ui16)
			{
				const f32 f = *reinterpret_cast<f32*>(data_ptr);
				const u16 u = static_cast<u16>(f);
				SFG_MEMCPY((u8*)ptr + sizeof(u) * sub_index, &u, sizeof(u16));
			}
			else if (ft == reflected_field_type::rf_string)
			{
				string* str = reinterpret_cast<string*>(ptr);
				*str		= (const char*)data_ptr;
			}
			else if (ft == reflected_field_type::rf_bool)
			{
				const bool b = *reinterpret_cast<u8*>(data_ptr);
				SFG_MEMCPY(ptr, &b, sizeof(bool));
			}
			else if (ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_enum)
			{
				const u8 u = *reinterpret_cast<u8*>(data_ptr);
				SFG_MEMCPY(ptr, &u, sizeof(u8));
			}
			else if (ft == reflected_field_type::rf_uint)
			{
				const f32 f	  = *reinterpret_cast<f32*>(data_ptr);
				const u32 u32 = static_cast<f32>(f);
				SFG_MEMCPY(ptr, &u32, sizeof(u32));
			}
			else if (ft == reflected_field_type::rf_int)
			{
				const f32 f = *reinterpret_cast<f32*>(data_ptr);
				const i32 i = static_cast<i32>(f);
				SFG_MEMCPY(ptr, &i, sizeof(i32));
			}
			else if (ft == reflected_field_type::rf_resource && ref->field->_is_list)
			{
				vector<resource_handle>& v	 = ref->field->value(ref->obj).cast_ref<vector<resource_handle>>();
				resource_handle			 res = *reinterpret_cast<resource_handle*>(data_ptr);
				v[ref->list_index]			 = res;
			}
			else
				SFG_MEMCPY(ptr, data_ptr, ref->field->get_type_size());

			if (m.has_function("on_reflected_changed"_hs))
			{
				const reflected_field_changed_params params = {
					.w			 = editor::get().get_app().get_world(),
					.object_ptr	 = ref->obj,
					.data_ptr	 = data_ptr,
					.field_title = TO_SID(ref->field->_title),
					.list_index	 = ref->list_index,
				};
				m.invoke_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, params);
			}
		}
	}

	bool gui_builder::invoke_control_button(vekt::id widget)
	{
		auto it = vector_util::find_if(_control_buttons, [widget](const control_button_binding& b) -> bool { return b.widget == widget; });
		if (it == _control_buttons.end())
			return false;

		if (callbacks.on_control_button)
			callbacks.on_control_button(callbacks.callback_ud, it->obj, it->type, it->button_id);
		return true;
	}

	void gui_builder::set_toggle_button_state(vekt::id widget, bool toggled)
	{
		auto it = vector_util::find_if(_toggle_buttons, [widget](const gui_toggle_button& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != _toggle_buttons.end());

		it->toggled = toggled;

		vekt::widget_gfx& gfx = _builder->widget_get_gfx(widget);
		gfx.color			  = it->toggled ? editor_theme::get().col_accent_second_dim : vector4();
	}

	void gui_builder::remove_impl(vekt::id id)
	{
		auto it_res = std::find_if(_resources.begin(), _resources.end(), [id](const gui_resource& it) -> bool { return it.widget == id; });
		auto it_txt = std::find_if(_text_fields.begin(), _text_fields.end(), [id](const gui_text_field& it) -> bool { return it.widget == id; });
		auto it_c	= std::find_if(_checkboxes.begin(), _checkboxes.end(), [id](const gui_checkbox& it) -> bool { return it.widget == id; });
		auto it_dd	= std::find_if(_dropdowns.begin(), _dropdowns.end(), [id](const gui_dropdown& it) -> bool { return it.widget == id; });
		auto it_r	= std::find_if(_reflected.begin(), _reflected.end(), [id](const reflected_property& it) -> bool { return it.widget == id; });
		auto it_b	= std::find_if(_control_buttons.begin(), _control_buttons.end(), [id](const control_button_binding& it) -> bool { return it.widget == id; });
		auto it_tb	= std::find_if(_toggle_buttons.begin(), _toggle_buttons.end(), [id](const gui_toggle_button& it) -> bool { return it.widget == id; });

		if (it_tb != _toggle_buttons.end())
			_toggle_buttons.erase(it_tb);

		if (it_r != _reflected.end())
			_reflected.erase(it_r);

		if (it_b != _control_buttons.end())
			_control_buttons.erase(it_b);

		if (it_res != _resources.end())
			_resources.erase(it_res);

		if (it_dd != _dropdowns.end())
		{
			for (const char* s : it_dd->items)
				editor::get().get_text_allocator().deallocate(s);
			_dropdowns.erase(it_dd);
		}

		if (it_txt != _text_fields.end())
			_text_fields.erase(it_txt);

		if (it_c != _checkboxes.end())
			_checkboxes.erase(it_c);

		const vekt::widget_meta& meta = _builder->widget_get_meta(id);

		for (vekt::id c : meta.children)
		{
			remove_impl(c);
		}
	}

	void gui_builder::deallocate_children(vekt::id id)
	{
		const vekt::widget_meta& meta = _builder->widget_get_meta(id);

		for (vekt::id c : meta.children)
		{
			remove_impl(c);
			_builder->deallocate(c);
		}
	}

	void gui_builder::deallocate(vekt::id id)
	{
		remove_impl(id);
		_builder->deallocate(id);
	}

	id gui_builder::begin_area(bool fill, bool scroller)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_child_pos_column;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_relative;
			sz.size.x	   = 1.0f;

			if (fill)
			{
				sz.flags |= size_flags::sf_y_fill;
			}
			else
			{
				sz.flags |= size_flags::sf_y_total_children;
			}
			sz.spacing		 = editor_theme::get().item_spacing;
			sz.child_margins = {editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding | gfx_flags::gfx_clip_children;
			gfx.color		= editor_theme::get().col_area_bg;
			gfx.draw_order	= _draw_order;

			// rounding_props& rp = _builder->widget_get_rounding(w);
			// rp.rounding		   = editor_theme::get().area_rounding;
			// rp.segments		   = 16;

			if (scroller)
			{
				// gfx.color			   = vector4(1, 0, 0, 1);
				sz.child_margins.left  = 0.0f;
				sz.child_margins.right = 0.0f;
			}
		}

		if (scroller)
		{
			const id scroll_bg = new_widget(true);
			{
				pos_props& pp = _builder->widget_get_pos_props(scroll_bg);
				pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_end | pos_flags::pf_overlay;
				pp.pos		  = VEKT_VEC2(1.0f, 0.0f);

				size_props& sz = _builder->widget_get_size_props(scroll_bg);
				sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_relative;
				sz.size.x	   = editor_theme::get().scroll_thickness;
				sz.size.y	   = 1.0f;

				widget_gfx& gfx = _builder->widget_get_gfx(scroll_bg);
				gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
				gfx.color		= editor_theme::get().col_scroll_bar_bg;

				rounding_props& rp = _builder->widget_get_rounding(scroll_bg);
				rp.segments		   = 16;
				rp.rounding		   = editor_theme::get().scroll_rounding;
			}

			const id scroll = new_widget();
			{
				pos_props& pp = _builder->widget_get_pos_props(scroll);
				pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative;
				pp.pos		  = VEKT_VEC2(0.0f, 0.0f);

				size_props& sz = _builder->widget_get_size_props(scroll);
				sz.flags	   = size_flags::sf_x_relative;
				sz.size.x	   = 1.0f;

				scroll_props& sc = _builder->widget_get_scroll_props(scroll);
				sc.scroll_parent = w;

				widget_gfx& gfx = _builder->widget_get_gfx(scroll);
				gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
				gfx.color		= editor_theme::get().col_scroll_bar;

				rounding_props& rp = _builder->widget_get_rounding(scroll);
				rp.segments		   = 16;
				rp.rounding		   = editor_theme::get().scroll_rounding;

				hover_callback& hb = _builder->widget_get_hover_callbacks(scroll);
				hb.on_hover_begin  = on_hover_begin_hand_c;
				hb.on_hover_end	   = on_hover_end_hand_c;
			}

			pop_stack();
		}

		return w;
	}

	void gui_builder::end_area()
	{
		pop_stack();
	}

	vekt::id gui_builder::begin_row()
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_child_pos_row;
			pp.pos		  = VEKT_VEC2(0.0f, 0.0f);

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_total_children | size_flags::sf_y_max_children;
			sz.spacing		 = editor_theme::get().row_spacing;
			sz.child_margins = {editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= editor_theme::get().col_root;
			gfx.draw_order	= _draw_order;

			_builder->widget_get_user_data(w).ptr = callbacks.user_data;
		}
		return w;
	}

	void gui_builder::end_row()
	{
		pop_stack();
	}

	vekt::id gui_builder::begin_column()
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_child_pos_column;
			pp.pos		  = VEKT_VEC2(0.0f, 0.0f);

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_max_children | size_flags::sf_y_total_children;
			sz.spacing		 = editor_theme::get().row_spacing;
			sz.child_margins = {editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= editor_theme::get().col_root;
			gfx.draw_order	= _draw_order;

			_builder->widget_get_user_data(w).ptr = callbacks.user_data;
		}
		return w;
	}

	void gui_builder::end_column()
	{
		pop_stack();
	}

	// -----------------------------------------------------------------------------
	// property - single
	// -----------------------------------------------------------------------------

	id gui_builder::add_property_single_label(const char* label, size_t buffer_capacity)
	{
		const id row = add_property_row();
		const id w	 = add_label(label, buffer_capacity);
		pop_stack();
		return w;
	}

	gui_builder::id_pair gui_builder::add_property_single_button(const char* label, size_t buffer_capacity)
	{
		const id	  row = add_property_row();
		const id_pair p	  = add_button(label, buffer_capacity);
		pop_stack();
		return p;
	}

	id gui_builder::add_property_single_hyperlink(const char* label, size_t buffer_capacity)
	{
		const id row = add_property_row();
		const id w	 = add_hyperlink(label, buffer_capacity);
		pop_stack();
		return w;
	}

	// -----------------------------------------------------------------------------
	// property - rows
	// -----------------------------------------------------------------------------

	gui_builder::id_pair gui_builder::add_property_row_checkbox(const char* label, bool initial_state)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id w = add_checkbox(initial_state);
		pop_stack();
		pop_stack();
		return {row, w};
	}

	gui_builder::id_pair gui_builder::add_property_row_resource(const char* label, const char* extension, const char* initial_resource, string_id type_id, size_t buffer_capacity)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id w = add_resource(initial_resource, extension, type_id, buffer_capacity);
		pop_stack();

		pop_stack();
		return {row, w};
	}

	gui_builder::id_pair gui_builder::add_property_row_dropdown(const char* label, const char* initial_text, size_t buffer_capacity)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id w = add_dropdown(initial_text, buffer_capacity);
		pop_stack();

		pop_stack();
		return {row, w};
	}

	gui_builder::id_pair gui_builder::add_property_row_slider(const char* label, size_t buffer_capacity, f32 min, f32 max, f32 val, bool is_int)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id w = add_text_field("0.000", buffer_capacity, gui_text_field_type::number, is_int ? 0 : 3, 0, min, max, val, is_int ? 2 : 1).first;
		set_text_field_value(w, val, false, is_int);
		pop_stack();

		pop_stack();
		return {row, w};
	}

	vekt::id gui_builder::add_reflected_field(field_base* field, string_id type_id, void* object_ptr)
	{

		if (field->_no_ui)
			return 0;

		const char*				   title		 = field->_title.c_str();
		const char*				   tooltip		 = field->_tooltip.c_str();
		const f32				   min			 = field->_min;
		const f32				   max			 = field->_max;
		const reflected_field_type type			 = field->_type;
		const string_id			   field_type_id = field->_sub_type_id;
		const u8				   clamped		 = field->_clamped;

		if (type == reflected_field_type::rf_bool)
		{
			const bool	  val = field->value(object_ptr).cast<bool>();
			const id_pair ids = add_property_row_checkbox(title, val);

			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_uint8)
		{
			const u8	  val = field->value(object_ptr).cast<u8>();
			const id_pair ids = clamped ? add_property_row_slider(title, 16, min, max, val, true) : add_property_row_slider(title, 16, 0, 255, val, true);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_float)
		{
			const f32 val = field->value(object_ptr).cast<f32>();
			if (clamped)
			{
				const id_pair p = add_property_row_slider(title, 16, min, max, val);
				_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = p.second});
				return p.first;
			}

			const id_trip ids = add_property_row_text_field(title, "", 16, gui_text_field_type::number, 3, 0.1f);
			set_text_field_value(ids.second, val, false);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_int)
		{
			const i32	  val = field->value(object_ptr).cast<i32>();
			const id_pair p	  = clamped ? add_property_row_slider(title, 16, min, max, val, true) : add_property_row_slider(title, 16, 0, UINT32_MAX, val, true);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = p.second});
			return p.first;
		}
		else if (type == reflected_field_type::rf_uint)
		{
			const u32	  val = field->value(object_ptr).cast<u32>();
			const id_pair p	  = clamped ? add_property_row_slider(title, 16, min, max, val, true) : add_property_row_slider(title, 16, 0, UINT32_MAX, val, true);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = p.second});
			return p.first;
		}

		else if (type == reflected_field_type::rf_vector2)
		{
			const vector2 val = field->value(object_ptr).cast<vector2>();
			const id_trip ids = add_property_row_vector2(title, "", 16, 3, 0.1f);
			set_text_field_value(ids.second, val.x, false);
			set_text_field_value(ids.third, val.y, false);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.third});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_vector2ui16)
		{
			const vector2ui16 val = field->value(object_ptr).cast<vector2ui16>();
			const id_trip	  ids = add_property_row_vector2(title, "", 16, 0, 1.0f);
			set_text_field_value(ids.second, val.x, false, true);
			set_text_field_value(ids.third, val.y, false, true);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.third});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_vector3)
		{
			const vector3 val = field->value(object_ptr).cast<vector3>();
			const id_quat ids = add_property_row_vector3(title, "", 16, 3, 0.1f);
			set_text_field_value(ids.second, val.x, false);
			set_text_field_value(ids.third, val.y, false);
			set_text_field_value(ids.fourth, val.z, false);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.third});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.fourth});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_vector4)
		{
			const vector4  val = field->value(object_ptr).cast<vector4>();
			const id_penth ids = add_property_row_vector4(title, "", 16, 3, 0.1f);
			set_text_field_value(ids.second, val.x, false);
			set_text_field_value(ids.third, val.y, false);
			set_text_field_value(ids.fourth, val.z, false);
			set_text_field_value(ids.fifth, val.w, false);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.third});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.fourth});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.fifth});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_color)
		{
			const color	   val = field->value(object_ptr).cast<color>();
			const id_penth ids = add_property_row_vector4(title, "", 16, 3, 0.1f);
			set_text_field_value(ids.second, val.x, false);
			set_text_field_value(ids.third, val.y, false);
			set_text_field_value(ids.fourth, val.z, false);
			set_text_field_value(ids.fifth, val.w, false);
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.third});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.fourth});
			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.fifth});
			return ids.first;
		}
		else if (type == reflected_field_type::rf_enum)
		{
			const u8 val = field->value(object_ptr).cast<u8>();
			SFG_ASSERT(field->_enum_list.size() > val);

			const id_pair ids = add_property_row_dropdown(title, field->_enum_list[val].c_str(), 256);

			u8 i = 0;
			for (const malloc_string& str : field->_enum_list)
			{
				add_dropdown_item(ids.second, str.c_str(), val == i);
				i++;
			}

			_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});

			return ids.first;
		}
		else if (type == reflected_field_type::rf_resource)
		{
			const char* extension = reflection::get().resolve(field_type_id).get_tag_str().c_str();

			if (field->_is_list)
			{
				vector<resource_handle>& v	= field->value(object_ptr).cast_ref<vector<resource_handle>>();
				const u32				 sz = static_cast<u32>(v.size());

				for (u32 i = 0; i < sz; i++)
				{
					const resource_handle h	 = v[i];
					vekt::id			  pp = NULL_WIDGET_ID;

					if (h.is_null())
					{
						pp = add_property_row_resource(title, extension, "no_resource", field->_sub_type_id, 512).second;
					}
					else
					{
						world&			  wrld		   = editor::get().get_app().get_world();
						resource_manager& rm		   = wrld.get_resource_manager();
						void*			  resource_ptr = rm.get_resource(field_type_id, h);
						const string&	  path		   = rm.get_loaded_path_by_handle(field_type_id, h);
						pp							   = add_property_row_resource(title, extension, path.c_str(), field->_sub_type_id, 512).second;
					}

					_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = pp, .list_index = i});
				}
				return 0;
			}
			else
			{
				id_pair ids = {};

				const resource_handle h = field->value(object_ptr).cast<resource_handle>();

				if (h.is_null())
				{
					ids = add_property_row_resource(title, extension, "no_resource", field->_sub_type_id, 512);
				}
				else
				{
					world&			  wrld		   = editor::get().get_app().get_world();
					resource_manager& rm		   = wrld.get_resource_manager();
					void*			  resource_ptr = rm.get_resource(field_type_id, h);
					const string&	  path		   = rm.get_loaded_path_by_handle(field_type_id, h);
					ids							   = add_property_row_resource(title, extension, path.c_str(), field->_sub_type_id, 512);
				}
				_reflected.push_back({.obj = object_ptr, .type = type_id, .field = field, .widget = ids.second});
				return ids.first;
			}
		}
		else if (type == reflected_field_type::rf_entity)
		{
			return 0;
		}

		SFG_ASSERT(false);

		return 0;
	}

	vekt::id gui_builder::add_control_button(const char* title, string_id type_id, void* object_ptr, string_id button_id)
	{
		const id_pair ids = add_button(title);
		_control_buttons.push_back({.obj = object_ptr, .type = type_id, .button_id = button_id, .widget = ids.first});
		return ids.first;
	}

	gui_builder::id_pair gui_builder::add_property_row_label(const char* label, const char* text, size_t buffer_capacity)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		const id id0 = add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id id1 = add_label(text, buffer_capacity);
		pop_stack();

		pop_stack();

		return {id0, id1};
	}

	gui_builder::id_trip gui_builder::add_property_row_text_field(const char* label, const char* text, size_t buffer_capacity, gui_text_field_type type, unsigned int decimals, f32 increment)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id_pair field = add_text_field(text, buffer_capacity, type, decimals, increment);
		pop_stack();
		pop_stack();
		return {row, field.first, field.second};
	}

	gui_builder::id_trip gui_builder::add_property_row_vector2(const char* label, const char* text, size_t buffer_capacity, unsigned int decimals, f32 increment)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id_pair x = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 0);
		pop_stack();

		add_row_cell(0.0f);
		const id_pair y = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 1);
		pop_stack();

		pop_stack();
		return {row, x.first, y.first};
	}

	gui_builder::id_quat gui_builder::add_property_row_vector3(const char* label, const char* text, size_t buffer_capacity, unsigned int decimals, f32 increment)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id_pair x = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 0);
		pop_stack();

		add_row_cell(0.0f);
		const id_pair y = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 1);
		pop_stack();

		add_row_cell(0.0f);
		const id_pair z = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 2);
		pop_stack();
		pop_stack();
		return {row, x.first, y.first, z.first};
	}

	gui_builder::id_penth gui_builder::add_property_row_vector4(const char* label, const char* text, size_t buffer_capacity, unsigned int decimals, f32 increment)
	{
		const id row = add_property_row();

		add_row_cell(editor_theme::get().property_cell_div);
		add_label(label);
		pop_stack();

		add_row_separator(editor_theme::get().col_accent);

		add_row_cell(0.0f);
		const id_pair x = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 0);
		pop_stack();

		add_row_cell(0.0f);
		const id_pair y = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 1);
		pop_stack();

		add_row_cell(0.0f);
		const id_pair z = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 2);
		pop_stack();

		add_row_cell(0.0f);
		const id_pair w = add_text_field(text, buffer_capacity, gui_text_field_type::number, decimals, increment, 0.0f, 0.0f, 0.0f, 0, 3);
		pop_stack();

		pop_stack();
		return {row, x.first, y.first, z.first, w.first};
	}

	// -----------------------------------------------------------------------------
	// property - utils
	// -----------------------------------------------------------------------------

	id gui_builder::add_row_separator(const vector4& color)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_y_relative;
			pp.pos.y	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_relative;
			sz.size.x	   = editor_theme::get().seperator_thickness;
			sz.size.y	   = 1.0f;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= color;
			gfx.draw_order	= _draw_order;
		}

		return w;
	}

	id gui_builder::add_col_separator(const vector4& color)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_y_abs | size_flags::sf_x_relative;
			sz.size.x	   = 1.0f;
			sz.size.y	   = editor_theme::get().seperator_thickness;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= color;
			gfx.draw_order	= _draw_order;
		}

		return w;
	}

	id gui_builder::add_property_row()
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_child_pos_row;
			pp.pos.x	  = 0.0f;

			size_props& sz		   = _builder->widget_get_size_props(w);
			sz.flags			   = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x			   = 1.0f;
			sz.size.y			   = editor_theme::get().row_height;
			sz.spacing			   = editor_theme::get().row_spacing;
			sz.child_margins.left  = _indent;
			sz.child_margins.right = editor_theme::get().outer_margin * 2;

			// widget_gfx& gfx = _builder->widget_get_gfx(w);
			// gfx.flags		= gfx_flags::gfx_is_rect;
			// gfx.color		= editor_theme::get().col_accent;
			// gfx.draw_order	= _draw_order;
		};
		return w;
	}

	id gui_builder::add_row_cell(f32 size)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_child_pos_row;
			pp.pos.y	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_y_relative;
			sz.size.y	   = 1.0f;

			if (vekt::math::equals(size, 0.0f))
				sz.flags |= size_flags::sf_x_fill;
			else
			{
				sz.flags |= size_flags::sf_x_relative;
				sz.size.x = size;
			}

			// widget_gfx& gfx = _builder->widget_get_gfx(w);
			// gfx.flags		= gfx_flags::gfx_clip_children;
			// gfx.color		= editor_theme::get().col_accent;
			// gfx.draw_order	= _draw_order;
		}
		return w;
	}

	// -----------------------------------------------------------------------------
	// single items
	// -----------------------------------------------------------------------------

	vekt::id gui_builder::add_sub_title(const char* title)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = editor_theme::get().item_spacing * 0.75f;
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_total_children;
			sz.size.x	   = 1.0f;
		}

		const id line = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(line);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_second_color;
			gfx.color		= editor_theme::get().col_title_line_end;
			gfx.draw_order	= _draw_order;

			second_color_props& sc = _builder->widget_get_second_color(line);
			sc.color			   = editor_theme::get().col_title_line_start;

			pos_props& pp = _builder->widget_get_pos_props(line);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(line);
			sz.size		   = VEKT_VEC2(editor_theme::get().title_line_width, editor_theme::get().title_line_height);
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_abs;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_title;
			gfx.draw_order	= _draw_order;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_default;
			_builder->widget_set_text(txt, title);
		}

		_builder->widget_add_child(w, txt);
		_builder->widget_add_child(w, line);
		return w;
	}

	gui_builder::id_trip gui_builder::add_component_title(const char* title)
	{
		const id w0 = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w0);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w0);
			sz.flags		 = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x		 = 1.0f;
			sz.size.y		 = editor_theme::get().row_height;
			sz.child_margins = {0.0f, 0.0f, editor_theme::get().outer_margin, editor_theme::get().outer_margin * 2};
		}

		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative;
			pp.pos.x	  = 0.0f;
			pp.pos.y	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_relative | size_flags::sf_y_relative;
			sz.size.x		 = 1.0f;
			sz.size.y		 = 1.0f;
			sz.child_margins = {0.0f, 0.0f, editor_theme::get().outer_margin, 0.0f};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= editor_theme::get().col_button;
			gfx.draw_order	= _draw_order;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = editor_theme::get().frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_button_press;
			icp.hovered_color	   = editor_theme::get().col_button_hover;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = callbacks.on_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = callbacks.user_data;
		}

		const id txt = new_widget();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_text;
			gfx.draw_order	= _draw_order;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.0f;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_default;
			_builder->widget_set_text(txt, title);
		}

		const id actions = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(actions);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_end | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center | pos_flags::pf_child_pos_row;
			pp.pos.x	  = 1.0f;
			pp.pos.y	  = 0.5f;

			size_props& sz = _builder->widget_get_size_props(actions);
			sz.flags	   = size_flags::sf_x_total_children | size_flags::sf_y_relative;
			sz.size.y	   = 1.0f;
			sz.spacing	   = editor_theme::get().row_spacing;
		}

		const id reset_button = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(reset_button);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center | pos_flags::pf_child_pos_row;
			pp.pos.y	  = 0.5f;

			size_props& sz = _builder->widget_get_size_props(reset_button);
			sz.flags	   = size_flags::sf_x_copy_y | size_flags::sf_y_relative;
			sz.size.y	   = 1.0f;

			widget_gfx& gfx = _builder->widget_get_gfx(reset_button);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= vector4();
			gfx.draw_order	= _draw_order + 1;

			input_color_props& icp = _builder->widget_get_input_colors(reset_button);
			icp.pressed_color	   = editor_theme::get().col_frame_bg;
			icp.hovered_color	   = editor_theme::get().col_light;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(reset_button);
			mc.on_mouse		   = callbacks.on_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(reset_button);
			ud.ptr				 = callbacks.user_data;
		}

		const id reset_icon = new_widget();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(reset_icon);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_accent_second;
			gfx.draw_order	= _draw_order + 1;

			pos_props& pp = _builder->widget_get_pos_props(reset_icon);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(reset_icon);
			tp.font		   = editor_theme::get().font_icons;
			tp.scale	   = 0.75f;
			_builder->widget_set_text(reset_icon, ICON_ROTATE);
		}

		pop_stack(); // pop reset_button

		const id remove_button = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(remove_button);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center | pos_flags::pf_child_pos_row;
			pp.pos.y	  = 0.5f;

			size_props& sz = _builder->widget_get_size_props(remove_button);
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.flags	   = size_flags::sf_x_copy_y | size_flags::sf_y_relative;
			sz.size.y	   = 1.0f;

			widget_gfx& gfx = _builder->widget_get_gfx(remove_button);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= vector4();
			gfx.draw_order	= _draw_order + 1;

			input_color_props& icp = _builder->widget_get_input_colors(remove_button);
			icp.pressed_color	   = editor_theme::get().col_frame_bg;
			icp.hovered_color	   = editor_theme::get().col_light;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(remove_button);
			mc.on_mouse		   = callbacks.on_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(remove_button);
			ud.ptr				 = callbacks.user_data;
		}

		const id remove_icon = new_widget();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(remove_icon);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_accent_second;
			gfx.draw_order	= _draw_order + 1;

			pos_props& pp = _builder->widget_get_pos_props(remove_icon);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(remove_icon);
			tp.font		   = editor_theme::get().font_icons;
			tp.scale	   = 0.75f;
			_builder->widget_set_text(remove_icon, ICON_CROSS);
		}

		pop_stack(); // remove button
		pop_stack(); // actions;
		pop_stack(); // inner;
		pop_stack(); // outer;

		return {w, reset_button, remove_button};
	}

	id gui_builder::add_title(const char* title)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = editor_theme::get().item_spacing * 0.75f;
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_total_children;
			sz.size.x	   = 1.0f;
		}

		const id line = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(line);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_second_color;
			gfx.color		= editor_theme::get().col_title_line_start;
			gfx.draw_order	= _draw_order;

			second_color_props& sc = _builder->widget_get_second_color(line);
			sc.color			   = editor_theme::get().col_title_line_end;

			pos_props& pp = _builder->widget_get_pos_props(line);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_end;
			pp.pos.x	  = 1.0f;

			size_props& sz = _builder->widget_get_size_props(line);
			sz.size		   = VEKT_VEC2(editor_theme::get().title_line_width, editor_theme::get().title_line_height);
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_abs;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_title;
			gfx.draw_order	= _draw_order;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_end;
			pp.pos.x	  = 1.0f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_title;
			_builder->widget_set_text(txt, title);
		}

		_builder->widget_add_child(w, txt);
		_builder->widget_add_child(w, line);
		return w;
	}

	id gui_builder::add_label(const char* label, size_t buffer_capacity)
	{
		const id w = new_widget();

		widget_gfx& gfx = _builder->widget_get_gfx(w);
		gfx.flags		= gfx_flags::gfx_is_text;
		gfx.color		= editor_theme::get().col_text;
		gfx.draw_order	= _draw_order;

		pos_props& pp = _builder->widget_get_pos_props(w);
		pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
		pp.pos.x	  = 0.0f;
		pp.pos.y	  = 0.5f;

		text_props& tp = _builder->widget_get_text(w);
		tp.font		   = editor_theme::get().font_default;
		_builder->widget_set_text(w, label, buffer_capacity);

		return w;
	}

	id gui_builder::add_hyperlink(const char* label, size_t buffer_capacity)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.0f;
			pp.pos.y	  = 0.5f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = 1.0f;
			sz.flags	   = size_flags::sf_x_max_children | size_flags::sf_y_total_children;
			sz.size.x	   = 1.0f;

			_builder->widget_get_hover_callbacks(w).on_hover_begin = on_hover_begin_hand_c;
			_builder->widget_get_hover_callbacks(w).on_hover_end   = on_hover_end_hand_c;
			_builder->widget_get_mouse_callbacks(w).on_mouse	   = callbacks.on_mouse;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_hyperlink;
			gfx.draw_order	= _draw_order;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_default;
			_builder->widget_set_text(txt, label, buffer_capacity);
		}

		const id line = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(line);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= editor_theme::get().col_hyperlink;
			gfx.draw_order	= _draw_order;

			pos_props& pp = _builder->widget_get_pos_props(line);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(line);
			sz.size		   = VEKT_VEC2(.0f, 1.0f);
			sz.flags	   = size_flags::sf_x_fill | size_flags::sf_y_abs;
		}

		_builder->widget_add_child(w, txt);
		_builder->widget_add_child(w, line);

		return w;
	}

	gui_builder::id_pair gui_builder::add_button(const char* title, size_t buffer_capacity)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_max_children | size_flags::sf_y_abs;
			sz.size.y		 = editor_theme::get().item_height;
			sz.child_margins = {editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= editor_theme::get().col_button;
			gfx.draw_order	= _draw_order;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = editor_theme::get().frame_thickness;
			st.color		 = editor_theme::get().col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = editor_theme::get().frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_button_press;
			icp.hovered_color	   = editor_theme::get().col_button_hover;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = callbacks.on_mouse;
		}

		const id txt = add_label(title, buffer_capacity);
		{
			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;

			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.draw_order	= _draw_order + 1;
		}

		pop_stack();
		return {w, txt};
	}

	gui_builder::id_pair gui_builder::add_icon_button(const char* icon, size_t buffer_capacity, f32 size, bool is_row, vector4 color)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);

			if (is_row)
			{
				pp.flags = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
				pp.pos.y = 0.5f;
			}
			else
			{
				pp.flags = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_center;
				pp.pos.x = 0.5f;
			}

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.size.x = sz.size.y = editor_theme::get().item_height * 1.5f;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= vector4();
			gfx.draw_order	= _draw_order;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_frame_bg;
			icp.hovered_color	   = editor_theme::get().col_light;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = callbacks.on_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = callbacks.user_data;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= math::almost_equal(color.w, 0.0f) ? editor_theme::get().col_text : color;
			gfx.draw_order	= _draw_order + 1;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_icons;
			tp.scale	   = size;
			_builder->widget_set_text(txt, icon, buffer_capacity);
		}

		_builder->widget_add_child(w, txt);

		pop_stack();
		return {w, txt};
	}

	gui_builder::id_pair gui_builder::add_toggle_button(const char* icon, bool toggled_on, size_t buffer_capacity, f32 size, bool is_row, vector4 color)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);

			if (is_row)
			{
				pp.flags = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
				pp.pos.y = 0.5f;
			}
			else
			{
				pp.flags = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_center;
				pp.pos.x = 0.5f;
			}

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.size.x = sz.size.y = editor_theme::get().item_height * 1.5f;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= toggled_on ? editor_theme::get().col_accent_second : vector4();
			gfx.draw_order	= _draw_order;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = on_toggle_button_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_frame_bg;
			icp.hovered_color	   = editor_theme::get().col_light;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= math::almost_equal(color.w, 0.0f) ? editor_theme::get().col_text : color;
			gfx.draw_order	= _draw_order + 1;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_icons;
			tp.scale	   = size;
			_builder->widget_set_text(txt, icon, buffer_capacity);
		}

		_builder->widget_add_child(w, txt);

		_toggle_buttons.push_back({
			.widget	 = w,
			.toggled = toggled_on,
		});

		pop_stack();
		return {w, txt};
	}

	id gui_builder::set_fill_x(vekt::id id)
	{
		size_props& sz = _builder->widget_get_size_props(id);
		sz.flags &= ~size_flags::sf_x_abs;
		sz.flags &= ~size_flags::sf_x_relative;
		sz.flags &= ~size_flags::sf_x_copy_y;
		sz.flags &= ~size_flags::sf_x_max_children;
		sz.flags &= ~size_flags::sf_x_total_children;
		sz.flags |= size_flags::sf_x_fill;
		return id;
	}

	void gui_builder::set_text_field_text(gui_text_field& tf, const char* text)
	{
		const size_t sz = strlen(text);
		SFG_ASSERT(sz < tf.buffer_capacity);
		char* c = (char*)tf.buffer;

		if (sz == 0)
		{
			c[0]			 = '\0';
			tf.buffer_size	 = 0;
			tf.caret_end_pos = tf.caret_pos = 0;
			return;
		}

		SFG_MEMCPY(c, (char*)text, sz);
		tf.buffer_size	  = sz;
		c[tf.buffer_size] = '\0';
		tf.caret_pos	  = math::clamp(tf.caret_pos, (u32)0, tf.buffer_size);
		tf.caret_end_pos  = tf.caret_pos;
		_builder->widget_update_text(tf.text_widget);
	}

	void gui_builder::set_text_field_text(vekt::id id, const char* text, bool skip_if_focused)
	{
		auto it = vector_util::find_if(_text_fields, [id](const gui_text_field& tf) -> bool { return tf.widget == id; });
		SFG_ASSERT(it != _text_fields.end());

		if (skip_if_focused && _builder->widget_get_hover_callbacks(it->widget).is_focused)
			return;

		set_text_field_text(*it, text);
	}

	void gui_builder::set_text_field_value(vekt::id id, f32 f, bool skip_if_focused, bool is_int)
	{
		auto it = vector_util::find_if(_text_fields, [id](const gui_text_field& tf) -> bool { return tf.widget == id; });
		SFG_ASSERT(it != _text_fields.end());

		if (skip_if_focused && _builder->widget_get_hover_callbacks(it->widget).is_focused)
			return;

		it->value = f;

		char* cur = (char*)it->buffer;
		if (is_int)
		{
			const i32 i = static_cast<i32>(f);
			char_util::append_i32(cur, cur + it->buffer_capacity, i);
		}
		else
		{
			char_util::append_double(cur, cur + it->buffer_capacity, f, 3.0f);
		}
		size_t diff		= cur - it->buffer;
		it->buffer_size = static_cast<unsigned int>(diff);

		if (it->is_slider)
		{
			it->value	   = math::clamp(it->value, it->min, it->max);
			size_props& sz = _builder->widget_get_size_props(it->sliding_widget);
			sz.size.x	   = math::remap(it->value, it->min, it->max, 0.0f, 1.0f);
		}

		_builder->widget_update_text(it->text_widget);
	}

	void gui_builder::text_field_edit_complete(gui_text_field& tf)
	{
		if (tf.type == gui_text_field_type::number)
		{
			set_text_field_value(tf.widget, tf.value, false, tf.decimals == 0);
		}
	}

	void gui_builder::set_checkbox_value(vekt::id id, unsigned char value)
	{
		auto it = std::find_if(_checkboxes.begin(), _checkboxes.end(), [id](const gui_checkbox& cb) -> bool { return cb.widget == id; });
		if (it != _checkboxes.end())
		{
			it->state = value;
			_builder->widget_set_visible(it->text_widget, value);
		}
	}

	void gui_builder::set_widget_enabled(vekt::id w, bool enabled, const vector4& enabled_col, const vector4& disabled_col)
	{
		widget_gfx& gfx = _builder->widget_get_gfx(w);
		gfx.color		= enabled ? enabled_col : disabled_col;

		const widget_meta& m = _builder->widget_get_meta(w);
		if (m.children.empty())
			return;

		const id txt = m.children[0];
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			if (gfx.flags & gfx_flags::gfx_is_text)
				gfx.color = enabled ? editor_theme::get().col_text : editor_theme::get().col_text_dim;
		}
	}

	gui_builder::id_pair gui_builder::add_text_field(const char* text, size_t buffer_capacity, gui_text_field_type type, unsigned int decimals, f32 increment, f32 min, f32 max, f32 val, unsigned char is_slider, unsigned int sub_index)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;
			pp.pos.x	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x		 = 1.0f;
			sz.size.y		 = editor_theme::get().item_height;
			sz.child_margins = {0.0f, 0.0f, is_slider ? 0.0f : editor_theme::get().inner_margin, is_slider ? 0.0f : editor_theme::get().inner_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_custom_pass | gfx_flags::gfx_focusable;
			gfx.color		= editor_theme::get().col_frame_bg;
			gfx.draw_order	= _draw_order;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = editor_theme::get().frame_thickness;
			st.color		 = editor_theme::get().col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = editor_theme::get().frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.hovered_color	   = editor_theme::get().col_area_bg;
			icp.focus_color		   = editor_theme::get().col_accent;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = on_text_field_mouse;
			mc.on_drag		   = on_text_field_drag;

			key_callback& kc = _builder->widget_get_key_callbacks(w);
			kc.on_key		 = on_text_field_key;

			hover_callback& hb = _builder->widget_get_hover_callbacks(w);
			// hb.receive_mouse   = 1;
			hb.on_hover_begin  = on_hover_begin_text_field;
			hb.on_hover_end	   = on_hover_end_text_field;
			hb.on_focus_lost   = on_text_field_focus_lost;
			hb.on_focus_gained = on_text_field_focus_gained;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;

			custom_passes& cp	= _builder->widget_get_custom_pass(w);
			cp.custom_draw_pass = on_text_field_draw;
		}

		const id txt = add_label(text, buffer_capacity);
		{
			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = is_slider ? 0.5f : 0.0f;
			pp.pos.y	  = 0.5f;

			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			if (is_slider)
			{
				pp.flags |= pos_flags::pf_x_anchor_center;
				gfx.draw_order = _draw_order + 2;
			}
			else
				gfx.draw_order = _draw_order + 1;
		}

		text_props& tp = _builder->widget_get_text(txt);

		gui_text_field tf = {
			.buffer			 = tp.text,
			.widget			 = w,
			.text_widget	 = txt,
			.buffer_size	 = text == nullptr ? 0 : static_cast<u32>(strlen(text)),
			.buffer_capacity = static_cast<unsigned int>(tp.text_capacity),
			.decimals		 = decimals,
			.sub_index		 = sub_index,
			.value_increment = increment,
			.min			 = min,
			.max			 = max,
			.type			 = type,
			.is_slider		 = is_slider,
		};

		if (is_slider)
		{
			const id w2 = new_widget();
			{
				pos_props& pp = _builder->widget_get_pos_props(w2);
				pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative;
				pp.pos.y	  = 0.0f;
				pp.pos.x	  = 0.0f;

				size_props& sz = _builder->widget_get_size_props(w2);
				sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_relative;
				sz.size.x	   = math::clamp(math::remap(val, min, max, 0.0f, 1.0f), 0.0f, 1.0f);
				sz.size.y	   = 1.0f;

				widget_gfx& gfx = _builder->widget_get_gfx(w2);
				gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
				gfx.color		= editor_theme::get().col_accent_dim;
				gfx.draw_order	= _draw_order + 1;

				rounding_props& rp = _builder->widget_get_rounding(w2);
				rp.rounding		   = editor_theme::get().frame_rounding;
				rp.segments		   = 8;
			}

			tf.sliding_widget = w2;
		}

		_text_fields.push_back(tf);

		pop_stack();
		return {w, txt};
	}

	id gui_builder::new_widget(bool push_to_stack)
	{
		const id w = _builder->allocate();

		const id parent = _stack_ptr == 0 ? _root : stack();

		_builder->widget_add_child(parent, w);

		if (push_to_stack)
			push_stack(w);

		_builder->widget_get_user_data(w).ptr = callbacks.user_data;
		return w;
	}

	void gui_builder::push_stack(id s)
	{
		_stack[_stack_ptr] = s;
		_stack_ptr++;
		ASSERT(_stack_ptr < STACK_SIZE);
	}

	id gui_builder::pop_stack()
	{
		ASSERT(_stack_ptr > 0);
		const id last = _stack[_stack_ptr - 1];
		_stack_ptr--;
		return last;
	}

	id gui_builder::stack()
	{
		ASSERT(_stack_ptr > 0);
		return _stack[_stack_ptr - 1];
	}

	vekt::id gui_builder::add_checkbox(bool initial_state)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;
			pp.pos.x	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.size.x		 = editor_theme::get().item_height;
			sz.size.y		 = editor_theme::get().item_height;
			sz.child_margins = {editor_theme::get().inner_margin * 0.25f, editor_theme::get().inner_margin * 0.25f, editor_theme::get().inner_margin * 0.25f, editor_theme::get().inner_margin * 0.25f};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= editor_theme::get().col_frame_bg;
			gfx.draw_order	= _draw_order;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = editor_theme::get().frame_thickness;
			st.color		 = editor_theme::get().col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = editor_theme::get().frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_button_press;
			icp.hovered_color	   = editor_theme::get().col_button_hover;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = on_checkbox_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;
		}

		const id txt = _builder->allocate();
		{
			_builder->widget_add_child(w, txt);

			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= editor_theme::get().col_accent_second;
			gfx.draw_order	= _draw_order + 1;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = editor_theme::get().font_icons;
			tp.scale	   = 0.7f;
			_builder->widget_set_text(txt, ICON_CHECK, 4);
			_builder->widget_set_visible(txt, initial_state);
		}

		gui_checkbox cb = {.widget = w, .text_widget = txt, .state = static_cast<unsigned char>(initial_state ? 1 : 0)};
		_checkboxes.push_back(cb);

		pop_stack();
		return w;
	}

	vekt::id gui_builder::add_resource(const char* res, const char* extension, string_id type_id, size_t buffer_capacity)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;
			pp.pos.x	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x		 = 1.0f;
			sz.size.y		 = editor_theme::get().item_height;
			sz.child_margins = {0.0f, 0.0f, editor_theme::get().inner_margin, editor_theme::get().inner_margin * 0.25f};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_has_hover_color | gfx_flags::gfx_has_press_color | gfx_clip_children;
			gfx.color		= editor_theme::get().col_frame_bg;
			gfx.draw_order	= _draw_order;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = editor_theme::get().frame_thickness;
			st.color		 = editor_theme::get().col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = editor_theme::get().frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_button_press;
			icp.hovered_color	   = editor_theme::get().col_button_hover;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = on_resource_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;
		}

		const id txt = add_label(res, buffer_capacity);
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.draw_order	= _draw_order + 1;
		}

		pop_stack();

		const gui_resource r = {
			.extension	 = editor::get().get_text_allocator().allocate(extension),
			.type		 = type_id,
			.widget		 = w,
			.text_widget = txt,
		};
		_resources.push_back(r);

		return w;
	}

	vekt::id gui_builder::add_dropdown(const char* initial_text, size_t buffer_capacity)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;
			pp.pos.x	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x		 = 1.0f;
			sz.size.y		 = editor_theme::get().item_height;
			sz.child_margins = {0.0f, 0.0f, editor_theme::get().inner_margin, editor_theme::get().inner_margin * 0.25f};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_has_hover_color | gfx_flags::gfx_has_press_color | gfx_clip_children;
			gfx.color		= editor_theme::get().col_frame_bg;
			gfx.draw_order	= _draw_order;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = editor_theme::get().frame_thickness;
			st.color		 = editor_theme::get().col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = editor_theme::get().frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = editor_theme::get().col_button_press;
			icp.hovered_color	   = editor_theme::get().col_button_hover;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = on_dropdown_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;
		}

		const id txt = add_label(initial_text, buffer_capacity);
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.draw_order	= _draw_order + 1;
		}

		pop_stack();

		gui_dropdown d = {};
		d.widget	   = w;
		d.text_widget  = txt;
		_dropdowns.push_back(d);

		return w;
	}

	void gui_builder::add_dropdown_item(vekt::id dropdown_widget, const char* label, bool is_selected)
	{
		auto it = std::find_if(_dropdowns.begin(), _dropdowns.end(), [dropdown_widget](const gui_dropdown& d) { return d.widget == dropdown_widget; });
		SFG_ASSERT(it != _dropdowns.end());

		const char* stored = editor::get().get_text_allocator().allocate(label);
		if (is_selected)
			it->selected = static_cast<u8>(it->items.size());

		it->items.push_back(stored);

		// If no text set, set to first item
		if (it->items.size() == 1)
		{
			// _builder->widget_set_text(it->text_widget, stored);
		}
	}

	vekt::input_event_result gui_builder::on_dropdown_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type != vekt::input_event_type::pressed || ev.button != static_cast<u16>(input_code::mouse_0))
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = std::find_if(gb->_dropdowns.begin(), gb->_dropdowns.end(), [widget](const gui_dropdown& d) { return d.widget == widget; });
		SFG_ASSERT(it != gb->_dropdowns.end());

		gb->_dropdown_ctx_bindings.resize(0);

		editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);
		const u8 sz = static_cast<u8>(it->items.size());

		for (u8 i = 0; i < sz; ++i)
		{
			const char*	   item_label = it->items[i];
			const vekt::id btn		  = editor::get().get_gui_controller().add_context_menu_item_toggle(item_label, i == it->selected);
			gb->_dropdown_ctx_bindings.push_back({.item_widget = btn, .dropdown_widget = widget, .index = i});
		}
		editor::get().get_gui_controller().end_context_menu();

		return vekt::input_event_result::handled;
	}

	void gui_builder::on_context_item(vekt::id widget)
	{
		auto it = std::find_if(_dropdown_ctx_bindings.begin(), _dropdown_ctx_bindings.end(), [widget](const dropdown_ctx_binding& d) { return d.item_widget == widget; });
		if (it == _dropdown_ctx_bindings.end())
			return;

		// Update visible text
		auto it_dd = std::find_if(_dropdowns.begin(), _dropdowns.end(), [it](const gui_dropdown& d) { return d.widget == it->dropdown_widget; });
		if (it_dd != _dropdowns.end())
		{
			const unsigned int idx = it->index;

			it_dd->selected = idx;
			if (idx < it_dd->items.size())
				_builder->widget_set_text(it_dd->text_widget, it_dd->items[idx]);
		}

		invoke_reflection(it->dropdown_widget, &it->index, 0);
	}

};
