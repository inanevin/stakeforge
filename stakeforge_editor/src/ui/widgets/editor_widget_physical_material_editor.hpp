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

#include "ui/widgets/editor_widget_reflection.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/physical_material_def.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	namespace ui
	{
		class ui_context;
	}

	class editor_widget_physical_material_editor_t final
	{
	public:
		editor_widget_physical_material_editor_t()															 = default;
		~editor_widget_physical_material_editor_t()															 = default;
		editor_widget_physical_material_editor_t(const editor_widget_physical_material_editor_t&)			 = delete;
		editor_widget_physical_material_editor_t& operator=(const editor_widget_physical_material_editor_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();
		void set_physical_materials(span_t<const sid_t> physical_materials);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_display();
		void clear_display();
		void append_property_row(ui::widget_id_t row);
		bool can_mutate_ui_topology() const;
		void begin_physical_material_edit();
		void submit_physical_material_edit();
		void clear_physical_material_edit();
		void request_physical_materials_refresh(span_t<const sid_t> physical_materials);
		void request_display_refresh();
		void flush_pending_ui_mutations();
		void on_physical_material_edit_begin();
		void on_physical_material_edited();
		void on_physical_material_edit_submitted();

		static void on_physical_material_edit_begin(void* user_data);
		static void on_physical_material_edited(void* user_data);
		static void on_physical_material_edit_submitted(void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;

		editor_widget_reflection_t						_reflection							= {};
		vector_t<editor_widget_reflection_fold_state_t> _field_states						= {};
		vector_t<ui::widget_id_t>						_rows								= {};
		vector_t<ui::widget_id_t>						_dividers							= {};
		vector_t<ui::widget_id_t>						_labels								= {};
		vector_t<sid_t>									_pending_physical_material_ids		= {};
		vector_t<physical_material_def_t>				_physical_materials					= {};
		vector_t<sid_t>									_physical_material_ids				= {};
		vector_t<physical_material_def_t>				_edit_previous_physical_materials	= {};
		vector_t<sid_t>									_edit_physical_material_ids			= {};
		bool											_reflection_initialized				= false;
		bool											_edit_active						= false;
		bool											_refresh_physical_materials_pending = false;
	};
}
