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

#include "world/editor_world_gizmo.hpp"
#include <sfg/data/char_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/render/render_view.hpp>
#include <sfg/runtime/render/world_render_view.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/runtime/world/world_util.hpp>

namespace sfg
{
#define EDITOR_WORLD_GIZMO_ROTATION_SEGMENTS	   64
#define EDITOR_WORLD_GIZMO_SCALE_MIN			   0.001f
#define EDITOR_WORLD_GIZMO_UNIFORM_SCALE_PER_PIXEL 0.0025f
#define EDITOR_WORLD_GIZMO_AXIS_PARALLEL_EPS	   0.01f

	static constexpr u32 GIZMO_PLANE_AXES[3][3] = {
		{0, 1, 2},
		{1, 2, 0},
		{2, 0, 1},
	};

	struct editor_world_gizmo_t::frame_t
	{
		render_view_t view				= {};
		quat_t		  axis_rotations[3] = {};
		vec3f_t		  axes[3]			= {};
		vec3f_t		  ring_u[3]			= {};
		vec3f_t		  ring_v[3]			= {};
		vec3f_t		  camera_forward	= vec3f_t::zero;
		vec3f_t		  camera_right		= vec3f_t::zero;
		vec3f_t		  camera_up			= vec3f_t::zero;
		vec3f_t		  pivot				= vec3f_t::zero;
		vec2u16_t	  resolution		= vec2u16_t::zero;
		f32			  world_scale		= 0.0f;
	};

	struct editor_world_gizmo_t::hit_t
	{
		vec3f_t				rotation_direction = vec3f_t::zero;
		vec2f_t				rotation_tangent   = vec2f_t::zero;
		editor_gizmo_axis_e axis			   = editor_gizmo_axis_e::invalid;
	};

	void editor_world_gizmo_t::uninit()
	{
		cancel_action();
		clear_hover();
	}

	bool editor_world_gizmo_t::calculate_frame(const editor_gizmo_input_t& input, frame_t& out_frame) const
	{
		const world_render_view_t& camera	  = *input.view;
		const vec2u16_t			   resolution = input.resolution;

		if (input.control_type == editor_transform_control_type_e::invalid || resolution.x == 0 || resolution.y == 0)
			return false;

		out_frame.view.calculate(camera, resolution, 1.0f);
		out_frame.camera_forward = camera.rot.get_forward();
		out_frame.camera_right	 = camera.rot.get_right();
		out_frame.camera_up		 = camera.rot.get_up();
		out_frame.pivot			 = input.target.position;

		const quat_t orientation	= input.locality == editor_transform_locality_e::local ? input.target.rotation : quat_t::identity;
		const quat_t axis_models[3] = {
			quat_t::angle_axis(-90.0f, {0.0f, 0.0f, 1.0f}),
			quat_t::identity,
			quat_t::angle_axis(-90.0f, vec3f_t::right),
		};

		for (u32 i = 0; i < 3; ++i)
		{
			out_frame.axis_rotations[i] = orientation * axis_models[i];
			out_frame.axes[i]			= out_frame.axis_rotations[i].get_up();
			out_frame.ring_u[i]			= out_frame.axis_rotations[i].get_right();
			out_frame.ring_v[i]			= -out_frame.axis_rotations[i].get_forward();
		}

		const vec4f_t pivot_clip = out_frame.view.view_proj * vec4f_t(out_frame.pivot.x, out_frame.pivot.y, out_frame.pivot.z, 1.0f);

		if (pivot_clip.w <= MATH_EPS)
			return false;

		out_frame.resolution  = resolution;
		out_frame.world_scale = math::max(PIXEL_SIZE * 2.0f * pivot_clip.w * math::tan(camera.fov_degrees * DEG_2_RAD * 0.5f) / static_cast<f32>(resolution.y), MIN_WORLD_SIZE);
		return true;
	}

	bool editor_world_gizmo_t::project_point(const frame_t& frame, const vec3f_t& point, vec2f_t& out_position) const
	{
		if (!world_util_t::world_position_to_relative_position(frame.view.view_proj, point, out_position))
			return false;

		out_position.x *= static_cast<f32>(frame.resolution.x);
		out_position.y *= static_cast<f32>(frame.resolution.y);
		return true;
	}

