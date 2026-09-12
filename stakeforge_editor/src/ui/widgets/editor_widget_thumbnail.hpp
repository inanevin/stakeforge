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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct editor_widget_thumbnail_config_t
	{
		sid_t thumbnail = NULL_SID;
	};

	class editor_widget_thumbnail_t final
	{
	public:
		editor_widget_thumbnail_t()											   = default;
		~editor_widget_thumbnail_t()										   = default;
		editor_widget_thumbnail_t(const editor_widget_thumbnail_t&)			   = delete;
		editor_widget_thumbnail_t& operator=(const editor_widget_thumbnail_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_thumbnail_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_thumbnail(sid_t thumbnail);
		void set_visible(bool visible);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_frame();
		void set_default_frame();
		void set_texture_frame();

		static void on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		ui::ui_context* _ui			  = nullptr;
		ui::widget_id_t _root		  = NULL_WIDGET;
		sid_t			_thumbnail	  = NULL_SID;
		u32				_tick_counter = 0;
		bool			_ticking	  = false;
	};
}
