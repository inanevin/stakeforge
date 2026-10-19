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

#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/data/string.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_modal_component_add_t final
	{
	public:
		editor_modal_component_add_t()												 = default;
		~editor_modal_component_add_t()												 = default;
		editor_modal_component_add_t(const editor_modal_component_add_t&)			 = delete;
		editor_modal_component_add_t& operator=(const editor_modal_component_add_t&) = delete;

		void open(ui::ui_context& ui, editor_world_handle_t world, span_t<const entity_id_t> entities);
		void close();

	private:
		struct item_t
		{
			string_t		search_text = {};
			const char*		name		= nullptr;
			const char*		category	= nullptr;
			sid_t			type		= 0;
			char			icon[8]		= {};
			ui::widget_id_t tile		= NULL_WIDGET;
		};

		void		init(ui::ui_context& ui, ui::widget_id_t parent);
		void		uninit();
		void		rebuild();
		void		refresh_selection();
		static void init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void uninit_content(void* user_data);
		static void on_search_changed(void* user_data);
		static void on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_item_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_button_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_add(void* user_data);
		static void on_cancel(void* user_data);

	private:
		editor_input_field_t   _search			= {};
		editor_scrollbar_t	   _scrollbar		= {};
		editor_widget_button_t _cancel			= {};
		editor_widget_button_t _add				= {};
		vector_t<item_t>	   _items			= {};
		vector_t<entity_id_t>  _entities		= {};
		string_t			   _search_text		= {};
		string_t			   _search_lower	= {};
		ui::ui_context*		   _ui				= nullptr;
		editor_world_handle_t  _world			= {};
		sid_t				   _selected		= 0;
		ui::widget_id_t		   _root			= NULL_WIDGET;
		ui::widget_id_t		   _area			= NULL_WIDGET;
		ui::widget_id_t		   _column			= NULL_WIDGET;
		bool				   _refresh_pending = false;
	};
}
