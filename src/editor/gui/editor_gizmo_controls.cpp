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

#include "editor_gizmo_controls.hpp"
#include "editor/editor.hpp"
#include "editor/gui/editor_gui_controller.hpp"
#include "editor/gui/editor_panel_entities.hpp"
#include "editor/editor_theme.hpp"
#include "app/app.hpp"

#include "input/input_mappings.hpp"
#include "math/math.hpp"
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"
#include "gui/vekt.hpp"

namespace SFG
{

#define UNIFORM_RECT_SIZE 16.0f
#define ROTATE_SEGMENTS	  48

	namespace
	{
		bool project_point_render_thread(world_screen& screen, const vector2& panel_pos, const vector2& panel_size, const vector3& world_pos, vector2& out)
		{
			vector2 screen_pos = vector2::zero;
			if (!screen.world_to_screen_render_thread(world_pos, screen_pos))
				return false;

			const vector2ui16& res = screen.get_world_resolution();
			if (res.x == 0 || res.y == 0)
				return false;

			const f32 scale_x = panel_size.x / static_cast<f32>(res.x);
			const f32 scale_y = panel_size.y / static_cast<f32>(res.y);

			out.x = panel_pos.x + screen_pos.x * scale_x;
			out.y = panel_pos.y + screen_pos.y * scale_y;
			return true;
		}

		vector3 axis_from_gizmo(gizmo_axis axis)
		{
			switch (axis)
			{
			case gizmo_axis::x:
				return vector3::right;
			case gizmo_axis::y:
				return vector3::up;
			case gizmo_axis::z:
				return vector3::forward;
			default:
				return vector3::zero;
			}
		}

		vector3 axis_from_gizmo(gizmo_axis axis, const quat& rot, gizmo_space space)
		{
			const vector3 axis_vec = axis_from_gizmo(axis);
			if (space == gizmo_space::local)
				return rot * axis_vec;
			return axis_vec;
		}

		vector4 color_from_axis(gizmo_axis axis, gizmo_axis active_axis)
		{
			switch (axis)
			{
			case gizmo_axis::x:
				return active_axis == gizmo_axis::x ? editor_theme::get().col_accent : editor_theme::get().color_axis_x;
			case gizmo_axis::y:
				return active_axis == gizmo_axis::y ? editor_theme::get().col_accent : editor_theme::get().color_axis_y;
			case gizmo_axis::z:
				return active_axis == gizmo_axis::z ? editor_theme::get().col_accent : editor_theme::get().color_axis_z;
			default:
				return vector4::zero;
			}
		}

		f32 distance_point_segment(const vector2& p, const vector2& a, const vector2& b)
		{
			const vector2 ab		 = b - a;
			const f32	  ab_len_sqr = ab.magnitude_sqr();
			if (ab_len_sqr < MATH_EPS)
				return vector2::distance(p, a);

			const f32	  t		  = math::clamp(vector2::dot(p - a, ab) / ab_len_sqr, 0.0f, 1.0f);
			const vector2 closest = a + ab * t;
			return vector2::distance(p, closest);
		}

