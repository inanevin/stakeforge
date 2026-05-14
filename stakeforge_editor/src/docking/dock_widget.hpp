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

#include "docking/dock_area.hpp"
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class dock_widget_t final
	{
	public:
		dock_widget_t()									   = default;
		~dock_widget_t()								   = default;
		dock_widget_t(const dock_widget_t&)				   = delete;
		dock_widget_t& operator=(const dock_widget_t&)	   = delete;
		dock_widget_t(dock_widget_t&&) noexcept			   = default;
		dock_widget_t& operator=(dock_widget_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static constexpr u32 DOCK_POOL_INITIAL_CAPACITY = 32;

		dock_node_handle_t	 create_leaf_node(ui::widget_id_t parent);
		void				 dock_node_add_panel(dock_node_t& node, editor_panel_t* panel);
		void				 set_leaf_active_panel(dock_node_t& node, sid_t active_tab);
		dock_node_handle_t	 alloc_dock_node();
		void				 free_dock_node(dock_node_handle_t handle);
		dock_border_handle_t alloc_dock_border();
		void				 free_dock_border(dock_border_handle_t handle);

		static void on_leaf_tab_switched(editor_tab_area_t& tab_area, sid_t identifier, void* user_data);

	private:
		ui::ui_context*											  _ui			= nullptr;
		ui::widget_id_t											  _root			= NULL_WIDGET;
		dock_node_handle_t										  _root_node	= {};
		dynamic_gen_pool_t<dock_node_t, u16, dock_node_tag_t>	  _dock_nodes	= {};
		dynamic_gen_pool_t<dock_border_t, u16, dock_border_tag_t> _dock_borders = {};
	};
}
