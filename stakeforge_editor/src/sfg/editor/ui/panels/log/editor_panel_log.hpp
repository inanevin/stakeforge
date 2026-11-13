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

#pragma once

#include "editor/editor_exports.hpp"

#include "editor/ui/panels/editor_panel.hpp"
#include "editor/ui/widgets/editor_widget_input_field.hpp"
#include "editor/ui/widgets/editor_widgets_dropdown.hpp"
#include "editor/ui/widgets/editor_widgets_icon_button.hpp"
#include "editor/ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/core/data/fixed_vector.hpp>
#include <sfg/core/data/string.hpp>

namespace sfg
{
	struct editor_log_record_t;

	enum class log_source_filter_e : u8
	{
		all,
		engine,
		game,
	};

	enum log_level_filter_flags_e : u8
	{
		log_level_filter_info  = 1 << 0,
		log_level_filter_trace = 1 << 1,
		log_level_filter_warn  = 1 << 2,
		log_level_filter_err   = 1 << 3,
		log_level_filter_all   = log_level_filter_info | log_level_filter_trace | log_level_filter_warn | log_level_filter_err,
	};

	class SFG_EDITOR_API editor_panel_log_t final : public editor_panel_t
	{
	public:
		editor_panel_log_t();
		~editor_panel_log_t() override								   = default;
		editor_panel_log_t(const editor_panel_log_t& other)			   = delete;
		editor_panel_log_t& operator=(const editor_panel_log_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;
		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void make_visible(bool visible) override;

	private:
		static constexpr u32 FILTER_BUTTON_COUNT = 4;

		struct log_row_t
		{
			u64				sequence = 0;
			ui::widget_id_t root	 = NULL_WIDGET;
			ui::widget_id_t icon	 = NULL_WIDGET;
			ui::widget_id_t text	 = NULL_WIDGET;
			bool			visible	 = false;
		};

		struct log_filter_button_data_t
		{
			editor_panel_log_t* panel = nullptr;
			u8					flag  = 0;
		};

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh_rows();
		void create_row(log_row_t& row);
		void update_row(log_row_t& row, const editor_log_record_t& record, const char* text);
		void set_row_visible(log_row_t& row, bool visible);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool is_record_visible(const editor_log_record_t& record, const char* text) const;
		bool is_scrolled_to_end() const;

		// -----------------------------------------------------------------------------
		// handlers
		// -----------------------------------------------------------------------------

		static u16	get_selected_source(void* user_data);
		static void on_source_pressed(u16 value, void* user_data);
		static void on_filter_pressed(bool toggled, void* user_data);
		static void on_clear_pressed(bool toggled, void* user_data);
		static void on_search_changed(void* user_data);
		static void on_log_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_log_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		editor_dropdown_t		  _source_dropdown						   = {};
		editor_input_field_t	  _search_input							   = {};
		editor_scrollbar_t		  _scrollbar							   = {};
		editor_icon_button_t	  _filter_buttons[FILTER_BUTTON_COUNT]	   = {};
		editor_icon_button_t	  _clear_button							   = {};
		log_filter_button_data_t  _filter_button_data[FILTER_BUTTON_COUNT] = {};
		fixed_vector_t<log_row_t> _rows									   = {};
		string_t				  _search_text							   = {};
		u64						  _revision								   = 0;
		ui::widget_id_t			  _top_row								   = NULL_WIDGET;
		ui::widget_id_t			  _body									   = NULL_WIDGET;
		log_source_filter_e		  _source_filter						   = log_source_filter_e::all;
		u8						  _log_filter_flags						   = log_level_filter_all;
		bool					  _filter_dirty							   = true;
		bool					  _is_visible							   = true;
	};
}
