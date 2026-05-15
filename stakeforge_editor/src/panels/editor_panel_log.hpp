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

#pragma once

#include "panels/editor_panel.hpp"
#include "widgets/editor_widgets_dropdown.hpp"
#include "widgets/editor_widgets_icon_button.hpp"
#include "widgets/editor_widgets_scrollbar.hpp"
#include <sfg/data/mutex.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	enum class log_level;
}

namespace sfg
{
	enum class log_source_type_e : u8
	{
		all,
		editor,
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

	class editor_panel_log_t final : public editor_panel_t
	{
	public:
		editor_panel_log_t();
		~editor_panel_log_t() override							 = default;
		editor_panel_log_t(const editor_panel_log_t&)			 = delete;
		editor_panel_log_t& operator=(const editor_panel_log_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;
		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void make_visible(bool visible) override;

	private:
		struct log_filter_button_data_t
		{
			editor_panel_log_t* panel = nullptr;
			u8					flag  = 0;
		};

		static u16	get_selected_source(void* user_data);
		static void on_source_pressed(u16 value, void* user_data);
		static void on_filter_pressed(bool toggled, void* user_data);
		static void on_log(log_level level, const char* msg, void* user_data);
		static void on_log_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		bool		is_filter_enabled(u8 flag) const;
		void		drain_pending_logs();
		void		add_log_row(log_level level, const char* text);
		void		refresh_log_filter_visibility();
		void		trim_log_rows();

	private:
		struct log_record_t
		{
			string_t  text;
			log_level level;
		};

		struct log_row_t
		{
			ui::widget_id_t root = NULL_WIDGET;
			ui::widget_id_t icon = NULL_WIDGET;
			ui::widget_id_t text = NULL_WIDGET;
			u8				flag = 0;
		};

	private:
		editor_dropdown_t		 _source_dropdown;
		editor_scrollbar_t		 _scrollbar;
		editor_icon_button_t	 _info_button;
		editor_icon_button_t	 _trace_button;
		editor_icon_button_t	 _warn_button;
		editor_icon_button_t	 _err_button;
		log_filter_button_data_t _info_filter_data;
		log_filter_button_data_t _trace_filter_data;
		log_filter_button_data_t _warn_filter_data;
		log_filter_button_data_t _err_filter_data;
		ui::widget_id_t			 _top_row = NULL_WIDGET;
		ui::widget_id_t			 _body	  = NULL_WIDGET;
		vector_t<log_row_t>		 _rows;
		vector_t<log_record_t>	 _pending_logs;
		vector_t<log_record_t>	 _drained_logs;
		mutex_t					 _pending_logs_mtx;
		log_source_type_e		 _source_type	   = log_source_type_e::all;
		u32						 _listener_id	   = 0;
		u8						 _log_filter_flags = log_level_filter_all;
	};
}