		void draw_move_gizmo(vekt::builder* builder, const gizmo_draw_context& ctx, gizmo_space space, gizmo_axis active_axis)
		{
			const f32 dist	  = vector3::distance(ctx.entity_pos, ctx.cam_pos);
			const f32 gizmo_len = math::max(0.5f, dist * 0.1f);

			struct axis_draw
			{
				gizmo_axis id;
				vector3	   axis;
				vector4	   color;
			};

			const axis_draw axes[] = {
				{gizmo_axis::x, axis_from_gizmo(gizmo_axis::x, ctx.entity_rot, space), color_from_axis(gizmo_axis::x, active_axis)},
				{gizmo_axis::y, axis_from_gizmo(gizmo_axis::y, ctx.entity_rot, space), color_from_axis(gizmo_axis::y, active_axis)},
				{gizmo_axis::z, axis_from_gizmo(gizmo_axis::z, ctx.entity_rot, space), color_from_axis(gizmo_axis::z, active_axis)},
			};

			for (const axis_draw& axis : axes)
			{
				vector2 end_screen = vector2::zero;
				if (!project_point_render_thread(*ctx.screen, ctx.root_pos, ctx.root_size, ctx.entity_pos + axis.axis * gizmo_len, end_screen))
					continue;

				builder->add_line_aa({
					.p0			  = ctx.center_screen,
					.p1			  = end_screen,
					.color		  = axis.color,
					.thickness	  = 2.0f,
					.aa_thickness = 2,
					.draw_order	  = 5,
				});

				// arrows
				{
					const vector2 axis_dir	  = (end_screen - ctx.center_screen).normalized();
					const vector2 axis_ortho  = vector2(-axis_dir.y, axis_dir.x);
					const f32	  arrow_len	  = math::clamp(gizmo_len * 10.0f, 6.0f, 14.0f);
					const f32	  arrow_width = arrow_len * 0.6f;
					const vector2 arrow_base  = end_screen - axis_dir * arrow_len;
					const vector2 arrow_left  = arrow_base + axis_ortho * (arrow_width * 0.5f);
					const vector2 arrow_right = arrow_base - axis_ortho * (arrow_width * 0.5f);

					builder->add_line_aa({
						.p0			  = end_screen,
						.p1			  = arrow_left,
						.color		  = axis.color,
						.thickness	  = 2.0f,
						.aa_thickness = 2,
						.draw_order	  = 6,
					});

					builder->add_line_aa({
						.p0			  = end_screen,
						.p1			  = arrow_right,
						.color		  = axis.color,
						.thickness	  = 2.0f,
						.aa_thickness = 2,
						.draw_order	  = 6,
					});
				}

				// uniform move
				{
					const vector2		   half		  = vector2(UNIFORM_RECT_SIZE * 0.5f, UNIFORM_RECT_SIZE * 0.5f);
					const vector2		   rect_min	  = ctx.center_screen - half;
					const vector2		   rect_max	  = ctx.center_screen + half;
					const vector4		   rect_color = active_axis == gizmo_axis::uniform ? editor_theme::get().col_accent : editor_theme::get().col_text;
					const vekt::widget_gfx gfx		  = {
							   .color	   = rect_color,
							   .draw_order = 6,
							   .flags	   = vekt::gfx_flags::gfx_is_rect,
					   };

					builder->add_filled_rect({
						.gfx			 = gfx,
						.min			 = rect_min,
						.max			 = rect_max,
						.color_start	 = rect_color,
						.color_end		 = rect_color,
						.color_direction = vekt::direction::horizontal,
						.widget_id		 = 0,
						.multi_color	 = false,
					});
				}
			}
		}

		void draw_rotate_gizmo(vekt::builder* builder, const gizmo_draw_context& ctx, gizmo_space space, gizmo_axis active_axis)
		{
			const f32 dist	  = vector3::distance(ctx.entity_pos, ctx.cam_pos);
			const f32 gizmo_len = math::max(0.5f, dist * 0.1f);

			struct axis_circle
			{
				gizmo_axis id;
				vector3	   basis0;
				vector3	   basis1;
				vector4	   color;
			};

			vector3 x_basis0 = vector3::up;
			vector3 x_basis1 = vector3::forward;
			vector3 y_basis0 = vector3::right;
			vector3 y_basis1 = vector3::forward;
			vector3 z_basis0 = vector3::right;
			vector3 z_basis1 = vector3::up;

			if (space == gizmo_space::local)
			{
				x_basis0 = ctx.entity_rot * x_basis0;
				x_basis1 = ctx.entity_rot * x_basis1;
				y_basis0 = ctx.entity_rot * y_basis0;
				y_basis1 = ctx.entity_rot * y_basis1;
				z_basis0 = ctx.entity_rot * z_basis0;
				z_basis1 = ctx.entity_rot * z_basis1;
			}

			const axis_circle circles[] = {
				{gizmo_axis::x, x_basis0, x_basis1, color_from_axis(gizmo_axis::x, active_axis)},
				{gizmo_axis::y, y_basis0, y_basis1, color_from_axis(gizmo_axis::y, active_axis)},
				{gizmo_axis::z, z_basis0, z_basis1, color_from_axis(gizmo_axis::z, active_axis)},
			};

			const unsigned int segments = ROTATE_SEGMENTS;
			const f32		   radius	= gizmo_len * 0.75f;

			for (const axis_circle& circle : circles)
			{
				vector2 prev	 = vector2::zero;
				bool	has_prev = false;
				for (unsigned int i = 0; i <= segments; ++i)
				{
					const f32	  t			= static_cast<f32>(i) / static_cast<f32>(segments);
					const f32	  ang		= t * 2.0f * MATH_PI;
					const f32	  c			= math::cos(ang);
					const f32	  s			= math::sin(ang);
					const vector3 world_pos = ctx.entity_pos + (circle.basis0 * c + circle.basis1 * s) * radius;
					vector2		  cur		= vector2::zero;
					if (!project_point_render_thread(*ctx.screen, ctx.root_pos, ctx.root_size, world_pos, cur))
					{
						has_prev = false;
						continue;
					}

					if (has_prev)
					{
						builder->add_line_aa({
							.p0			  = prev,
							.p1			  = cur,
							.color		  = circle.color,
							.thickness	  = 2.0f,
							.aa_thickness = 2,
							.draw_order	  = 5,
						});
					}
					prev	 = cur;
					has_prev = true;
				}
			}
		}

