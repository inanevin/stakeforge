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

#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/resources/vertex.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/world/world_debug_draw_config.hpp>

namespace sfg::ui
{
	class vg_canvas_t;
}

namespace sfg
{
	class color_t;
	class mat4x3_t;
	class world_t;
	struct aabb_t;
	struct world_debug_draw_snapshot_t;

	enum class debug_draw_depth_e : u8
	{
		depth_tested,
		always_visible,
	};

	enum class debug_draw_text_alignment_e : u8
	{
		top_left,
		top_center,
		top_right,
		center_left,
		center,
		center_right,
		bottom_left,
		bottom_center,
		bottom_right,
	};

	class world_debug_draw_t final
	{
	public:
		world_debug_draw_t();
		~world_debug_draw_t();
		world_debug_draw_t(const world_debug_draw_t&)			 = delete;
		world_debug_draw_t& operator=(const world_debug_draw_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_debug_draw_config_t& config);
		void uninit();
		void begin_frame();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void draw_line(const vec3f_t& from, const vec3f_t& to, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_arrow(const vec3f_t& from, const vec3f_t& to, const color_t& color, f32 head_length = 0.15f, f32 head_radius = 0.06f, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_triangle(const vec3f_t& p0, const vec3f_t& p1, const vec3f_t& p2, const color_t& color);
		void draw_polyline(span_t<const vec3f_t> points, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, bool closed = false);
		void draw_aabb(const aabb_t& bounds, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_box(const mat4x3_t& transform, const vec3f_t& half_extents, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_rectangle(const vec3f_t& center, const vec3f_t& right, const vec3f_t& up, const vec2f_t& size, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_arc(const vec3f_t& center, const vec3f_t& normal, const vec3f_t& start_direction, f32 radius, f32 angle_radians, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 16);
		void draw_circle(const vec3f_t& center, f32 radius, const vec3f_t& normal, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 32);
		void draw_sphere(const vec3f_t& center, f32 radius, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 32);
		void draw_capsule(const vec3f_t& center, f32 radius, f32 half_height, const vec3f_t& direction, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 32);
		void draw_cylinder(const vec3f_t& center, f32 radius, f32 half_height, const vec3f_t& direction, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 32);
		void draw_cone(const vec3f_t& origin, const vec3f_t& direction, f32 length, f32 half_angle_radians, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 24);
		void draw_cone(
			const vec3f_t& origin, const vec3f_t& direction, const vec3f_t& up, f32 length, const vec2f_t& half_angles_radians, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested, u32 segments = 24);
		void draw_frustum(const vec3f_t& origin, const vec3f_t& direction, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_frustum(
			const vec3f_t& origin, const vec3f_t& direction, const vec3f_t& up, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px = 2.0f, debug_draw_depth_e depth = debug_draw_depth_e::depth_tested);
		void draw_text_2d(const vec2f_t& position, const char* text, const color_t& color, f32 size_px = 14.0f, debug_draw_text_alignment_e alignment = debug_draw_text_alignment_e::top_left, resource_handle_t font = NULL_RESOURCE_HANDLE);
		void draw_text_3d(const vec3f_t&			  position,
						  const char*				  text,
						  const color_t&			  color,
						  f32						  size_px		= 14.0f,
						  debug_draw_depth_e		  depth			= debug_draw_depth_e::always_visible,
						  debug_draw_text_alignment_e alignment		= debug_draw_text_alignment_e::center,
						  const vec2f_t&			  screen_offset = vec2f_t::zero,
						  resource_handle_t			  font			= NULL_RESOURCE_HANDLE);
		void draw_texture_3d(const vec3f_t&		position,
							 resource_handle_t	texture,
							 const vec2f_t&		size_px,
							 const color_t&		color,
							 entity_id_t		entity_id		 = NULL_ENTITY_ID,
							 debug_draw_depth_e depth			 = debug_draw_depth_e::always_visible,
							 const vec2f_t&		screen_offset	 = vec2f_t::zero,
							 bool				is_linear_sample = true);
		void debug_draw_missing_resources(const world_t& world);
		void write_snapshot(world_debug_draw_snapshot_t& snapshot);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline u32 get_dropped_line_count() const
		{
			return _dropped_line_count;
		}

		inline u32 get_dropped_triangle_count() const
		{
			return _dropped_triangle_count;
		}

		inline u32 get_dropped_text_count() const
		{
			return _dropped_text_count;
		}

		inline u32 get_dropped_texture_count() const
		{
			return _dropped_texture_count;
		}

	private:
		struct text_command_t
		{
			resource_handle_t			font		  = NULL_RESOURCE_HANDLE;
			vec4f_t						color		  = vec4f_t::zero;
			vec3f_t						anchor		  = vec3f_t::zero;
			vec2f_t						screen_offset = vec2f_t::zero;
			f32							size_px		  = 14.0f;
			u32							text_offset	  = 0;
			u32							text_length	  = 0;
			debug_draw_text_alignment_e alignment	  = debug_draw_text_alignment_e::top_left;
			debug_draw_depth_e			depth		  = debug_draw_depth_e::always_visible;
			bool						is_screen	  = false;
		};

		struct texture_command_t
		{
			vec4f_t			   color			= vec4f_t::zero;
			vec3f_t			   position			= vec3f_t::zero;
			resource_handle_t  texture			= NULL_RESOURCE_HANDLE;
			vec2f_t			   size_px			= vec2f_t::zero;
			vec2f_t			   screen_offset	= vec2f_t::zero;
			entity_id_t		   entity_id		= NULL_ENTITY_ID;
			debug_draw_depth_e depth			= debug_draw_depth_e::always_visible;
			bool			   is_linear_sample = true;
		};

		vector_t<vertex_debug_line_t>	  _vertices;
		vector_t<primitive_index>		  _indices;
		vector_t<vertex_debug_triangle_t> _triangle_vertices;
		vector_t<primitive_index>		  _triangle_indices;
		vector_t<text_command_t>		  _text_commands;
		vector_t<texture_command_t>		  _texture_commands;
		vector_t<char>					  _text_bytes;
		ui::vg_canvas_t*				  _text_canvas			  = nullptr;
		world_debug_draw_config_t		  _config				  = {};
		u32								  _dropped_line_count	  = 0;
		u32								  _dropped_triangle_count = 0;
		u32								  _dropped_text_count	  = 0;
		u32								  _dropped_texture_count  = 0;
	};
}
