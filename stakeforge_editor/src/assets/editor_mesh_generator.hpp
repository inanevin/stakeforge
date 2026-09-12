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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	class ostream_t;

	struct editor_mesh_generator_cube_params_t
	{
		vec3f_t size = vec3f_t::one;
	};

	struct editor_mesh_generator_plane_params_t
	{
		vec2f_t size = vec2f_t::one;
	};

	struct editor_mesh_generator_sphere_params_t
	{
		f32 radius	 = 0.5f;
		u16 segments = 32;
		u16 rings	 = 16;
	};

	struct editor_mesh_generator_cylinder_params_t
	{
		f32 radius	 = 0.5f;
		f32 height	 = 1.0f;
		u16 segments = 32;
	};

	struct editor_mesh_generator_capsule_params_t
	{
		f32 radius			 = 0.5f;
		f32 height			 = 2.0f;
		u16 segments		 = 32;
		u16 hemisphere_rings = 8;
	};

	struct editor_mesh_generator_translation_gizmo_params_t
	{
		f32 shaft_radius = 0.0125f;
		f32 arrow_radius = 0.075f;
		f32 arrow_length = 0.2f;
		u16 segments	 = 16;
	};

	struct editor_mesh_generator_scale_gizmo_params_t
	{
		f32 shaft_radius = 0.0125f;
		f32 cube_size	 = 0.15f;
		u16 segments	 = 16;
	};

	struct editor_mesh_generator_rotation_gizmo_params_t
	{
		f32 radius	  = 1.0f;
		f32 thickness = 0.025f;
		u16 segments  = 64;
	};

	class editor_mesh_generator_t final
	{
	public:
		editor_mesh_generator_t()										   = default;
		~editor_mesh_generator_t()										   = default;
		editor_mesh_generator_t(const editor_mesh_generator_t&)			   = delete;
		editor_mesh_generator_t& operator=(const editor_mesh_generator_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool generate_cube(const editor_mesh_generator_cube_params_t& params, ostream_t& out);
		static bool generate_plane(const editor_mesh_generator_plane_params_t& params, ostream_t& out);
		static bool generate_sphere(const editor_mesh_generator_sphere_params_t& params, ostream_t& out);
		static bool generate_cylinder(const editor_mesh_generator_cylinder_params_t& params, ostream_t& out);
		static bool generate_capsule(const editor_mesh_generator_capsule_params_t& params, ostream_t& out);
		static bool generate_translation_gizmo(const editor_mesh_generator_translation_gizmo_params_t& params, ostream_t& out);
		static bool generate_scale_gizmo(const editor_mesh_generator_scale_gizmo_params_t& params, ostream_t& out);
		static bool generate_rotation_gizmo(const editor_mesh_generator_rotation_gizmo_params_t& params, ostream_t& out);
	};
}
