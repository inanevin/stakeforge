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
#include <sfg/math/mat4x3.hpp>

struct tg3_model;
struct tg3_animation;
struct tg3_primitive;
struct tg3_skin;

namespace sfg
{
	enum class glb_axis_e : u8;
	struct animation_def_t;
	class quat_t;
	struct primitive_skinned_def_t;
	struct primitive_static_def_t;
	struct physics_collision_mesh_def_t;
	struct vec3f_t;
	struct vec4f_t;

	struct glb_basis_conversion_t
	{
		mat4x3_t transform		   = mat4x3_t::identity;
		mat4x3_t inverse_transform = mat4x3_t::identity;
	};

	class editor_glb_import_util_t final
	{
	public:
		editor_glb_import_util_t()											 = delete;
		~editor_glb_import_util_t()											 = delete;
		editor_glb_import_util_t(const editor_glb_import_util_t&)			 = delete;
		editor_glb_import_util_t& operator=(const editor_glb_import_util_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static glb_basis_conversion_t make_basis(glb_axis_e up_axis, glb_axis_e forward_axis);
		static vec3f_t				  convert_vector(const glb_basis_conversion_t& basis, const vec3f_t& value);
		static vec3f_t				  convert_scale(const glb_basis_conversion_t& basis, const vec3f_t& value);
		static vec4f_t				  convert_tangent(const glb_basis_conversion_t& basis, const vec4f_t& value);
		static quat_t				  convert_rotation(const glb_basis_conversion_t& basis, const quat_t& value);
		static quat_t				  convert_rotation_tangent(const glb_basis_conversion_t& basis, const quat_t& value);
		static mat4x3_t				  convert_transform(const glb_basis_conversion_t& basis, const mat4x3_t& value);

		static i32	find_attribute(const tg3_primitive& primitive, const char* name);
		static bool import_animation(const tg3_model& model, const tg3_animation& animation, const glb_basis_conversion_t& basis, animation_def_t& out, u32& out_skin_index);
		static bool read_inverse_bind_matrix(const tg3_model& model, const tg3_skin& skin, const glb_basis_conversion_t& basis, u32 joint_index, mat4x3_t& out_matrix);
		static bool import_static_primitive(const tg3_model& model, const tg3_primitive& primitive, const glb_basis_conversion_t& basis, u32 material_index, primitive_static_def_t& out);
		static bool import_skinned_primitive(const tg3_model& model, const tg3_primitive& primitive, const glb_basis_conversion_t& basis, u32 material_index, primitive_skinned_def_t& out);
		static bool import_collision_primitive(const tg3_model& model, const tg3_primitive& primitive, const glb_basis_conversion_t& basis, physics_collision_mesh_def_t& out);
	};
}
