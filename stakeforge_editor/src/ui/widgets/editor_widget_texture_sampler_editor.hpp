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
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include "ui/widgets/editor_widget_reflection.hpp"

namespace sfg
{
	namespace ui
	{
		class ui_context;
	}

	class editor_widget_texture_sampler_editor_t final
	{
	public:
		editor_widget_texture_sampler_editor_t()														 = default;
		~editor_widget_texture_sampler_editor_t()														 = default;
		editor_widget_texture_sampler_editor_t(const editor_widget_texture_sampler_editor_t&)			 = delete;
		editor_widget_texture_sampler_editor_t& operator=(const editor_widget_texture_sampler_editor_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();
		void set_texture_samplers(span_t<const sid_t> samplers);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_display();
		void clear_display();
		void append_property_row(ui::widget_id_t row);
		void begin_sampler_edit();
		void submit_sampler_edit();
		void clear_sampler_edit();
		void on_sampler_edit_begin();
		void on_sampler_edited();
		void on_sampler_edit_submitted();

		static void on_sampler_edit_begin(void* user_data);
		static void on_sampler_edited(void* user_data);
		static void on_sampler_edit_submitted(void* user_data);

	private:
		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;

		editor_widget_reflection_t						_reflection				= {};
		vector_t<editor_widget_reflection_fold_state_t> _field_states			= {};
		vector_t<ui::widget_id_t>						_rows					= {};
		vector_t<ui::widget_id_t>						_dividers				= {};
		vector_t<ui::widget_id_t>						_labels					= {};
		vector_t<sampler_desc_t>						_samplers				= {};
		vector_t<sid_t>									_sampler_ids			= {};
		vector_t<sampler_desc_t>						_edit_previous_samplers = {};
		vector_t<sid_t>									_edit_sampler_ids		= {};
		bool											_reflection_initialized = false;
		bool											_edit_active			= false;
	};
}