	editor_world_gizmo_t::hit_t editor_world_gizmo_t::pick(const frame_t& frame, editor_transform_control_type_e control_type, vec2f_t relative_position) const
	{
		const vec2f_t mouse = {
			relative_position.x * static_cast<f32>(frame.resolution.x),
			relative_position.y * static_cast<f32>(frame.resolution.y),
		};
		const f32 hit_radius_sqr   = HIT_RADIUS_PX * HIT_RADIUS_PX;
		f32		  closest_distance = hit_radius_sqr;
		hit_t	  result		   = {};

		if (control_type == editor_transform_control_type_e::rotate)
		{
			for (u32 axis = 0; axis < 3; ++axis)
			{
				vec2f_t previous	   = vec2f_t::zero;
				bool	previous_valid = project_point(frame, frame.pivot + frame.ring_u[axis] * frame.world_scale, previous);

				for (u32 segment = 1; segment <= EDITOR_WORLD_GIZMO_ROTATION_SEGMENTS; ++segment)
				{
					const f32  angle		 = static_cast<f32>(segment) / static_cast<f32>(EDITOR_WORLD_GIZMO_ROTATION_SEGMENTS) * MATH_TWO_PI;
					vec2f_t	   current		 = vec2f_t::zero;
					const bool current_valid = project_point(frame, frame.pivot + (frame.ring_u[axis] * math::cos(angle) + frame.ring_v[axis] * math::sin(angle)) * frame.world_scale, current);

					if (previous_valid && current_valid)
					{
						const f32 distance = vec2f_t::distance_sqr_to_segment(mouse, previous, current);

						if (distance < closest_distance)
						{
							closest_distance		  = distance;
							result.axis				  = static_cast<editor_gizmo_axis_e>(axis);
							result.rotation_direction = (frame.ring_u[axis] * math::cos(angle) + frame.ring_v[axis] * math::sin(angle)).normalized();
							result.rotation_tangent	  = (current - previous).normalized();
						}
					}

					previous	   = current;
					previous_valid = current_valid;
				}
			}

			return result;
		}

		vec2f_t pivot_pixels = vec2f_t::zero;

		if (!project_point(frame, frame.pivot, pivot_pixels))
			return result;

		if (control_type == editor_transform_control_type_e::move || control_type == editor_transform_control_type_e::scale)
		{
			const f32 central_hit_radius_sqr = CENTRAL_HIT_RADIUS_PX * CENTRAL_HIT_RADIUS_PX;

			if ((mouse - pivot_pixels).magnitude_sqr() <= central_hit_radius_sqr)
			{
				result.axis = editor_gizmo_axis_e::central;
				return result;
			}
		}

		if (control_type == editor_transform_control_type_e::move)
		{
			world_ray_t ray = {};

			if (world_util_t::relative_position_to_world_ray(frame.view.inv_view_proj, relative_position, ray))
			{
				f32 best_alignment = 0.0f;

				for (u32 plane = 0; plane < 3; ++plane)
				{
					const vec3f_t& axis_a	   = frame.axes[GIZMO_PLANE_AXES[plane][0]];
					const vec3f_t& axis_b	   = frame.axes[GIZMO_PLANE_AXES[plane][1]];
					const vec3f_t& normal	   = frame.axes[GIZMO_PLANE_AXES[plane][2]];
					vec3f_t		   plane_point = vec3f_t::zero;

					if (!calculate_plane_point(ray, frame.pivot, normal, plane_point))
						continue;

					const vec3f_t offset	   = (plane_point - frame.pivot) / frame.world_scale;
					const f32	  coordinate_a = vec3f_t::dot(offset, axis_a);
					const f32	  coordinate_b = vec3f_t::dot(offset, axis_b);
					const f32	  plane_min	   = PLANE_CENTER - PLANE_SIZE * 0.5f;
					const f32	  plane_max	   = PLANE_CENTER + PLANE_SIZE * 0.5f;

					if (coordinate_a < plane_min || coordinate_a > plane_max || coordinate_b < plane_min || coordinate_b > plane_max)
						continue;

					const f32 alignment = math::abs(vec3f_t::dot(ray.direction, normal));

					if (alignment > best_alignment)
					{
						best_alignment = alignment;
						result.axis	   = static_cast<editor_gizmo_axis_e>(static_cast<u32>(editor_gizmo_axis_e::xy) + plane);
					}
				}

				if (result.axis != editor_gizmo_axis_e::invalid)
					return result;
			}
		}

		for (u32 axis = 0; axis < 3; ++axis)
		{
			vec2f_t endpoint_pixels = vec2f_t::zero;

			if (!project_point(frame, frame.pivot + frame.axes[axis] * frame.world_scale, endpoint_pixels))
				continue;

			const f32 distance = vec2f_t::distance_sqr_to_segment(mouse, pivot_pixels, endpoint_pixels);

			if (distance < closest_distance)
			{
				closest_distance = distance;
				result.axis		 = static_cast<editor_gizmo_axis_e>(axis);
			}
		}

		return result;
	}

