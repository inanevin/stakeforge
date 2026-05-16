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

#include <sfg/data/vector.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include "widgets/editor_tab_area.hpp"

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
		vector_t<editor_panel_t*> panels;
		rectf_t					  preview_rects[5] = {};
		f32						  split_value	   = 0.0f;
		dock_border_handle_t	  border		   = {};
		dock_node_handle_t		  split_negative   = {};
		dock_node_handle_t		  split_positive   = {};
		ui::widget_id_t			  widget		   = NULL_WIDGET;
		ui::widget_id_t			  body			   = NULL_WIDGET;
		editor_tab_area_t		  tab_area;
		dock_node_type_e		  node_type		  = dock_node_type_e::leaf;
		dock_split_direction_e	  split_direction = dock_split_direction_e::horizontal;
		dock_preview_e			  hovered_preview = dock_preview_e::none;
		bool					  is_payload_over = false;
	};
}
