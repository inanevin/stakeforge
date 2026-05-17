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

#include "ui/editor_popup_controller.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/widgets/editor_widgets_input_field.hpp"
#include <sfg/data/string.hpp>

namespace sfg
{
	enum assets_filter_flags_e : u8
	{
		assets_filter_all		 = 1 << 0,
		assets_filter_favourites = 1 << 1,
	};

	class editor_panel_assets_t final : public editor_panel_t
	{
	public:
		editor_panel_assets_t();
		~editor_panel_assets_t() override							   = default;
		editor_panel_assets_t(const editor_panel_assets_t&)			   = delete;
		editor_panel_assets_t& operator=(const editor_panel_assets_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;
		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void make_visible(bool visible) override;

	private:
		void apply_pane_split();

		void open_filter_popup();

		static void on_filter_popup_pressed(u16 value, void* user_data);
		static void on_filter_button_pressed(bool toggled, void* user_data);
		static void on_search_changed(const char* value, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_icon_button_t  _filter_button			= {};
		editor_input_field_t  _search_input				= {};
		editor_split_border_t _split_border				= {};
		ui::widget_id_t		  _assets_left_pane			= NULL_WIDGET;
		ui::widget_id_t		  _assets_left_pane_top_row = NULL_WIDGET;
		ui::widget_id_t		  _assets_left_pane_body	= NULL_WIDGET;
		ui::widget_id_t		  _assets_body_pane			= NULL_WIDGET;
		string_t			  _search_str				= {};
		f32					  _pane_split				= 0.3f;
		u8					  _filter_flags				= assets_filter_all;
	};
}
