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

#include "core/common/sfg_exports.hpp"

#include "core/common/size_definitions.hpp"
#include "world_canvas_types.hpp"
#include "core/math/vec2f.hpp"
#include "core/math/vec4f.hpp"
#include "core/runtime/resources/resource_handle.hpp"
#include "core/runtime/world/ecs_defs.hpp"

namespace sfg
{
	class world_t;
	struct world_canvas_render_snapshot_t;
	struct world_canvas_draw_snapshot_t;
	struct component_system_canvas_t;

	namespace ui
	{
		class ui_context;
		struct layout_in_t;
		struct vg_rect_paint_t;
		struct vg_text_style_t;
	}

	class SFG_API world_canvas_controller_t final
	{
	public:
		world_canvas_controller_t()											   = default;
		~world_canvas_controller_t()										   = default;
		world_canvas_controller_t(const world_canvas_controller_t&)			   = delete;
		world_canvas_controller_t& operator=(const world_canvas_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world);
		void uninit();
		void clear();
		void clear_widgets();
		void destroy_entity(entity_id_t entity);
		void sync_create_destroy_canvases();
		void tick(f32 dt_seconds);

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void write_render_snapshot(world_canvas_render_snapshot_t& snapshot) const;

		// -----------------------------------------------------------------------------
		// input
		// -----------------------------------------------------------------------------

		bool key_event(u16 key, u16 scan_code, u8 action);
		bool mouse_button_event(u8 button, u8 action, const vec2f_t& position);
		bool mouse_move_event(const vec2f_t& position);
		bool mouse_wheel_event(const vec2f_t& position, f32 delta);
		bool is_keyboard_focus_active() const;
		void dispatch_events();

		// -----------------------------------------------------------------------------
		// widgets
		// -----------------------------------------------------------------------------

		canvas_widget_handle_t create_frame(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const ui::vg_rect_paint_t& style);
		canvas_widget_handle_t create_text(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_text_style_t& style);
		canvas_widget_handle_t create_image(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, resource_handle_t texture, const vec4f_t& tint);
		canvas_widget_handle_t create_button(
			entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_rect_paint_t& frame_style, const ui::vg_text_style_t& text_style, const vec4f_t& hover_color, const vec4f_t& press_color);
		bool destroy_widget(entity_id_t canvas, canvas_widget_handle_t widget);
		bool clear_widgets(entity_id_t canvas);
		bool set_layout(entity_id_t canvas, canvas_widget_handle_t widget, const ui::layout_in_t& layout);
		bool set_visible(entity_id_t canvas, canvas_widget_handle_t widget, bool visible);
		bool set_enabled(entity_id_t canvas, canvas_widget_handle_t widget, bool enabled);
		bool set_text(entity_id_t canvas, canvas_widget_handle_t widget, const char* text);
		bool set_frame_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_rect_paint_t& style);
		bool set_text_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_text_style_t& style);
		bool set_image(entity_id_t canvas, canvas_widget_handle_t widget, resource_handle_t texture, const vec4f_t& tint);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		ui::ui_context* get_ui_context(entity_id_t canvas);

	private:
		void						  create_canvas(entity_id_t entity, const component_canvas_t& component);
		void						  destroy_canvas(entity_id_t entity);
		bool						  map_position(const component_system_canvas_t& runtime, const vec2f_t& position, vec2f_t& mapped, f32& distance) const;
		component_system_canvas_t*	  route_pointer(const vec2f_t& position);
		void						  append_runtime(const component_system_canvas_t& runtime, world_canvas_draw_snapshot_t& destination) const;
		static bool					  is_recreation_required(const component_canvas_t& config, const component_canvas_t& other);
		static canvas_widget_handle_t make_handle(const component_system_canvas_t& runtime, u16 id);
		static u16					  resolve_handle(const component_system_canvas_t& runtime, canvas_widget_handle_t handle);
		static void					  invalidate_recursive(component_system_canvas_t& runtime, u16 id);

		world_t* _world				= nullptr;
		u16		 _widget_generation = 0;
	};
}