		void draw_scale_gizmo(vekt::builder* builder, const gizmo_draw_context& ctx, gizmo_space space, gizmo_axis active_axis)
		{
			const f32 dist	  = vector3::distance(ctx.entity_pos, ctx.cam_pos);
			const f32 gizmo_len = math::max(0.5f, dist * 0.1f);

			struct axis_draw
			{
				gizmo_axis id;
				vector3	   axis;
				vector4	   color;
			};

			const axis_draw axes[] = {
				{gizmo_axis::x, axis_from_gizmo(gizmo_axis::x, ctx.entity_rot, space), color_from_axis(gizmo_axis::x, active_axis)},
				{gizmo_axis::y, axis_from_gizmo(gizmo_axis::y, ctx.entity_rot, space), color_from_axis(gizmo_axis::y, active_axis)},
				{gizmo_axis::z, axis_from_gizmo(gizmo_axis::z, ctx.entity_rot, space), color_from_axis(gizmo_axis::z, active_axis)},
			};

			for (const axis_draw& axis : axes)
			{
				vector2 end_screen = vector2::zero;
				if (!project_point_render_thread(*ctx.screen, ctx.root_pos, ctx.root_size, ctx.entity_pos + axis.axis * gizmo_len, end_screen))
					continue;

				builder->add_line_aa({
					.p0			  = ctx.center_screen,
					.p1			  = end_screen,
					.color		  = axis.color,
					.thickness	  = 2.0f,
					.aa_thickness = 2,
					.draw_order	  = 5,
				});

				const f32			   cap_size = math::clamp(gizmo_len * 10.0f, 6.0f, 12.0f);
				const vector2		   cap_half = vector2(cap_size * 0.5f, cap_size * 0.5f);
				const vector2		   cap_min	= end_screen - cap_half;
				const vector2		   cap_max	= end_screen + cap_half;
				const vekt::widget_gfx gfx		= {
						 .color		 = axis.color,
						 .draw_order = 6,
						 .flags		 = vekt::gfx_flags::gfx_is_rect,
				 };

				builder->add_filled_rect({
					.gfx			 = gfx,
					.min			 = cap_min,
					.max			 = cap_max,
					.color_start	 = axis.color,
					.color_end		 = axis.color,
					.color_direction = vekt::direction::horizontal,
					.widget_id		 = 0,
					.multi_color	 = false,
				});
			}

			// uniform scale
			{
				const vector2		   half		  = vector2(UNIFORM_RECT_SIZE * 0.5f, UNIFORM_RECT_SIZE * 0.5f);
				const vector2		   rect_min	  = ctx.center_screen - half;
				const vector2		   rect_max	  = ctx.center_screen + half;
				const vector4		   rect_color = active_axis == gizmo_axis::uniform ? editor_theme::get().col_accent : editor_theme::get().col_text;
				const vekt::widget_gfx gfx		  = {
						   .color	   = rect_color,
						   .draw_order = 6,
						   .flags	   = vekt::gfx_flags::gfx_is_rect,
				   };

				builder->add_filled_rect({
					.gfx			 = gfx,
					.min			 = rect_min,
					.max			 = rect_max,
					.color_start	 = rect_color,
					.color_end		 = rect_color,
					.color_direction = vekt::direction::horizontal,
					.widget_id		 = 0,
					.multi_color	 = false,
				});
			}
		}
	}

