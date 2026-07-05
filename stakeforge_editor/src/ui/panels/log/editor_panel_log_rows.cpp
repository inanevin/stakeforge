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
#include "ui/panels/log/editor_panel_log.hpp"
#include "ui/panels/log/editor_panel_log_internal.hpp"
#include "ui/editor_text_rasterization.hpp"

#include <sfg/data/frame_string.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_log_t::add_log_row(log_level level, const char* text)
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const u64 hash = hashing_t::hash_u64(text);
		if (_is_collapsed)
		{
			for (size_t i = 0; i < _rows.size(); ++i)
			{
				log_row_t& row = _rows[i];
				if (row.hash != hash)
					continue;

				row.count++;
				update_log_row_text(row);
				move_log_row_to_bottom(i);
				log_row_t& moved = _rows.back();
				set_log_row_visible(moved, _is_visible && is_log_row_visible(moved));
				return;
			}
		}

		log_row_t row = {};
		row.raw_text  = text;
		row.hash	  = hash;
		row.flag	  = log_level_to_filter_flag(level);

		row.root = ui.allocate_widget();
		ui.set_widget_debug_name(row.root, "log_row");
		tree.attach(_body, row.root);

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.flags			= ui::wf_visible;
		row_in.size_mode_x		= ui::axis_mode_e::sum_children;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value.y		= theme.item_height;
		row_in.flow				= ui::flow_e::row;
		row_in.child_spacing	= theme.item_spacing;

		row.icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon, "log_row_icon");
		tree.attach(row.root, row.icon);

		ui::layout_in_t& icon_in = tree.in(row.icon);
		icon_in.flags			 = ui::wf_visible;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		icon_in.size_value		 = {theme.item_height, 1.0f};

		ui.set_widget_text(row.icon, log_level_to_icon(level));
		paint.set_text(row.icon,
					   ui.widget_text(row.icon),
					   ui.widget_text_len(row.icon),
					   {.font = theme.font_icons, .color = log_level_to_color(level, theme), .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		row.text = ui.allocate_widget();
		ui.set_widget_debug_name(row.text, "log_row_text");
		tree.attach(row.root, row.text);

		ui::layout_in_t& text_in = tree.in(row.text);
		text_in.flags			 = ui::wf_visible;

		paint.set_text(row.text, nullptr, 0, {.font = theme.font_default_mono, .color = log_level_to_color(level, theme), .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_rows.push_back(std::move(row));
		log_row_t& stored = _rows.back();
		update_log_row_text(stored);
		set_log_row_visible(stored, _is_visible && is_log_row_visible(stored));
		trim_log_rows();
	}

	void editor_panel_log_t::update_log_row_text(log_row_t& row)
	{
		frame_string_t<char> display_text;
		const string_t		 tag = file_system_t::get_system_time_tag_str(row.count);
		display_text.assign(tag.c_str(), tag.size());
		display_text += ' ';
		display_text.append(row.raw_text.c_str(), row.raw_text.size());
		_ui->set_widget_text(row.text, display_text.c_str());
	}

	void editor_panel_log_t::move_log_row_to_bottom(size_t index)
	{
		SFG_ASSERT(index < _rows.size());

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.detach(_rows[index].root);
		tree.attach(_body, _rows[index].root);

		if (index + 1 == _rows.size())
			return;

		log_row_t moved = std::move(_rows[index]);
		_rows.erase(_rows.begin() + index);
		_rows.push_back(std::move(moved));
	}

	void editor_panel_log_t::collapse_existing_rows()
	{
		if (!can_mutate_ui_topology())
		{
			request_collapse_rows();
			return;
		}

		const size_t row_count = _rows.size();
		if (row_count < 2)
			return;

		frame_vector_t<bool> remove(row_count, false);
		for (size_t i = 0; i < row_count; ++i)
		{
			if (remove[i])
				continue;
			log_row_t& base = _rows[i];
			for (size_t j = i + 1; j < row_count; ++j)
			{
				if (remove[j] || _rows[j].hash != base.hash)
					continue;
				base.count += _rows[j].count;
				_ui->deallocate_widget(_rows[j].root);
				remove[j] = true;
			}
		}

		size_t write = 0;
		for (size_t read = 0; read < row_count; ++read)
		{
			if (remove[read])
				continue;
			if (write != read)
				_rows[write] = std::move(_rows[read]);
			++write;
		}
		_rows.resize(write);

		for (log_row_t& row : _rows)
		{
			update_log_row_text(row);
			set_log_row_visible(row, _is_visible && is_log_row_visible(row));
		}
	}

	void editor_panel_log_t::clear_log_rows()
	{
		for (const log_row_t& row : _rows)
			_ui->deallocate_widget(row.root);
		_rows.clear();
	}

	void editor_panel_log_t::clear_logs()
	{
		if (!can_mutate_ui_topology())
		{
			request_clear_logs();
			return;
		}

		{
			LOCK_GUARD(_log_storage_mtx);
			_stored_logs.clear();
			_pending_logs.clear();
			_log_storage_generation++;
			_storage_generation = _log_storage_generation;
			_next_log_sequence	= _next_stored_log_sequence;
		}
		_drained_logs.clear();
		clear_log_rows();
	}

	void editor_panel_log_t::refresh_log_filter_visibility()
	{
		for (const log_row_t& row : _rows)
			set_log_row_visible(row, _is_visible && is_log_row_visible(row));
	}

	void editor_panel_log_t::set_log_row_visible(const log_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree	 = _ui->get_tree();
		const u16		   flags = visible ? ui::wf_visible : 0;
		tree.in(row.root).flags	 = flags;
		tree.in(row.icon).flags	 = flags;
		tree.in(row.text).flags	 = flags;
	}

	void editor_panel_log_t::trim_log_rows()
	{
		while (_rows.size() > EDITOR_LOG_PANEL_ROW_CAPACITY)
		{
			_ui->deallocate_widget(_rows.front().root);
			_rows.erase(_rows.begin());
		}
	}

}
