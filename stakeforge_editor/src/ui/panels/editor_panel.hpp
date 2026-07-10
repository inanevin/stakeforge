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

#include "ui/panels/editor_panel_types.hpp"
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_panel_t
	{
	public:
		editor_panel_t();
		virtual ~editor_panel_t()							 = default;
		editor_panel_t(const editor_panel_t&)				 = delete;
		editor_panel_t& operator=(const editor_panel_t&)	 = delete;
		editor_panel_t(editor_panel_t&&) noexcept			 = default;
		editor_panel_t& operator=(editor_panel_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		virtual void init(ui::ui_context& ui, ui::widget_id_t parent);
		virtual void uninit();
		virtual void serialize(nlohmann::json& j) const;
		virtual void deserialize(const nlohmann::json& j);
		void		 assign(ui::ui_context& ui, ui::widget_id_t parent);
		void		 deassign();
		virtual void make_visible(bool visible);
		void		 set_title(const char* title);
		void		 set_icon(const char* icon);
		void		 set_type(editor_panel_type_e type);
		void		 set_instance_id(sid_t instance_id);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}
		inline bool is_inited() const
		{
			return _ui != nullptr;
		}
		inline const char* get_title() const
		{
			return _title;
		}
		inline const char* get_icon() const
		{
			return _icon;
		}
		inline editor_panel_type_e get_type() const
		{
			return _type;
		}
		inline sid_t get_instance_id() const
		{
			return _instance_id;
		}

	protected:
		ui::ui_context*		_ui			 = nullptr;
		ui::widget_id_t		_root		 = NULL_WIDGET;
		const char*			_title		 = "";
		const char*			_icon		 = nullptr;
		sid_t				_instance_id = 0;
		editor_panel_type_e _type		 = editor_panel_type_e::max;
	};
}