	void editor_gizmo_controls::init(vekt::builder* builder)
	{
		_builder = builder;
	}

	void editor_gizmo_controls::draw(const vector2& root_pos, const vector2& root_size, const vector2& game_render_size)
	{
		_last_root_pos		   = root_pos;
		_last_root_size		   = root_size;
		_last_game_render_size = game_render_size;

		gizmo_draw_context ctx = {};
		if (!get_context_selected(ctx))
			return;

		gizmo_axis active_axis = _active_axis;

		if (active_axis == gizmo_axis::none)
			active_axis = _hovered_axis;

		switch (_style)
		{
		case gizmo_style::move:
			draw_move_gizmo(_builder, ctx, _space, active_axis);
			break;
		case gizmo_style::rotate:
			draw_rotate_gizmo(_builder, ctx, _space, active_axis);
			break;
		case gizmo_style::scale:
			draw_scale_gizmo(_builder, ctx, _space, active_axis);
			break;
		default:
			break;
		}

		entity_manager& em = editor::get().get_app().get_world().get_entity_manager();

		const char* name = em.get_entity_meta(ctx.selected).name;
		if (name && name[0] != '\0')
		{
			vekt::text_props tp = {};
			tp.text				= name;
			tp.font				= editor_theme::get().font_default;

			const vector2		   text_size = vekt::builder::get_text_size(tp);
			const vector2		   padding	 = vector2(6.0f, 4.0f);
			const vector2		   text_pos	 = vector2(ctx.center_screen.x - text_size.x * 0.5f, ctx.center_screen.y + 28.0f);
			const vector2		   rect_min	 = text_pos - padding;
			const vector2		   rect_max	 = text_pos + text_size + padding;
			const vekt::widget_gfx gfx		 = {
					  .color	  = editor_theme::get().col_frame_bg,
					  .draw_order = 6,
					  .flags	  = vekt::gfx_flags::gfx_is_rect,
			  };

			_builder->add_filled_rect({
				.gfx			 = gfx,
				.min			 = rect_min,
				.max			 = rect_max,
				.color_start	 = editor_theme::get().col_frame_bg,
				.color_end		 = editor_theme::get().col_frame_bg,
				.color_direction = vekt::direction::horizontal,
				.widget_id		 = 0,
				.multi_color	 = false,
			});

			_builder->add_text(tp, editor_theme::get().col_text, text_pos, text_size, 7, nullptr);
		}
	}

	bool editor_gizmo_controls::on_mouse_event(const window_event& ev)
	{
		if (ev.button != input_code::mouse_0)
			return false;

		if (ev.sub_type == window_event_sub_type::release && _active_axis != gizmo_axis::none)
		{
			set_active_axis(gizmo_axis::none);
			_drag_amount = 0.0f;
			_drag_offset = vector3::zero;
			return true;
		}

		// start dragging a gizmo, store relative data
		if (ev.sub_type == window_event_sub_type::press && _hovered_axis != gizmo_axis::none)
		{
			editor_gui_controller& cont		= editor::get().get_gui_controller();
			editor_panel_entities* entities = cont.get_entities();

			const world_handle selected = entities->get_selected();
			if (selected.is_null())
				return false;

			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();

			_active_axis	  = _hovered_axis;
			_hovered_axis	  = gizmo_axis::none;
			_drag_last_mouse  = vector2(static_cast<f32>(ev.value.x), static_cast<f32>(ev.value.y));
			_drag_amount	  = 0.0f;
			_drag_offset	  = vector3::zero;
			_drag_start_pos	  = em.get_entity_position_abs(selected);
			_drag_start_rot	  = em.get_entity_rotation_abs(selected);
			_drag_start_scale = em.get_entity_scale_abs(selected);
			return true;
		}

		return false;
	}

