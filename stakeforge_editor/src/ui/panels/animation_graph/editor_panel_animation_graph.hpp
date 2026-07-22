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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"

#include <sfg/data/string.hpp>

namespace sfg
{
	class editor_panel_animation_graph_t final : public editor_panel_t
	{
	public:
		editor_panel_animation_graph_t();
		~editor_panel_animation_graph_t() override										 = default;
		editor_panel_animation_graph_t(const editor_panel_animation_graph_t&)			 = delete;
		editor_panel_animation_graph_t& operator=(const editor_panel_animation_graph_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void serialize(nlohmann::json& json) const override;
		void deserialize(const nlohmann::json& json) override;
		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_graph(sid_t graph_id, const char* asset_name);

	private:
		void apply_pane_split();

		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_split_border_t _split_border = {};
		string_t			  _asset_name	= {};
		sid_t				  _graph_id		= NULL_SID;
		ui::widget_id_t		  _left_pane	= NULL_WIDGET;
		ui::widget_id_t		  _right_pane	= NULL_WIDGET;
		f32					  _pane_split	= 0.72f;
	};
}