	bool editor_world_gizmo_t::calculate_axis_parameter(const world_ray_t& ray, const vec3f_t& pivot, const vec3f_t& axis, f32& out_parameter) const
	{
		const f32 parallel = vec3f_t::dot(axis, ray.direction);
		const f32 denom	   = 1.0f - parallel * parallel;

		if (denom <= EDITOR_WORLD_GIZMO_AXIS_PARALLEL_EPS)
			return false;

		const vec3f_t offset = ray.origin - pivot;
		out_parameter		 = (vec3f_t::dot(axis, offset) - parallel * vec3f_t::dot(ray.direction, offset)) / denom;
		return true;
	}

	bool editor_world_gizmo_t::calculate_rotation_direction(const world_ray_t& ray, const vec3f_t& pivot, const vec3f_t& axis, vec3f_t& out_direction) const
	{
		const f32 denom = vec3f_t::dot(ray.direction, axis);

		if (math::abs(denom) <= EDITOR_WORLD_GIZMO_AXIS_PARALLEL_EPS)
			return false;

		const f32 distance = vec3f_t::dot(pivot - ray.origin, axis) / denom;
		out_direction	   = (ray.origin + ray.direction * distance - pivot).normalized();
		return !out_direction.is_zero();
	}

	bool editor_world_gizmo_t::calculate_plane_point(const world_ray_t& ray, const vec3f_t& pivot, const vec3f_t& normal, vec3f_t& out_point) const
	{
		const f32 denom = vec3f_t::dot(ray.direction, normal);

		if (math::abs(denom) <= EDITOR_WORLD_GIZMO_AXIS_PARALLEL_EPS)
			return false;

		const f32 distance = vec3f_t::dot(pivot - ray.origin, normal) / denom;
		out_point		   = ray.origin + ray.direction * distance;
		return true;
	}

	void editor_world_gizmo_t::update_hover(const editor_gizmo_input_t& input, vec2f_t relative_position)
	{
		if (_action_active)
			return;

		frame_t frame = {};
		_hovered_axis = calculate_frame(input, frame) ? pick(frame, input.control_type, relative_position).axis : editor_gizmo_axis_e::invalid;
	}

	void editor_world_gizmo_t::clear_hover()
	{
		if (!_action_active)
			_hovered_axis = editor_gizmo_axis_e::invalid;
	}

