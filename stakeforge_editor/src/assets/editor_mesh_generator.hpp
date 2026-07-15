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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	class ostream_t;

	struct editor_mesh_generator_cube_params_t
	{
		vec3f_t size = vec3f_t::one;
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
		f32 shaft_radius = 0.025f;
		f32 arrow_radius = 0.075f;
		f32 arrow_length = 0.2f;
		u16 segments	 = 16;
	};

	struct editor_mesh_generator_scale_gizmo_params_t
	{
		f32 shaft_radius = 0.025f;
		f32 cube_size	 = 0.15f;
		u16 segments	 = 16;
	};

	struct editor_mesh_generator_rotation_gizmo_params_t
	{
		f32 radius	  = 1.0f;
		f32 thickness = 0.05f;
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
		static bool generate_sphere(const editor_mesh_generator_sphere_params_t& params, ostream_t& out);
		static bool generate_cylinder(const editor_mesh_generator_cylinder_params_t& params, ostream_t& out);
		static bool generate_capsule(const editor_mesh_generator_capsule_params_t& params, ostream_t& out);
		static bool generate_translation_gizmo(const editor_mesh_generator_translation_gizmo_params_t& params, ostream_t& out);
		static bool generate_scale_gizmo(const editor_mesh_generator_scale_gizmo_params_t& params, ostream_t& out);
		static bool generate_rotation_gizmo(const editor_mesh_generator_rotation_gizmo_params_t& params, ostream_t& out);
	};
}
