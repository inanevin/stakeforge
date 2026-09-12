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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/data/vector.hpp>

namespace sfg
{
	struct resource_entry_t;

	class editor_panel_resources_t final : public editor_panel_t
	{
	public:
		editor_panel_resources_t();
		~editor_panel_resources_t() override								 = default;
		editor_panel_resources_t(const editor_panel_resources_t&)			 = delete;
		editor_panel_resources_t& operator=(const editor_panel_resources_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

	private:
		struct resource_row_t
		{
			ui::widget_id_t root	= NULL_WIDGET;
			ui::widget_id_t divider = NULL_WIDGET;
		};

		struct resource_row_source_t
		{
			const resource_entry_t* entry = nullptr;
			sid_t					hash  = 0;
		};

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh_rows();
		void clear_rows();
		void add_row(sid_t hash, const resource_entry_t& entry);

		// -----------------------------------------------------------------------------
		// handlers
		// -----------------------------------------------------------------------------

		static void on_resources_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		editor_scrollbar_t				_scrollbar			 = {};
		vector_t<resource_row_t>		_rows				 = {};
		vector_t<resource_row_source_t> _row_sources		 = {};
		ui::widget_id_t					_header				 = NULL_WIDGET;
		ui::widget_id_t					_body				 = NULL_WIDGET;
		u64								_resource_generation = 0;
		u32								_refresh_tick		 = 0;
	};
}