	bool editor_world_gizmo_t::begin_action(const editor_gizmo_input_t& input, const editor_gizmo_callbacks_t& callbacks, vec2f_t relative_position)
	{
		SFG_ASSERT(!_action_active);

		const vec2u16_t resolution = input.resolution;
		frame_t			frame	   = {};

		if (!calculate_frame(input, frame))
			return false;

		const hit_t hit = pick(frame, input.control_type, relative_position);

		if (hit.axis == editor_gizmo_axis_e::invalid)
			return false;

		const u32  handle_index = static_cast<u32>(hit.axis);
		const bool axis_handle	= handle_index < 3;
		const bool plane_handle = hit.axis >= editor_gizmo_axis_e::xy && hit.axis <= editor_gizmo_axis_e::zx;

		_active_axis	   = hit.axis;
		_hovered_axis	   = hit.axis;
		_control_type	   = input.control_type;
		_locality		   = input.locality;
		_target_generation = input.target.generation;
		_pivot			   = frame.pivot;
		_axis_world		   = axis_handle ? frame.axes[handle_index] : vec3f_t::zero;

		if (plane_handle)
		{
			const u32 plane		  = handle_index - static_cast<u32>(editor_gizmo_axis_e::xy);
			_central_plane_right  = frame.axes[GIZMO_PLANE_AXES[plane][0]];
			_central_plane_up	  = frame.axes[GIZMO_PLANE_AXES[plane][1]];
			_central_plane_normal = frame.axes[GIZMO_PLANE_AXES[plane][2]];
		}
		else
		{
			_central_plane_normal = frame.camera_forward;
			_central_plane_right  = frame.camera_right;
			_central_plane_up	  = frame.camera_up;
		}

		_world_scale		  = frame.world_scale;
		_orientation		  = _locality == editor_transform_locality_e::local ? frame.axis_rotations[1] : quat_t::identity;
		_initial_mouse_pixels = {
			relative_position.x * static_cast<f32>(resolution.x),
			relative_position.y * static_cast<f32>(resolution.y),
		};

		_initial_rotation_direction = hit.rotation_direction;
		_rotation_screen_tangent	= hit.rotation_tangent;

		_snap_translate = input.snap_translate;
		_snap_rotate	= input.snap_rotate;
		_snap_scale		= input.snap_scale;

		if (axis_handle)
		{
			vec2f_t pivot_pixels = vec2f_t::zero;
			project_point(frame, frame.pivot, pivot_pixels);

			vec2f_t endpoint_pixels = vec2f_t::zero;
			project_point(frame, frame.pivot + _axis_world * frame.world_scale, endpoint_pixels);
			_axis_screen_direction = (endpoint_pixels - pivot_pixels).normalized();
			_axis_pixels_per_world = vec2f_t::distance(pivot_pixels, endpoint_pixels) / frame.world_scale;
		}

		world_ray_t ray = {};

		if (world_util_t::relative_position_to_world_ray(frame.view.inv_view_proj, relative_position, ray))
		{
			if (!axis_handle)
				_central_plane_valid = calculate_plane_point(ray, _pivot, _central_plane_normal, _initial_plane_point);
			else
			{
				_axis_parameter_valid = calculate_axis_parameter(ray, _pivot, _axis_world, _initial_axis_parameter);
				_rotation_plane_valid = calculate_rotation_direction(ray, _pivot, _axis_world, _initial_rotation_direction);
			}
		}

		if (!callbacks.begin(callbacks.user_data))
		{
			clear_action();
			return false;
		}

		_callbacks	   = callbacks;
		_action_active = true;
		return true;
	}

