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
#include "ui/panels/editor_panel_log.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_LOG_PANEL_ROW_CAPACITY 192

	namespace
	{
		u32 g_log_panel_listener_id = 1;

		const editor_dropdown_item_t LOG_SOURCE_ITEMS[] = {
			{.text = "All", .value = static_cast<u16>(log_source_type_e::all)},
			{.text = "Editor", .value = static_cast<u16>(log_source_type_e::editor)},
			{.text = "Game", .value = static_cast<u16>(log_source_type_e::game)},
		};

		const char* log_source_type_to_string(log_source_type_e type)
		{
			switch (type)
			{
			case log_source_type_e::all:
				return "all";
			case log_source_type_e::editor:
				return "editor";
			case log_source_type_e::game:
				return "game";
			}
			return "all";
		}

		log_source_type_e log_source_type_from_string(const char* value)
		{
			const sid_t id = TO_SID(value);
			if (id == TO_SID("editor"))
				return log_source_type_e::editor;
			if (id == TO_SID("game"))
				return log_source_type_e::game;
			return log_source_type_e::all;
		}

		u8 log_level_to_filter_flag(log_level level)
		{
			switch (level)
			{
			case log_level::error:
				return log_level_filter_err;
			case log_level::warning:
				return log_level_filter_warn;
			case log_level::trace:
				return log_level_filter_trace;
			case log_level::info:
			case log_level::progress:
				return log_level_filter_info;
			}
			return log_level_filter_info;
		}

		const char* log_level_to_icon(log_level level)
		{
			switch (level)
			{
			case log_level::error:
				return ICON_ERROR;
			case log_level::warning:
				return ICON_WARN;
			case log_level::trace:
				return ICON_TRACE;
			case log_level::info:
			case log_level::progress:
				return ICON_INFO;
			}
			return ICON_INFO;
		}

		vec4f_t log_level_to_color(log_level level, const editor_theme_t& theme)
		{
			switch (level)
			{
			case log_level::error:
				return theme.color_accent_err;
			case log_level::warning:
				return theme.color_accent_warn;
			case log_level::trace:
				return theme.color_accent1;
			case log_level::info:
			case log_level::progress:
				return theme.color_text0;
			}
			return theme.color_text0;
		}

		bool text_matches_search(const char* text, const string_t& search)
		{
			if (search.empty())
				return true;
			if (text == nullptr)
				return false;

			string_t haystack = text;
			string_t needle	  = search;
			string_util::to_lower(haystack);
			string_util::to_lower(needle);
			return haystack.find(needle) != string_t::npos;
		}

		string_t make_log_display_text(const string_t& raw_text, u32 count)
		{
			string_t text = file_system_t::get_system_time_tag_str(count);
			text += " ";
			text += raw_text;
			return text;
		}
	}

	editor_panel_log_t::editor_panel_log_t()
	{
		set_type(editor_panel_type_e::log);
		set_title(editor_panel_type_to_string(editor_panel_type_e::log));
	}

	void editor_panel_log_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_top_row, "log_top_row");
		tree.attach(_root, _top_row);

		ui::layout_in_t& title_in = tree.in(_top_row);
		title_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		title_in.size_mode_y	  = ui::axis_mode_e::fixed;
		title_in.size_value		  = {1.0f, theme.item_area_height};
		title_in.flow			  = ui::flow_e::row;
		title_in.child_spacing	  = theme.item_spacing;
		title_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		editor_dropdown_config_t source_dropdown_config = {};
		source_dropdown_config.items					= LOG_SOURCE_ITEMS;
		source_dropdown_config.item_count				= static_cast<u16>(sizeof(LOG_SOURCE_ITEMS) / sizeof(LOG_SOURCE_ITEMS[0]));
		source_dropdown_config.selected					= get_selected_source;
		source_dropdown_config.pressed					= on_source_pressed;
		source_dropdown_config.user_data				= this;
		source_dropdown_config.width					= editor_dropdown_width_e::fixed;
		source_dropdown_config.pos_y					= editor_dropdown_pos_y_e::center;
		source_dropdown_config.fixed_width				= theme.item_width;
		_source_dropdown.init(ui, _top_row, source_dropdown_config);

		editor_icon_button_config_t button_config = {};
		button_config.frame_color				  = {0.0f, 0.0f, 0.0f, 0.0f};
		button_config.hover_color				  = theme.color_panel_light;
		button_config.press_color				  = theme.color_frame;
		button_config.frame_toggled_color		  = theme.color_frame;
		button_config.size						  = theme.item_height;
		button_config.icon_size					  = theme.text_default_px_size;
		button_config.toggle_enabled			  = true;
		button_config.on_clicked				  = on_filter_pressed;

		_warn_filter_data		   = {.panel = this, .flag = log_level_filter_warn};
		button_config.icon		   = ICON_WARN;
		button_config.toggled_icon = ICON_WARN;
		button_config.icon_color   = theme.color_accent_warn;
		button_config.tooltip	   = "Warnings";
		button_config.toggled	   = is_filter_enabled(log_level_filter_warn);
		button_config.user_data	   = &_warn_filter_data;
		_warn_button.init(ui, _top_row, button_config);

		_err_filter_data		   = {.panel = this, .flag = log_level_filter_err};
		button_config.icon		   = ICON_ERROR;
		button_config.toggled_icon = ICON_ERROR;
		button_config.icon_color   = theme.color_accent_err;
		button_config.tooltip	   = "Errors";
		button_config.toggled	   = is_filter_enabled(log_level_filter_err);
		button_config.user_data	   = &_err_filter_data;
		_err_button.init(ui, _top_row, button_config);

		_info_filter_data		   = {.panel = this, .flag = log_level_filter_info};
		button_config.icon		   = ICON_INFO;
		button_config.toggled_icon = ICON_INFO;
		button_config.icon_color   = theme.color_text0;
		button_config.tooltip	   = "Info";
		button_config.toggled	   = is_filter_enabled(log_level_filter_info);
		button_config.user_data	   = &_info_filter_data;
		_info_button.init(ui, _top_row, button_config);

		_trace_filter_data		   = {.panel = this, .flag = log_level_filter_trace};
		button_config.icon		   = ICON_TRACE;
		button_config.toggled_icon = ICON_TRACE;
		button_config.icon_color   = theme.color_accent1;
		button_config.tooltip	   = "Trace";
		button_config.toggled	   = is_filter_enabled(log_level_filter_trace);
		button_config.user_data	   = &_trace_filter_data;
		_trace_button.init(ui, _top_row, button_config);

		button_config.icon		   = ICON_DD_DOWN;
		button_config.toggled_icon = ICON_DD_DOWN;
		button_config.icon_color   = theme.color_text0;
		button_config.tooltip	   = "Collapse";
		button_config.toggled	   = _is_collapsed;
		button_config.user_data	   = this;
		button_config.on_clicked   = on_collapse_pressed;
		_collapse_button.init(ui, _top_row, button_config);

		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.type						  = editor_input_field_type_e::text;
		search_config.on_text_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.flags |= ui::wf_overlay;
		search_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value	  = {1.0f, 0.5f};
		search_in.anchor_x	  = ui::anchor_e::end;
		search_in.anchor_y	  = ui::anchor_e::center;
		search_in.size_mode_x = ui::axis_mode_e::fixed;
		search_in.size_mode_y = ui::axis_mode_e::fixed;
		search_in.size_value  = {theme.item_width * 2.0f, theme.item_height};

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "log_body");
		tree.attach(_root, _body);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_clip_children | ui::wf_scroll_x | ui::wf_scroll_y;
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_in.size_value		 = {1.0f, 1.0f};
		body_in.flow			 = ui::flow_e::column;
		body_in.child_spacing	 = 0.0f;
		body_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t body_rect = {};
		body_rect.fill_color_a		  = theme.color_frame;
		body_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_body, body_rect);

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _body;
		scrollbar_config.axes					   = editor_scrollbar_axis_xy;
		_scrollbar.init(ui, scrollbar_config);

		_rows.reserve(EDITOR_LOG_PANEL_ROW_CAPACITY);
		_pending_logs.reserve(32);
		_drained_logs.reserve(32);
		_listener_id = g_log_panel_listener_id++;
		log_t::instance().add_listener(_listener_id, on_log, this);
		ui.set_pre_layout_tick(_body, on_log_tick, this);
	}

	void editor_panel_log_t::uninit()
	{
		log_t::instance().remove_listener(_listener_id);
		_scrollbar.uninit();
		_trace_button.uninit();
		_info_button.uninit();
		_err_button.uninit();
		_warn_button.uninit();
		_collapse_button.uninit();
		_search_input.uninit();
		_source_dropdown.uninit();
		editor_panel_t::uninit();

		_top_row = NULL_WIDGET;
		_body	 = NULL_WIDGET;
		_rows.clear();
		_pending_logs.clear();
		_drained_logs.clear();
		_search_text.clear();
		_listener_id = 0;
	}

	void editor_panel_log_t::serialize(nlohmann::json& j) const
	{
		j					  = nlohmann::json::object();
		j["source_type"]	  = log_source_type_to_string(_source_type);
		j["log_filter_flags"] = static_cast<u32>(_log_filter_flags);
		j["is_collapsed"]	  = _is_collapsed;
	}

	void editor_panel_log_t::deserialize(const nlohmann::json& j)
	{
		const string_t source_type = j.value<string_t>("source_type", "all");
		_source_type			   = log_source_type_from_string(source_type.c_str());
		_log_filter_flags		   = static_cast<u8>(j.value<u32>("log_filter_flags", static_cast<u32>(log_level_filter_all))) & log_level_filter_all;
		_is_collapsed			   = j.value("is_collapsed", false);
	}

	void editor_panel_log_t::make_visible(bool visible)
	{
		editor_panel_t::make_visible(visible);
		if (!visible)
			_source_dropdown.close();
	}

	u16 editor_panel_log_t::get_selected_source(void* user_data)
	{
		return static_cast<u16>(static_cast<editor_panel_log_t*>(user_data)->_source_type);
	}

	void editor_panel_log_t::on_source_pressed(u16 value, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->_source_type = static_cast<log_source_type_e>(value);
	}

	void editor_panel_log_t::on_filter_pressed(bool toggled, void* user_data)
	{
		const log_filter_button_data_t& data = *static_cast<log_filter_button_data_t*>(user_data);
		if (toggled)
			data.panel->_log_filter_flags |= data.flag;
		else
			data.panel->_log_filter_flags &= static_cast<u8>(~data.flag);
		data.panel->refresh_log_filter_visibility();
	}

	void editor_panel_log_t::on_collapse_pressed(bool toggled, void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		panel._is_collapsed		  = toggled;
		if (toggled)
			panel.collapse_existing_rows();
	}

	void editor_panel_log_t::on_search_changed(const char* value, void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		panel._search_text		  = value != nullptr ? value : "";
		panel.refresh_log_filter_visibility();
	}

	bool editor_panel_log_t::is_filter_enabled(u8 flag) const
	{
		return (_log_filter_flags & flag) != 0;
	}

	bool editor_panel_log_t::is_log_row_visible(const log_row_t& row) const
	{
		return is_filter_enabled(row.flag) && text_matches_search(row.raw_text.c_str(), _search_text);
	}

	void editor_panel_log_t::on_log(log_level level, const char* msg, void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		LOCK_GUARD(panel._pending_logs_mtx);
		panel._pending_logs.push_back({.text = msg, .level = level});
	}

	void editor_panel_log_t::on_log_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->drain_pending_logs();
	}

	void editor_panel_log_t::drain_pending_logs()
	{
		_drained_logs.clear();
		{
			LOCK_GUARD(_pending_logs_mtx);
			_pending_logs.swap(_drained_logs);
		}

		for (const log_record_t& record : _drained_logs)
			add_log_row(record.level, record.text.c_str());

		if (!_drained_logs.empty())
			_scrollbar.scroll_to_end_y();
	}

	void editor_panel_log_t::add_log_row(log_level level, const char* text)
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const u64 hash = hashing_t::hash_fnv_1a64(text);
		if (_is_collapsed)
		{
			log_row_t* existing = find_collapsed_row(hash);
			if (existing != nullptr)
			{
				existing->count++;
				update_log_row_text(*existing);
				move_log_row_to_bottom(*existing);
				set_log_row_visible(_rows.back(), is_log_row_visible(_rows.back()));
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
		tree.draw_order(row.root) = tree.draw_order_const(_body) + 1;

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
		tree.draw_order(row.icon) = tree.draw_order_const(row.root) + 1;

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
		tree.draw_order(row.text) = tree.draw_order_const(row.root) + 1;

		ui::layout_in_t& text_in = tree.in(row.text);
		text_in.flags			 = ui::wf_visible;

		const string_t display_text = make_log_display_text(row.raw_text, row.count);
		ui.set_widget_text(row.text, display_text.c_str());
		paint.set_text(row.text,
					   ui.widget_text(row.text),
					   ui.widget_text_len(row.text),
					   {.font = theme.font_default_mono, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_rows.push_back(row);
		set_log_row_visible(_rows.back(), is_log_row_visible(_rows.back()));
		trim_log_rows();
	}

	void editor_panel_log_t::update_log_row_text(log_row_t& row)
	{
		const string_t display_text = make_log_display_text(row.raw_text, row.count);
		_ui->set_widget_text(row.text, display_text.c_str());
	}

	editor_panel_log_t::log_row_t* editor_panel_log_t::find_collapsed_row(u64 hash)
	{
		for (log_row_t& row : _rows)
		{
			if (row.hash == hash)
				return &row;
		}
		return nullptr;
	}

	void editor_panel_log_t::move_log_row_to_bottom(log_row_t& row)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.detach(row.root);
		tree.attach(_body, row.root);

		for (auto it = _rows.begin(); it != _rows.end(); ++it)
		{
			if (&(*it) != &row)
				continue;

			log_row_t moved = std::move(*it);
			_rows.erase(it);
			_rows.push_back(std::move(moved));
			return;
		}
	}

	void editor_panel_log_t::collapse_existing_rows()
	{
		for (size_t i = 0; i < _rows.size(); ++i)
		{
			log_row_t& base = _rows[i];
			for (size_t j = i + 1; j < _rows.size();)
			{
				log_row_t& candidate = _rows[j];
				if (candidate.hash != base.hash)
				{
					++j;
					continue;
				}

				base.count += candidate.count;
				_ui->deallocate_widget(candidate.root);
				_rows.erase(_rows.begin() + j);
			}
			update_log_row_text(base);
			set_log_row_visible(base, is_log_row_visible(base));
		}
	}

	void editor_panel_log_t::refresh_log_filter_visibility()
	{
		for (const log_row_t& row : _rows)
			set_log_row_visible(row, is_log_row_visible(row));
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
