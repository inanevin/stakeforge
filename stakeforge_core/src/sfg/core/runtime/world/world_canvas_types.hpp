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

#include "ecs_defs.hpp"
#include "core/common/type_id.hpp"
#include "core/math/vec2f.hpp"
#include "core/runtime/resources/common_resources.hpp"

namespace sfg
{
	enum class canvas_render_stage_e : u8
	{
		before_post_process,
		after_post_process,
	};

	SFG_DEFINE_TYPE_ID(canvas_render_stage_e);

	struct component_canvas_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_canvas";

		resource_handle_t	  default_font			   = DEFAULT_FONT_ASSET_GUID;
		resource_handle_t	  default_shader		   = DEFAULT_CANVAS_UI_SHADER_ASSET_GUID;
		resource_handle_t	  text_shader			   = DEFAULT_CANVAS_UI_TEXT_SHADER_ASSET_GUID;
		resource_handle_t	  texture_shader		   = DEFAULT_CANVAS_UI_TEXTURE_SHADER_ASSET_GUID;
		vec2f_t				  canvas_size			   = {1000.0f, 600.0f};
		vec2f_t				  pivot					   = {0.5f, 0.5f};
		f32					  pixels_per_unit		   = 500.0f;
		u32					  text_pool_budget_bytes   = 16 * 1024;
		u32					  vertex_pool_budget_bytes = 128 * 1024;
		u32					  index_pool_budget_bytes  = 64 * 1024;
		i32					  sort_order			   = 0;
		f32					  ui_scale				   = 1.0f;
		u16					  max_widgets			   = 256;
		canvas_render_stage_e render_stage			   = canvas_render_stage_e::after_post_process;
		bool				  is_3d					   = false;
		bool				  input_enabled			   = true;
	};

	SFG_DEFINE_TYPE_ID(component_canvas_t);

	using canvas_widget_handle_t = u32;
#define NULL_CANVAS_WIDGET_HANDLE 0

	enum class canvas_event_type_e : u8
	{
		press,
		release,
		click,
		double_click,
		hover_enter,
		hover_exit,
		hover_move,
		drag_begin,
		drag,
		drag_end,
		focus_gain,
		focus_lose,
		key,
		wheel,
	};

	struct canvas_event_t
	{
		vec2f_t				   position	   = vec2f_t::zero;
		vec2f_t				   delta	   = vec2f_t::zero;
		canvas_widget_handle_t widget	   = NULL_CANVAS_WIDGET_HANDLE;
		entity_id_t			   canvas	   = NULL_ENTITY_ID;
		f32					   wheel_delta = 0.0f;
		u16					   key		   = 0;
		u16					   scan_code   = 0;
		canvas_event_type_e	   type		   = canvas_event_type_e::click;
		u8					   button	   = UINT8_MAX;
		u8					   action	   = 0;
		u8					   from_nav	   = 0;
	};

}
