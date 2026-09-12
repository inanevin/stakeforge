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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
	class paint_layer_t;
	class vg_canvas_t;
}

namespace sfg
{
	struct editor_widget_spinner_config_t
	{
		vec4f_t outer_color = {1.0f, 1.0f, 1.0f, 1.0f};
		vec4f_t inner_color = {1.0f, 1.0f, 1.0f, 1.0f};
	};

	class editor_widget_spinner_t final
	{
	public:
		editor_widget_spinner_t()										   = default;
		~editor_widget_spinner_t()										   = default;
		editor_widget_spinner_t(const editor_widget_spinner_t&)			   = delete;
		editor_widget_spinner_t& operator=(const editor_widget_spinner_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_spinner_config_t& config);
		void uninit();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static void draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		vec4f_t			_outer_color = {};
		vec4f_t			_inner_color = {};
		ui::ui_context* _ui			 = nullptr;
		ui::widget_id_t _root		 = NULL_WIDGET;
	};
}