	void editor_world_gizmo_t::update_action(const editor_gizmo_input_t& input, vec2f_t relative_position)
	{
		if (!_action_active)
			return;

		if (_target_generation != input.target.generation || _control_type != input.control_type || _locality != input.locality)
		{
			cancel_action();
			return;
		}

		frame_t frame = {};

		if (!calculate_frame(input, frame))
		{
			cancel_action();
			return;
		}

		const vec2u16_t resolution	 = input.resolution;
		const vec2f_t	mouse_pixels = {
			relative_position.x * static_cast<f32>(resolution.x),
			relative_position.y * static_cast<f32>(resolution.y),
		};

		const vec2f_t mouse_delta = mouse_pixels - _initial_mouse_pixels;
		world_ray_t	  ray		  = {};
		const bool	  ray_valid	  = world_util_t::relative_position_to_world_ray(frame.view.inv_view_proj, relative_position, ray);

		mat4x3_t delta = mat4x3_t::identity;

		switch (_control_type)
		{
		case editor_transform_control_type_e::move: {
			if (_active_axis == editor_gizmo_axis_e::central || (_active_axis >= editor_gizmo_axis_e::xy && _active_axis <= editor_gizmo_axis_e::zx))
			{
				vec3f_t current_plane_point = vec3f_t::zero;

				if (_central_plane_valid && ray_valid && calculate_plane_point(ray, _pivot, _central_plane_normal, current_plane_point))
				{
					vec3f_t translation = current_plane_point - _initial_plane_point;

					if (_snap_translate > 0.0f)
					{
						const f32 right = math::round(vec3f_t::dot(translation, _central_plane_right) / _snap_translate) * _snap_translate;
						const f32 up	= math::round(vec3f_t::dot(translation, _central_plane_up) / _snap_translate) * _snap_translate;
						translation		= _central_plane_right * right + _central_plane_up * up;
					}

					delta = mat4x3_t::translation(translation);
				}

				break;
			}

			f32 axis_delta			   = _axis_pixels_per_world > MATH_EPS ? vec2f_t::dot(mouse_delta, _axis_screen_direction) / _axis_pixels_per_world : 0.0f;
			f32 current_axis_parameter = 0.0f;

			if (_axis_parameter_valid && ray_valid && calculate_axis_parameter(ray, _pivot, _axis_world, current_axis_parameter))
				axis_delta = current_axis_parameter - _initial_axis_parameter;

			if (_snap_translate > 0.0f)
				axis_delta = math::round(axis_delta / _snap_translate) * _snap_translate;

			delta = mat4x3_t::translation(_axis_world * axis_delta);
			break;
		}
		case editor_transform_control_type_e::rotate: {
			f32		angle_degrees			   = vec2f_t::dot(mouse_delta, _rotation_screen_tangent) / PIXEL_SIZE * RAD_2_DEG;
			vec3f_t current_rotation_direction = vec3f_t::zero;

			if (_rotation_plane_valid && ray_valid && calculate_rotation_direction(ray, _pivot, _axis_world, current_rotation_direction))
			{
				const f32 dot_value = math::clamp(vec3f_t::dot(_initial_rotation_direction, current_rotation_direction), -1.0f, 1.0f);
				angle_degrees		= math::acos(dot_value) * RAD_2_DEG;

				if (vec3f_t::dot(_axis_world, vec3f_t::cross(_initial_rotation_direction, current_rotation_direction)) < 0.0f)
					angle_degrees = -angle_degrees;
			}

			if (_snap_rotate > 0.0f)
				angle_degrees = math::round(angle_degrees / _snap_rotate) * _snap_rotate;

			if (mouse_delta == vec2f_t::zero)
				angle_degrees = 0.0f;

			_rotation_angle_degrees		   = angle_degrees;
			const mat4x3_t pivot_to_origin = mat4x3_t::translation(-_pivot);
			const mat4x3_t origin_to_pivot = mat4x3_t::translation(_pivot);
			delta						   = origin_to_pivot * mat4x3_t::rotation(quat_t::angle_axis(angle_degrees, _axis_world)) * pivot_to_origin;
			break;
		}
		case editor_transform_control_type_e::scale: {
			if (_active_axis == editor_gizmo_axis_e::central)
			{
				f32 factor = 1.0f + mouse_delta.x * EDITOR_WORLD_GIZMO_UNIFORM_SCALE_PER_PIXEL;

				if (_snap_scale > 0.0f)
					factor = 1.0f + math::round((factor - 1.0f) / _snap_scale) * _snap_scale;

				factor						   = math::max(EDITOR_WORLD_GIZMO_SCALE_MIN, factor);
				const mat4x3_t pivot_to_origin = mat4x3_t::translation(-_pivot);
				const mat4x3_t origin_to_pivot = mat4x3_t::translation(_pivot);
				delta						   = origin_to_pivot * mat4x3_t::scale(vec3f_t(factor, factor, factor)) * pivot_to_origin;
				break;
			}

			f32 axis_delta			   = _axis_pixels_per_world > MATH_EPS ? vec2f_t::dot(mouse_delta, _axis_screen_direction) / _axis_pixels_per_world : 0.0f;
			f32 current_axis_parameter = 0.0f;

			if (_axis_parameter_valid && ray_valid && calculate_axis_parameter(ray, _pivot, _axis_world, current_axis_parameter))
				axis_delta = current_axis_parameter - _initial_axis_parameter;

			f32 factor = 1.0f + axis_delta / _world_scale;

			if (_snap_scale > 0.0f)
				factor = 1.0f + math::round((factor - 1.0f) / _snap_scale) * _snap_scale;

			factor		  = math::max(EDITOR_WORLD_GIZMO_SCALE_MIN, factor);
			vec3f_t scale = vec3f_t::one;

			switch (_active_axis)
			{
			case editor_gizmo_axis_e::x:
				scale.x = factor;
				break;
			case editor_gizmo_axis_e::y:
				scale.y = factor;
				break;
			case editor_gizmo_axis_e::z:
				scale.z = factor;
				break;
			default:
				SFG_ASSERT(false);
				break;
			}

			const mat4x3_t pivot_to_origin = mat4x3_t::translation(-_pivot);
			const mat4x3_t origin_to_pivot = mat4x3_t::translation(_pivot);
			const mat4x3_t basis		   = mat4x3_t::rotation(_orientation);
			delta						   = origin_to_pivot * basis * mat4x3_t::scale(scale) * basis.inverse() * pivot_to_origin;
			break;
		}
		default:
			SFG_ASSERT(false);
			break;
		}

		_callbacks.update(_callbacks.user_data, delta);
	}

