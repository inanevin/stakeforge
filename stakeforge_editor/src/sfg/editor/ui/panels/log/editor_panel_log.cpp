/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "editor_panel_log.hpp"
#include "editor_log_store.hpp"
#include "editor/ui/editor_text_rasterization.hpp"
#include "editor/ui/panels/editor_theme.hpp"
#include "editor/ui/widgets/editor_widgets_icons.hpp"
#include <sfg/core/data/frame_string.hpp>
#include <sfg/core/data/string_util.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/process.hpp>
#include <sfg/core/runtime/ui/ui_context.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_LOG_AUTO_SCROLL_SLOP	  1.0f
#define EDITOR_LOG_TIME_PREFIX_LENGTH 11

	namespace
	{
		const editor_dropdown_item_t LOG_SOURCE_ITEMS[] = {
			{
				.text  = "All",
				.value = static_cast<u16>(log_source_filter_e::all),
			},
			{
				.text  = "Engine",
				.value = static_cast<u16>(log_source_filter_e::engine),
			},
			{
				.text  = "Game",
				.value = static_cast<u16>(log_source_filter_e::game),
			},
		};

		const char* log_source_filter_to_string(log_source_filter_e filter)
		{
			switch (filter)
			{
			case log_source_filter_e::all:
				return "all";
			case log_source_filter_e::engine:
				return "engine";
			case log_source_filter_e::game:
				return "game";
			}

			return "all";
		}

		log_source_filter_e log_source_filter_from_string(const char* value)
		{
			const sid_t id = TO_SID(value);

			if (id == TO_SID("engine") || id == TO_SID("editor"))
				return log_source_filter_e::engine;

			if (id == TO_SID("game"))
				return log_source_filter_e::game;

			return log_source_filter_e::all;
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
	}

	editor_panel_log_t::editor_panel_log_t()
	{
		set_type(editor_panel_type_e::log);
		set_title(editor_panel_type_to_string(editor_panel_type_e::log));
		set_icon(ICON_PEN);
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

		ui::layout_in_t& top_in = tree.in(_top_row);
		top_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_in.size_mode_y		= ui::axis_mode_e::fixed;
		top_in.size_value		= {1.0f, theme.item_area_height};
		top_in.flow				= ui::flow_e::row;
		top_in.child_spacing	= theme.item_spacing;
		top_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		_source_dropdown.init(ui,
							  _top_row,
							  {
								  .items	   = LOG_SOURCE_ITEMS,
								  .selected	   = get_selected_source,
								  .pressed	   = on_source_pressed,
								  .user_data   = this,
								  .item_count  = static_cast<u16>(sizeof(LOG_SOURCE_ITEMS) / sizeof(LOG_SOURCE_ITEMS[0])),
								  .width	   = editor_dropdown_width_e::fixed,
								  .pos_y	   = editor_dropdown_pos_y_e::center,
								  .fixed_width = theme.item_width,
							  });

		editor_icon_button_config_t button_config = {};
		button_config.frame_color				  = vec4f_t::zero;
		button_config.outline_color				  = vec4f_t::zero;
		button_config.toggled_frame_color		  = theme.color_frame_light;
		button_config.toggled_outline_color		  = theme.color_outline_light;
		button_config.hover_color				  = theme.color_panel_light;
		button_config.toggled_hover_color		  = theme.color_panel_light;
		button_config.press_color				  = theme.color_frame_light;
		button_config.size						  = theme.item_height;
		button_config.icon_size					  = theme.text_big_px_size;
		button_config.rounding					  = theme.item_rounding;
		button_config.toggle_enabled			  = true;
		button_config.on_clicked				  = on_filter_pressed;

		const struct
		{
			const char* icon;
			vec4f_t		color;
			const char* tooltip;
			u8			flag;
		} filter_specs[FILTER_BUTTON_COUNT] = {
			{ICON_WARN, theme.color_accent_warn, "Warnings", log_level_filter_warn},
			{ICON_ERROR, theme.color_accent_err, "Errors", log_level_filter_err},
			{ICON_INFO, theme.color_text0, "Info", log_level_filter_info},
			{ICON_TRACE, theme.color_accent1, "Trace", log_level_filter_trace},
		};

		for (size_t i = 0; i < FILTER_BUTTON_COUNT; ++i)
		{
			_filter_button_data[i]	   = {.panel = this, .flag = filter_specs[i].flag};
			button_config.icon		   = filter_specs[i].icon;
			button_config.toggled_icon = filter_specs[i].icon;
			button_config.icon_color   = filter_specs[i].color;
			button_config.tooltip	   = filter_specs[i].tooltip;
			button_config.toggled	   = (_log_filter_flags & filter_specs[i].flag) != 0;
			button_config.user_data	   = &_filter_button_data[i];
			_filter_buttons[i].init(ui, _top_row, button_config);
		}

		button_config.icon			 = ICON_TRASH;
		button_config.toggled_icon	 = nullptr;
		button_config.icon_color	 = theme.color_accent_err;
		button_config.tooltip		 = "Clear";
		button_config.toggled		 = false;
		button_config.toggle_enabled = false;
		button_config.user_data		 = this;
		button_config.on_clicked	 = on_clear_pressed;
		_clear_button.init(ui, _top_row, button_config);

		u8* search_field = reinterpret_cast<u8*>(&_search_text);
		_search_input.init(ui,
						   _top_row,
						   {
							   .field =
								   {
									   .fields = {.data = &search_field, .size = 1},
									   .type   = editor_input_field_field_type_e::string,
								   },
							   .callbacks	= {.edited = on_search_changed, .user_data = this},
							   .placeholder = "Search",
						   });

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value		   = {1.0f, 0.5f};
		search_in.anchor_x		   = ui::anchor_e::end;
		search_in.anchor_y		   = ui::anchor_e::center;
		search_in.size_mode_x	   = ui::axis_mode_e::fill;
		search_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_in.size_value	   = {1, theme.item_height};

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "log_body");
		tree.attach(_root, _body);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_scroll_x | ui::wf_scroll_y;
		body_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_in.size_value		 = {1.0f, 1.0f};
		body_in.flow			 = ui::flow_e::column;
		body_in.child_spacing	 = 0.0f;
		body_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		paint.set_rect(_body, {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame});
		_scrollbar.init(ui, {.target = _body, .axes = editor_scrollbar_axis_xy});

		_rows.init(editor_log_store_t::RECORD_CAPACITY);
		_rows.resize(editor_log_store_t::RECORD_CAPACITY);
		_filter_dirty = true;
		ui.set_pre_layout_tick(_body, on_log_tick, this);
		refresh_rows();
	}

	void editor_panel_log_t::uninit()
	{
		_scrollbar.uninit();

		for (editor_icon_button_t& button : _filter_buttons)
			button.uninit();

		_clear_button.uninit();
		_search_input.uninit();
		_source_dropdown.uninit();

		_rows.uninit();
		_revision	  = 0;
		_filter_dirty = true;

		editor_panel_t::uninit();
	}

	void editor_panel_log_t::serialize(nlohmann::json& j) const
	{
		j					  = nlohmann::json::object();
		j["source_type"]	  = log_source_filter_to_string(_source_filter);
		j["log_filter_flags"] = static_cast<u32>(_log_filter_flags);
	}

	void editor_panel_log_t::deserialize(const nlohmann::json& j)
	{
		const string_t source_type = j.value<string_t>("source_type", "all");
		_source_filter			   = log_source_filter_from_string(source_type.c_str());
		_log_filter_flags		   = static_cast<u8>(j.value<u32>("log_filter_flags", static_cast<u32>(log_level_filter_all)));
	}

	void editor_panel_log_t::make_visible(bool visible)
	{
		_is_visible	  = visible;
		_filter_dirty = true;
		editor_panel_t::make_visible(visible);
		if (!visible)
			_source_dropdown.close();
	}

	void editor_panel_log_t::refresh_rows()
	{
		if (!_is_visible)
			return;
		const editor_log_store_t& store			  = editor_log_store_t::get();
		const bool				  history_changed = _revision != store.get_revision();
		if (!history_changed && !_filter_dirty)
			return;

		const bool scroll_to_end								= history_changed && is_scrolled_to_end();
		bool	   visible[editor_log_store_t::RECORD_CAPACITY] = {};
		for (u32 i = 0; i < store.get_record_count(); ++i)
		{
			const editor_log_record_t& record		  = store.get_record(i);
			const char*				   text			  = store.get_text(record);
			const u32				   slot			  = static_cast<u32>(record.sequence % editor_log_store_t::RECORD_CAPACITY);
			log_row_t&				   row			  = _rows[slot];
			const bool				   changed		  = row.sequence != record.sequence;
			bool					   matches_filter = row.visible;
			if (changed || _filter_dirty)
				matches_filter = is_record_visible(record, text);
			if (!matches_filter)
			{
				row.sequence = record.sequence;
				continue;
			}

			visible[slot]	   = true;
			const bool created = row.root == NULL_WIDGET;
			if (created)
				create_row(row);
			if (changed || !row.visible)
				update_row(row, record, text);
			if (!created && (changed || _filter_dirty))
			{
				ui::layout_tree_t& tree = _ui->get_tree();
				tree.attach(_body, row.root);
				tree.draw_order(row.root) = tree.draw_order_const(_body) + 1;
			}
			set_row_visible(row, true);
		}

		for (u32 i = 0; i < editor_log_store_t::RECORD_CAPACITY; ++i)
		{
			log_row_t& row = _rows[i];
			if (visible[i] || !row.visible)
				continue;
			set_row_visible(row, false);
			_ui->clear_widget_text(row.text);
		}

		_revision	  = store.get_revision();
		_filter_dirty = false;
		if (scroll_to_end)
			_scrollbar.scroll_to_end_y();
	}

	void editor_panel_log_t::create_row(log_row_t& row)
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		row.root = ui.allocate_widget();
		ui.set_widget_debug_name(row.root, "log_row");
		tree.attach(_body, row.root);
		tree.draw_order(row.root) = tree.draw_order_const(_body) + 1;
		ui::layout_in_t& row_in	  = tree.in(row.root);
		row_in.size_mode_x		  = ui::axis_mode_e::sum_children;
		row_in.size_mode_y		  = ui::axis_mode_e::max_children;
		row_in.flow				  = ui::flow_e::row;
		row_in.child_spacing	  = theme.item_spacing;
		ui.get_input().set_listener(row.root, {.on_click = on_log_row_clicked, .user_data = this});

		row.icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon, "log_row_icon");
		tree.attach(row.root, row.icon);
		ui::layout_in_t& icon_in = tree.in(row.icon);
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		icon_in.size_value		 = {theme.item_height, 1.0f};
		paint.set_text(row.icon, nullptr, 0, {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		row.text = ui.allocate_widget();
		ui.set_widget_debug_name(row.text, "log_row_text");
		tree.attach(row.root, row.text);
		paint.set_text(row.text, nullptr, 0, {.font = theme.font_default_mono, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_log_t::update_row(log_row_t& row, const editor_log_record_t& record, const char* text)
	{
		const std::time_t timestamp	 = static_cast<std::time_t>(record.timestamp);
		std::tm			  local_time = {};
#ifdef SFG_PLATFORM_WINDOWS
		const bool time_valid = localtime_s(&local_time, &timestamp) == 0;
#else
		const bool time_valid = localtime_r(&timestamp, &local_time) != nullptr;
#endif
		char prefix[EDITOR_LOG_TIME_PREFIX_LENGTH + 1] = "[--:--:--] ";
		if (time_valid)
		{
			const size_t written = std::strftime(prefix, sizeof(prefix), "[%H:%M:%S] ", &local_time);
			SFG_ASSERT(written == EDITOR_LOG_TIME_PREFIX_LENGTH);
		}
		else
			SFG_ERR("failed to convert log timestamp {0}", record.timestamp);

		frame_string_t<char> display_text = {};
		display_text.reserve(EDITOR_LOG_TIME_PREFIX_LENGTH + record.text_size + (record.truncated ? 12 : 0));
		display_text.append(prefix, EDITOR_LOG_TIME_PREFIX_LENGTH);
		display_text.append(text, record.text_size);
		if (record.truncated)
			display_text += " [truncated]";

		const vec4f_t color						  = log_level_to_color(record.level, editor_theme_t::get());
		_ui->get_paint().def(row.icon).text.color = color;
		_ui->get_paint().def(row.text).text.color = color;
		_ui->set_widget_text(row.icon, log_level_to_icon(record.level));
		_ui->set_widget_text(row.text, display_text.c_str(), static_cast<u32>(display_text.size()));
		row.sequence = record.sequence;
	}

	void editor_panel_log_t::set_row_visible(log_row_t& row, bool visible)
	{
		if (row.visible == visible)
			return;
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.in(row.root).flags = visible ? ui::wf_visible | ui::wf_input : 0;
		tree.in(row.icon).flags = visible ? ui::wf_visible : 0;
		tree.in(row.text).flags = visible ? ui::wf_visible : 0;
		row.visible				= visible;
	}

	bool editor_panel_log_t::is_record_visible(const editor_log_record_t& record, const char* text) const
	{
		if (_source_filter == log_source_filter_e::engine && record.source != log_source_e::engine)
			return false;
		if (_source_filter == log_source_filter_e::game && record.source != log_source_e::game)
			return false;
		if ((_log_filter_flags & log_level_to_filter_flag(record.level)) == 0)
			return false;
		if (_search_text.empty())
			return true;

		return string_util::contains_case_insensitive(text, record.text_size, _search_text.c_str(), _search_text.size());
	}

	bool editor_panel_log_t::is_scrolled_to_end() const
	{
		const ui::layout_in_t&	in	= _ui->get_tree().in_const(_body);
		const ui::layout_out_t& out = _ui->get_tree().out(_body);
		return out.max_scroll.y <= 0.0f || in.scroll_offset.y <= -out.max_scroll.y + EDITOR_LOG_AUTO_SCROLL_SLOP;
	}

	u16 editor_panel_log_t::get_selected_source(void* user_data)
	{
		return static_cast<u16>(static_cast<editor_panel_log_t*>(user_data)->_source_filter);
	}

	void editor_panel_log_t::on_source_pressed(u16 value, void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		panel._source_filter	  = static_cast<log_source_filter_e>(value);
		panel._filter_dirty		  = true;
	}

	void editor_panel_log_t::on_filter_pressed(bool toggled, void* user_data)
	{
		const log_filter_button_data_t& data = *static_cast<log_filter_button_data_t*>(user_data);
		if (toggled)
			data.panel->_log_filter_flags |= data.flag;
		else
			data.panel->_log_filter_flags &= static_cast<u8>(~data.flag);
		data.panel->_filter_dirty = true;
	}

	void editor_panel_log_t::on_clear_pressed(bool toggled, void* user_data)
	{
		editor_log_store_t::get().clear();
	}

	void editor_panel_log_t::on_search_changed(void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->_filter_dirty = true;
	}

	void editor_panel_log_t::on_log_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		const editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		const auto				  row	= std::find_if(panel._rows.begin(), panel._rows.end(), [id](const log_row_t& entry) { return entry.root == id; });
		SFG_ASSERT(row != panel._rows.end());
		const char* text   = panel._ui->widget_text(row->text);
		const u32	length = panel._ui->widget_text_len(row->text);
		SFG_ASSERT(length >= EDITOR_LOG_TIME_PREFIX_LENGTH);
		process::push_clipboard(text + EDITOR_LOG_TIME_PREFIX_LENGTH);
	}

	void editor_panel_log_t::on_log_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->refresh_rows();
	}
}
