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

#include "ui/editor_global_toolbar.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_main_toolbar_t final
	{
	public:
		editor_main_toolbar_t()										   = default;
		~editor_main_toolbar_t()									   = default;
		editor_main_toolbar_t(const editor_main_toolbar_t&)			   = delete;
		editor_main_toolbar_t& operator=(const editor_main_toolbar_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void serialize(nlohmann::json& j) const;
		void deserialize(const nlohmann::json& j);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool is_window_drag_region(const vec2f_t& pos) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_main_toolbar_world_view_e get_world_view() const
		{
			return editor_global_toolbar_t::get().get_world_view();
		}

	private:
		static u16	get_selected_world_view(void* user_data);
		static void on_world_view_pressed(u16 value, void* user_data);

	private:
		editor_dropdown_t _world_view_dropdown;
		ui::ui_context*	  _ui		   = nullptr;
		ui::widget_id_t	  _root		   = NULL_WIDGET;
		ui::widget_id_t	  _world_label = NULL_WIDGET;
	};

	void to_json(nlohmann::json& j, const editor_main_toolbar_world_view_e& view);
	void from_json(const nlohmann::json& j, editor_main_toolbar_world_view_e& view);
	void to_json(nlohmann::json& j, const editor_main_toolbar_t& toolbar);
	void from_json(const nlohmann::json& j, editor_main_toolbar_t& toolbar);
}
