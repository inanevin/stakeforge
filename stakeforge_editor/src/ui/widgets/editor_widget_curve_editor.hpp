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

#include "ui/widgets/editor_widget_curve_edit.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/curve_def.hpp>

namespace sfg
{
	class editor_widget_curve_editor_t final
	{
	public:
		editor_widget_curve_editor_t()												 = default;
		~editor_widget_curve_editor_t()												 = default;
		editor_widget_curve_editor_t(const editor_widget_curve_editor_t&)			 = delete;
		editor_widget_curve_editor_t& operator=(const editor_widget_curve_editor_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();
		void set_curves(span_t<const sid_t> curves);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_display();
		void clear_display();
		void begin_curve_edit();
		void submit_curve_edit();
		void clear_curve_edit();

		static void on_curve_edit_begin(void* user_data);
		static void on_curve_edited(void* user_data);
		static void on_curve_edit_submitted(void* user_data);

	private:
		ui::ui_context*									_ui						= nullptr;
		ui::widget_id_t									_root					= NULL_WIDGET;
		editor_widget_reflection_t						_reflection				= {};
		editor_widget_curve_edit_t						_curve_edit				= {};
		vector_t<editor_widget_reflection_fold_state_t> _field_states			= {};
		vector_t<ui::widget_id_t>						_labels					= {};
		vector_t<curve_def_t>							_curves					= {};
		vector_t<sid_t>									_curve_ids				= {};
		vector_t<curve_def_t>							_edit_previous_curves	= {};
		vector_t<sid_t>									_edit_curve_ids			= {};
		bool											_reflection_initialized = false;
		bool											_curve_edit_initialized = false;
		bool											_edit_active			= false;
	};
}
