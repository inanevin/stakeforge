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
		bool can_mutate_ui_topology() const;
		void begin_curve_edit();
		void submit_curve_edit();
		void clear_curve_edit();
		void request_curves_refresh(span_t<const sid_t> curves);
		void request_display_refresh();
		void flush_pending_ui_mutations();

		static void on_curve_edit_begin(void* user_data);
		static void on_curve_edited(void* user_data);
		static void on_curve_edit_submitted(void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		ui::ui_context*									_ui						= nullptr;
		ui::widget_id_t									_root					= NULL_WIDGET;
		editor_widget_reflection_t						_reflection				= {};
		editor_widget_curve_edit_t						_curve_edit				= {};
		vector_t<editor_widget_reflection_fold_state_t> _field_states			= {};
		vector_t<ui::widget_id_t>						_labels					= {};
		vector_t<sid_t>									_pending_curve_ids		= {};
		vector_t<curve_def_t>							_curves					= {};
		vector_t<sid_t>									_curve_ids				= {};
		vector_t<curve_def_t>							_edit_previous_curves	= {};
		vector_t<sid_t>									_edit_curve_ids			= {};
		bool											_reflection_initialized = false;
		bool											_curve_edit_initialized = false;
		bool											_edit_active			= false;
		bool											_refresh_curves_pending = false;
	};
}