	void editor_world_gizmo_t::draw_rotation_visualization(world_debug_draw_t& debug_draw, const color_t& line_color, const color_t& text_color, f32 text_size_px) const
	{
		if (!_action_active || _control_type != editor_transform_control_type_e::rotate)
			return;

		const vec3f_t current_direction = quat_t::angle_axis(_rotation_angle_degrees, _axis_world) * _initial_rotation_direction;
		const vec3f_t initial_endpoint	= _pivot + _initial_rotation_direction * _world_scale;
		const vec3f_t current_endpoint	= _pivot + current_direction * _world_scale;
		debug_draw.draw_line(_pivot, initial_endpoint, line_color, 2.0f, debug_draw_depth_e::always_visible);
		debug_draw.draw_line(_pivot, current_endpoint, line_color, 2.0f, debug_draw_depth_e::always_visible);

		char  rotation_text[32] = {};
		char* text_cur			= rotation_text;
		char* text_end			= rotation_text + sizeof(rotation_text);
		char_util::append_double(text_cur, text_end, _rotation_angle_degrees, 1);
		char_util::append(text_cur, text_end, " deg");
		debug_draw.draw_text_3d(current_endpoint, rotation_text, text_color, text_size_px, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_left, {6.0f, -6.0f});
	}

	void editor_world_gizmo_t::end_action(const editor_gizmo_input_t& input)
	{
		if (!_action_active)
			return;

		if (_target_generation != input.target.generation || _control_type != input.control_type || _locality != input.locality)
		{
			cancel_action();
			return;
		}

		const editor_gizmo_callbacks_t callbacks = _callbacks;
		clear_action();
		callbacks.commit(callbacks.user_data);
	}

	void editor_world_gizmo_t::cancel_action()
	{
		if (!_action_active)
			return;

		const editor_gizmo_callbacks_t callbacks = _callbacks;
		_hovered_axis							 = editor_gizmo_axis_e::invalid;
		clear_action();
		callbacks.cancel(callbacks.user_data);
	}

	void editor_world_gizmo_t::clear_action()
	{
		_callbacks = {};

		_orientation				= quat_t::identity;
		_initial_rotation_direction = vec3f_t::zero;
		_initial_plane_point		= vec3f_t::zero;
		_central_plane_normal		= vec3f_t::zero;
		_central_plane_right		= vec3f_t::zero;
		_central_plane_up			= vec3f_t::zero;
		_axis_world					= vec3f_t::zero;
		_pivot						= vec3f_t::zero;
		_initial_mouse_pixels		= vec2f_t::zero;
		_rotation_screen_tangent	= vec2f_t::zero;
		_axis_screen_direction		= vec2f_t::zero;

		_initial_axis_parameter = 0.0f;
		_axis_pixels_per_world	= 0.0f;
		_world_scale			= 0.0f;
		_rotation_angle_degrees = 0.0f;
		_snap_translate			= 0.0f;
		_snap_rotate			= 0.0f;
		_snap_scale				= 0.0f;

		_target_generation = 0;
		_control_type	   = editor_transform_control_type_e::invalid;
		_locality		   = editor_transform_locality_e::invalid;
		_active_axis	   = editor_gizmo_axis_e::invalid;

		_axis_parameter_valid = false;
		_rotation_plane_valid = false;
		_central_plane_valid  = false;
		_action_active		  = false;
	}
}
