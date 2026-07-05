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
