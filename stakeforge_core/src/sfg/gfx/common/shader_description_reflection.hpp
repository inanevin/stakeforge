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

#include "shader_description.hpp"
#include <sfg/common/hashing.hpp>

namespace sfg
{
	struct vertex_input_reflection_t
	{
		static constexpr sid_t TYPE_ID = "vertex_input_t"_hs;

		vertex_input_reflection_t();
	};

	struct cull_mode_reflection_t
	{
		static constexpr sid_t TYPE_ID = "cull_mode"_hs;

		cull_mode_reflection_t();
	};

	struct fill_mode_reflection_t
	{
		static constexpr sid_t TYPE_ID = "fill_mode"_hs;

		fill_mode_reflection_t();
	};

	struct front_face_reflection_t
	{
		static constexpr sid_t TYPE_ID = "front_face"_hs;

		front_face_reflection_t();
	};

	struct blend_factor_reflection_t
	{
		static constexpr sid_t TYPE_ID = "blend_factor"_hs;

		blend_factor_reflection_t();
	};

	struct blend_op_reflection_t
	{
		static constexpr sid_t TYPE_ID = "blend_op"_hs;

		blend_op_reflection_t();
	};

	struct stencil_op_reflection_t
	{
		static constexpr sid_t TYPE_ID = "stencil_op"_hs;

		stencil_op_reflection_t();
	};

	struct compare_op_reflection_t
	{
		static constexpr sid_t TYPE_ID = "compare_op"_hs;

		compare_op_reflection_t();
	};

	struct store_op_reflection_t
	{
		static constexpr sid_t TYPE_ID = "store_op"_hs;

		store_op_reflection_t();
	};

	struct load_op_reflection_t
	{
		static constexpr sid_t TYPE_ID = "load_op"_hs;

		load_op_reflection_t();
	};

	struct stencil_state_reflection_t
	{
		static constexpr sid_t TYPE_ID = "stencil_state_t"_hs;

		stencil_state_reflection_t();
	};

	inline vertex_input_reflection_t  g_reflect_vertex_input;
	inline cull_mode_reflection_t	  g_reflect_cull_mode;
	inline fill_mode_reflection_t	  g_reflect_fill_mode;
	inline front_face_reflection_t	  g_reflect_front_face;
	inline blend_factor_reflection_t  g_reflect_blend_factor;
	inline blend_op_reflection_t	  g_reflect_blend_op;
	inline stencil_op_reflection_t	  g_reflect_stencil_op;
	inline compare_op_reflection_t	  g_reflect_compare_op;
	inline store_op_reflection_t	  g_reflect_store_op;
	inline load_op_reflection_t		  g_reflect_load_op;
	inline stencil_state_reflection_t g_reflect_stencil_state;
}
