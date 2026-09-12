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

#include <sfg/data/vector.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include "ui/widgets/editor_tab_area.hpp"

namespace sfg
{
	class editor_panel_t;

	struct dock_node_tag_t;
	struct dock_border_tag_t;

	typedef pool_handle_t<u16, dock_node_tag_t>	  dock_node_handle_t;
	typedef pool_handle_t<u16, dock_border_tag_t> dock_border_handle_t;

	enum class dock_node_type_e : u8
	{
		leaf,
		split,
	};

	enum class dock_split_direction_e : u8
	{
		horizontal,
		vertical,
	};

	enum class dock_preview_e : u8
	{
		top,
		left,
		bottom,
		right,
		center,
		none,
	};

	const char*			   dock_node_type_to_string(dock_node_type_e type);
	dock_node_type_e	   dock_node_type_from_string(const char* value);
	const char*			   dock_split_direction_to_string(dock_split_direction_e direction);
	dock_split_direction_e dock_split_direction_from_string(const char* value);

	struct dock_border_t
	{
		dock_node_handle_t split	   = {};
		ui::widget_id_t	   widget	   = NULL_WIDGET;
		bool			   is_dragging = false;
	};

	struct dock_node_t
	{
		editor_tab_area_t		  tab_area;
		vector_t<editor_panel_t*> panels;
		rectf_t					  preview_rects[5] = {};
		f32						  split_value	   = 0.0f;
		dock_border_handle_t	  border		   = {};
		dock_node_handle_t		  split_negative   = {};
		dock_node_handle_t		  split_positive   = {};
		ui::widget_id_t			  widget		   = NULL_WIDGET;
		ui::widget_id_t			  body			   = NULL_WIDGET;
		dock_node_type_e		  node_type		   = dock_node_type_e::leaf;
		dock_split_direction_e	  split_direction  = dock_split_direction_e::horizontal;
		dock_preview_e			  hovered_preview  = dock_preview_e::none;
		bool					  is_payload_over  = false;
	};
}
