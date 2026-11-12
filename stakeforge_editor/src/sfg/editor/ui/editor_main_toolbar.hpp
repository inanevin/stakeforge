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

#include "editor/ui/widgets/editor_widgets_icon_button.hpp"
#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/vendor/nhlohmann/json_fwd.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	enum class editor_world_view_e : u8;
	enum class editor_play_state_e : u8;

	class SFG_EDITOR_API editor_main_toolbar_t final
	{
	public:
		editor_main_toolbar_t()												 = default;
		~editor_main_toolbar_t()											 = default;
		editor_main_toolbar_t(const editor_main_toolbar_t& other)			 = delete;
		editor_main_toolbar_t& operator=(const editor_main_toolbar_t& other) = delete;

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

		editor_world_view_e get_world_view() const;

	private:
		void refresh_play_controls();

		static void on_play_toggled(bool toggled, void* user_data);
		static void on_pause_toggled(bool toggled, void* user_data);
		static void on_edit_mode_toggled(bool toggled, void* user_data);
		static void on_debug_toggled(bool toggled, void* user_data);
		static void on_step_pressed(bool toggled, void* user_data);
		static void on_play_controls_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		editor_icon_button_t _play_button;
		editor_icon_button_t _pause_button;
		editor_icon_button_t _step_button;
		editor_icon_button_t _edit_button;
		editor_icon_button_t _debug_button;
		ui::ui_context*		 _ui					 = nullptr;
		ui::widget_id_t		 _root					 = NULL_WIDGET;
		ui::widget_id_t		 _play_frame			 = NULL_WIDGET;
		editor_world_view_e	 _pending_world_view	 = {};
		editor_play_state_e	 _displayed_state		 = {};
		bool				 _displayed_playing		 = false;
		bool				 _displayed_paused		 = false;
		bool				 _has_pending_world_view = false;
	};

	SFG_EDITOR_API void to_json(nlohmann::json& j, const editor_world_view_e& view);
	SFG_EDITOR_API void from_json(const nlohmann::json& j, editor_world_view_e& view);
	SFG_EDITOR_API void to_json(nlohmann::json& j, const editor_main_toolbar_t& toolbar);
	SFG_EDITOR_API void from_json(const nlohmann::json& j, editor_main_toolbar_t& toolbar);
}
