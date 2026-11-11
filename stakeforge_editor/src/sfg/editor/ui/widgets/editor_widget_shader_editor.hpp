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

#include "editor_widget_reflection.hpp"

#include <sfg/core/runtime/resources/shader_cook.hpp>

namespace sfg
{
	class SFG_EDITOR_API editor_widget_shader_editor_t final
	{
	public:
		editor_widget_shader_editor_t()														 = default;
		~editor_widget_shader_editor_t()													 = default;
		editor_widget_shader_editor_t(const editor_widget_shader_editor_t& other)			 = delete;
		editor_widget_shader_editor_t& operator=(const editor_widget_shader_editor_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_shaders(span_t<const sid_t> shaders);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static void on_edit_begin(void* user_data);
		static void on_edit_submitted(void* user_data);

		editor_widget_reflection_t	   _info	   = {};
		editor_widget_reflection_t	   _name	   = {};
		vector_t<shader_cook_config_t> _configs	   = {};
		vector_t<shader_cook_config_t> _previous   = {};
		vector_t<sid_t>				   _shader_ids = {};
		ui::ui_context*				   _ui		   = nullptr;
		ui::widget_id_t				   _root	   = NULL_WIDGET;
	};
}
