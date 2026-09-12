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

#include "ui/panels/editor_panel_types.hpp"
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
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
		void		 set_sub_item_id(sid_t sub_item_id);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}
		inline ui::ui_context& get_ui()
		{
			SFG_ASSERT(_ui != nullptr);
			return *_ui;
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
		inline sid_t get_sub_item_id() const
		{
			return _sub_item_id;
		}

	protected:
		void refresh_title(const char* detail = nullptr, const char* detail_prefix = nullptr, bool dirty = false);

	protected:
		ui::ui_context*		_ui			 = nullptr;
		ui::widget_id_t		_root		 = NULL_WIDGET;
		const char*			_title		 = "";
		const char*			_icon		 = nullptr;
		string_t			_title_text	 = {};
		sid_t				_instance_id = 0;
		sid_t				_sub_item_id = 0;
		editor_panel_type_e _type		 = editor_panel_type_e::max;
	};
}