	bool editor_gizmo_controls::on_mouse_move(const vector2& pos)
	{
		// we are moving a gizmo
		if (_active_axis != gizmo_axis::none)
		{
			const vector2 delta = pos - _drag_last_mouse;
			if (delta.is_zero())
				return true;

			if (_last_root_size.x <= 0.0f || _last_root_size.y <= 0.0f)
				return false;

			gizmo_draw_context ctx = {};
			if (!get_context_selected(ctx))
				return false;

			const f32		dist	  = vector3::distance(ctx.entity_pos, ctx.cam_pos);
			const f32		gizmo_len = math::max(0.5f, dist * 0.1f);
			entity_manager& em		  = editor::get().get_app().get_world().get_entity_manager();

			vector3 axis_world		= vector3::zero;
			vector2 axis_dir_screen = vector2::zero;
			f32	axis_pixels		= 0.0f;

			if (_active_axis != gizmo_axis::uniform && _style != gizmo_style::rotate)
			{
				axis_world				= axis_from_gizmo(_active_axis, _drag_start_rot, _space);
				vector2 axis_end_screen = vector2::zero;
				if (!project_point_render_thread(*ctx.screen, _last_root_pos, _last_root_size, ctx.entity_pos + axis_world * gizmo_len, axis_end_screen))
					return false;

				axis_dir_screen		 = axis_end_screen - ctx.center_screen;
				const f32 axis_len = axis_dir_screen.magnitude();
				if (axis_len < MATH_EPS)
					return false;
				axis_dir_screen /= axis_len;
				axis_pixels = vector2::dot(delta, axis_dir_screen);
			}

			if (_style == gizmo_style::move)
			{
				const vector4 view_pos		  = ctx.view * vector4(ctx.entity_pos.x, ctx.entity_pos.y, ctx.entity_pos.z, 1.0f);
				const f32	  z_dist		  = math::abs(view_pos.z);
				const f32	  fov_rad		  = math::degrees_to_radians(ctx.fov);
				const f32	  world_per_pixel = (2.0f * math::tan(0.5f * fov_rad) * z_dist) / _last_root_size.y;

				if (_active_axis == gizmo_axis::uniform)
				{
					const vector3 cam_right = ctx.cam_rot.get_right();
					const vector3 cam_up	= ctx.cam_rot.get_up();
					_drag_offset += (cam_right * (delta.x * world_per_pixel)) + (cam_up * (-delta.y * world_per_pixel));
					const vector3 new_pos = _drag_start_pos + _drag_offset;
					em.set_entity_position_abs(ctx.selected, new_pos);
					_drag_last_mouse = pos;
					return true;
				}

				_drag_amount += axis_pixels * world_per_pixel;
				const vector3 new_pos = _drag_start_pos + axis_world * _drag_amount;
				em.set_entity_position_abs(ctx.selected, new_pos);
			}
			else if (_style == gizmo_style::rotate)
			{
				if (_active_axis == gizmo_axis::uniform)
					return false;

				axis_world				= axis_from_gizmo(_active_axis, _drag_start_rot, _space);
				vector2 axis_end_screen = vector2::zero;
				if (!project_point_render_thread(*ctx.screen, _last_root_pos, _last_root_size, ctx.entity_pos + axis_world * gizmo_len, axis_end_screen))
					return false;

				axis_dir_screen		 = axis_end_screen - ctx.center_screen;
				const f32 axis_len = axis_dir_screen.magnitude();
				if (axis_len < MATH_EPS)
					return false;
				axis_dir_screen /= axis_len;

				const vector2 tangent	   = vector2(-axis_dir_screen.y, axis_dir_screen.x);
				const f32	  angle_pixels = vector2::dot(delta, tangent);
				_drag_amount += angle_pixels * 0.4f;
				const quat rot_delta = quat::angle_axis(_drag_amount, axis_world);
				em.set_entity_rotation_abs(ctx.selected, rot_delta * _drag_start_rot);
			}
			else if (_style == gizmo_style::scale)
			{
				if (_active_axis == gizmo_axis::uniform)
				{
					_drag_amount += (-delta.y) * 0.01f;
					const f32 uniform_factor = math::max(0.01f, 1.0f + _drag_amount);
					vector3		new_scale	   = _drag_start_scale * uniform_factor;
					new_scale.x				   = math::max(0.01f, new_scale.x);
					new_scale.y				   = math::max(0.01f, new_scale.y);
					new_scale.z				   = math::max(0.01f, new_scale.z);
					em.set_entity_scale_abs(ctx.selected, new_scale);
					_drag_last_mouse = pos;
					return true;
				}

				_drag_amount += axis_pixels * 0.01f;
				vector3 new_scale = _drag_start_scale;
				if (_active_axis == gizmo_axis::x)
					new_scale.x = math::max(0.01f, _drag_start_scale.x + _drag_amount);
				else if (_active_axis == gizmo_axis::y)
					new_scale.y = math::max(0.01f, _drag_start_scale.y + _drag_amount);
				else if (_active_axis == gizmo_axis::z)
					new_scale.z = math::max(0.01f, _drag_start_scale.z + _drag_amount);
				em.set_entity_scale_abs(ctx.selected, new_scale);
			}

			_drag_last_mouse = pos;
			return true;
		}

		if (_last_root_size.x <= 0.0f || _last_root_size.y <= 0.0f)
		{
			_hovered_axis = gizmo_axis::none;
			return false;
		}

		if (pos.x < _last_root_pos.x || pos.y < _last_root_pos.y || pos.x > _last_root_pos.x + _last_root_size.x || pos.y > _last_root_pos.y + _last_root_size.y)
		{
			_hovered_axis = gizmo_axis::none;
			return false;
		}

		editor_gui_controller& cont		= editor::get().get_gui_controller();
		editor_panel_entities* entities = cont.get_entities();

		const world_handle selected = entities->get_selected();
		if (selected.is_null())
		{
			_hovered_axis = gizmo_axis::none;
			return false;
		}

		world&			w  = editor::get().get_app().get_world();
		entity_manager& em = w.get_entity_manager();

		const world_handle main_cam_entity = em.get_main_camera_entity();
		if (main_cam_entity.is_null())
		{
			_hovered_axis = gizmo_axis::none;
			return false;
		}

		const vector3 entity_pos	= em.get_entity_position_abs(selected);
		const quat	  entity_rot	= em.get_entity_rotation_abs(selected);
		world_screen& screen		= w.get_screen();
		vector2		  center_screen = vector2::zero;
		f32		  dist			= 0.0f;
		vector2		  screen_pos	= vector2::zero;
		if (!screen.world_to_screen_render_thread(entity_pos, screen_pos, dist))
		{
			_hovered_axis = gizmo_axis::none;
			return false;
		}

		const vector2ui16& res = screen.get_world_resolution();
		if (res.x == 0 || res.y == 0)
		{
			_hovered_axis = gizmo_axis::none;
			return false;
		}

		const f32 scale_x = _last_root_size.x / static_cast<f32>(res.x);
		const f32 scale_y = _last_root_size.y / static_cast<f32>(res.y);
		center_screen		  = vector2(_last_root_pos.x + screen_pos.x * scale_x, _last_root_pos.y + screen_pos.y * scale_y);

		const f32 gizmo_len = math::max(0.5f, dist * 0.1f);
		const f32 threshold = 8.0f;

		gizmo_axis hovered	 = gizmo_axis::none;
		f32	   best_dist = threshold;

		if (_style == gizmo_style::move || _style == gizmo_style::scale)
		{
			const vector2 half	   = vector2(UNIFORM_RECT_SIZE * 0.5f, UNIFORM_RECT_SIZE * 0.5f);
			const vector2 rect_min = center_screen - half;
			const vector2 rect_max = center_screen + half;
			if (pos.x >= rect_min.x && pos.x <= rect_max.x && pos.y >= rect_min.y && pos.y <= rect_max.y)
			{
				_hovered_axis = gizmo_axis::uniform;
				return true;
			}

			const gizmo_axis axes[] = {gizmo_axis::x, gizmo_axis::y, gizmo_axis::z};
			for (gizmo_axis axis : axes)
			{
				const vector3 axis_world = axis_from_gizmo(axis, entity_rot, _space);
				vector2		  end_screen = vector2::zero;
				if (!project_point_render_thread(screen, _last_root_pos, _last_root_size, entity_pos + axis_world * gizmo_len, end_screen))
					continue;

				const f32 d = distance_point_segment(pos, center_screen, end_screen);
				if (d < best_dist)
				{
					best_dist = d;
					hovered	  = axis;
				}
			}
		}
		else if (_style == gizmo_style::rotate)
		{
			struct axis_circle
			{
				gizmo_axis id;
				vector3	   basis0;
				vector3	   basis1;
			};

			vector3 x_basis0 = vector3::up;
			vector3 x_basis1 = vector3::forward;
			vector3 y_basis0 = vector3::right;
			vector3 y_basis1 = vector3::forward;
			vector3 z_basis0 = vector3::right;
			vector3 z_basis1 = vector3::up;

			if (_space == gizmo_space::local)
			{
				x_basis0 = entity_rot * x_basis0;
				x_basis1 = entity_rot * x_basis1;
				y_basis0 = entity_rot * y_basis0;
				y_basis1 = entity_rot * y_basis1;
				z_basis0 = entity_rot * z_basis0;
				z_basis1 = entity_rot * z_basis1;
			}

			const axis_circle circles[] = {
				{gizmo_axis::x, x_basis0, x_basis1},
				{gizmo_axis::y, y_basis0, y_basis1},
				{gizmo_axis::z, z_basis0, z_basis1},
			};

			const unsigned int segments = ROTATE_SEGMENTS;
			const f32		   radius	= gizmo_len * 0.75f;

			for (const axis_circle& circle : circles)
			{
				vector2 prev	 = vector2::zero;
				bool	has_prev = false;
				f32	min_dist = threshold;

				for (unsigned int i = 0; i <= segments; ++i)
				{
					const f32	  t			= static_cast<f32>(i) / static_cast<f32>(segments);
					const f32	  ang		= t * 2.0f * MATH_PI;
					const f32	  c			= math::cos(ang);
					const f32	  s			= math::sin(ang);
					const vector3 world_pos = entity_pos + (circle.basis0 * c + circle.basis1 * s) * radius;
					vector2		  cur		= vector2::zero;
					if (!project_point_render_thread(screen, _last_root_pos, _last_root_size, world_pos, cur))
					{
						has_prev = false;
						continue;
					}

					if (has_prev)
					{
						const f32 d = distance_point_segment(pos, prev, cur);
						if (d < min_dist)
							min_dist = d;
					}
					prev	 = cur;
					has_prev = true;
				}

				if (min_dist < best_dist)
				{
					best_dist = min_dist;
					hovered	  = circle.id;
				}
			}
		}

		_hovered_axis = hovered;
		return hovered != gizmo_axis::none;
	}

