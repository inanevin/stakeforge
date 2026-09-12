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

#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	enum class editor_world_view_e : u8;
	enum class editor_play_mode_e : u8;

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

		editor_world_view_e get_world_view() const;

	private:
		void refresh_play_controls();

		static u16	get_selected_world_view(void* user_data);
		static void on_world_view_pressed(u16 value, void* user_data);
		static void on_play_toggled(bool toggled, void* user_data);
		static void on_play_physics_toggled(bool toggled, void* user_data);
		static void on_pause_toggled(bool toggled, void* user_data);
		static void on_step_pressed(bool toggled, void* user_data);
		static void on_play_controls_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		editor_dropdown_t	 _world_view_dropdown;
		editor_icon_button_t _play_button;
		editor_icon_button_t _play_physics_button;
		editor_icon_button_t _pause_button;
		editor_icon_button_t _step_button;
		ui::ui_context*		 _ui					 = nullptr;
		ui::widget_id_t		 _root					 = NULL_WIDGET;
		ui::widget_id_t		 _world_frame			 = NULL_WIDGET;
		ui::widget_id_t		 _world_label			 = NULL_WIDGET;
		ui::widget_id_t		 _play_frame			 = NULL_WIDGET;
		editor_world_view_e	 _pending_world_view	 = {};
		editor_play_mode_e	 _displayed_mode		 = {};
		bool				 _has_pending_world_view = false;
	};

	void to_json(nlohmann::json& j, const editor_world_view_e& view);
	void from_json(const nlohmann::json& j, editor_world_view_e& view);
	void to_json(nlohmann::json& j, const editor_main_toolbar_t& toolbar);
	void from_json(const nlohmann::json& j, editor_main_toolbar_t& toolbar);
}
