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