	bool editor_gizmo_controls::get_context_selected(gizmo_draw_context& out)
	{
		editor_gui_controller& cont		= editor::get().get_gui_controller();
		editor_panel_entities* entities = cont.get_entities();

		const world_handle selected = entities->get_selected();
		if (selected.is_null())
			return false;

		world&			w  = editor::get().get_app().get_world();
		entity_manager& em = w.get_entity_manager();

		const world_handle main_cam_entity = em.get_main_camera_entity();
		const world_handle main_cam_comp   = em.get_main_camera_comp();
		if (main_cam_entity.is_null() || main_cam_comp.is_null())
			return false;

		component_manager& cm		   = w.get_comp_manager();
		comp_camera&	   camera_comp = cm.get_component<comp_camera>(main_cam_comp);

		const matrix4x4 view = matrix4x4::view(em.get_entity_rotation_abs(main_cam_entity), em.get_entity_position_abs(main_cam_entity));

		const vector3 entity_pos   = em.get_entity_position_abs(selected);
		vector2		  gizmo_screen = vector2::zero;
		world_screen& screen = w.get_screen();
		if (!project_point_render_thread(screen, _last_root_pos, _last_root_size, entity_pos, gizmo_screen))
			return false;

		out = {
			.root_pos	   = _last_root_pos,
			.root_size	   = _last_root_size,
			.view		   = view,
			.center_screen = gizmo_screen,
			.entity_pos	   = entity_pos,
			.entity_rot	   = em.get_entity_rotation_abs(selected),
			.cam_rot	   = em.get_entity_rotation_abs(main_cam_entity),
			.cam_pos	   = em.get_entity_position_abs(main_cam_entity),
			.fov		   = camera_comp.get_fov_degrees(),
			.selected	   = selected,
			.screen		   = &screen,
		};

		return true;
	}
}
