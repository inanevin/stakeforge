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

#include "gfx_util.hpp"
#include "math/vec4f.hpp"
#include "gfx/backend/backend.hpp"

namespace sfg
{
	gfx_bind_layout_handle gfx_util_t::create_bind_layout_global(bool is_compute)
	{
		gfx_backend* backend = gfx_backend::get();

		gfx_bind_layout_handle layout = backend->create_empty_bind_layout();
		backend->bind_layout_add_constant(layout, constant_max, 0, 0, shader_stage::all);

		const shader_stage stg = is_compute ? shader_stage::compute : shader_stage::fragment;

		backend->bind_layout_add_immutable_sampler(layout, 0, 0, gfx_util_t::get_sampler_desc_anisotropic(), stg);
		backend->bind_layout_add_immutable_sampler(layout, 0, 1, gfx_util_t::get_sampler_desc_anisotropic_repeat(), stg);
		backend->bind_layout_add_immutable_sampler(layout, 0, 2, gfx_util_t::get_sampler_desc_linear(), stg);
		backend->bind_layout_add_immutable_sampler(layout, 0, 3, gfx_util_t::get_sampler_desc_linear_repeat(), stg);
		backend->bind_layout_add_immutable_sampler(layout, 0, 4, gfx_util_t::get_sampler_desc_nearest(), stg);
		backend->bind_layout_add_immutable_sampler(layout, 0, 5, gfx_util_t::get_sampler_desc_nearest_repeat(), stg);

		if (!is_compute)
		{
			backend->bind_layout_add_immutable_sampler(layout, 0, 8, gfx_util_t::get_sampler_desc_shadow_2d(), stg);
			backend->bind_layout_add_immutable_sampler(layout, 0, 9, gfx_util_t::get_sampler_desc_shadow_cube(), stg);
		}

		backend->finalize_bind_layout(layout, is_compute, true, "global_layout");

		return layout;
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_anisotropic()
	{
		return {
			.anisotropy = 6,
			.min_lod	= 0.0f,
			.max_lod	= 10.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_min_anisotropic | sampler_flags::saf_mag_anisotropic | sampler_flags::saf_mip_linear | sampler_flags::saf_border_transparent,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_anisotropic_repeat()
	{
		return {
			.anisotropy = 8,
			.min_lod	= 0.0f,
			.max_lod	= 10.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_min_anisotropic | sampler_flags::saf_mag_anisotropic | sampler_flags::saf_mip_linear | sampler_flags::saf_border_transparent,
			.address_u	= address_mode::repeat,
			.address_v	= address_mode::repeat,
		};
	}
	sampler_desc_t gfx_util_t::get_sampler_desc_linear()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 10.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_min_linear | sampler_flags::saf_mag_linear | sampler_flags::saf_mip_linear | sampler_flags::saf_border_transparent,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_linear_repeat()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 10.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_min_linear | sampler_flags::saf_mag_linear | sampler_flags::saf_mip_linear | sampler_flags::saf_border_transparent,
			.address_u	= address_mode::repeat,
			.address_v	= address_mode::repeat,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_nearest()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 10.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_min_nearest | sampler_flags::saf_mag_nearest | sampler_flags::saf_mip_nearest | sampler_flags::saf_border_transparent,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_nearest_repeat()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 10.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_min_nearest | sampler_flags::saf_mag_nearest | sampler_flags::saf_mip_nearest | sampler_flags::saf_border_transparent,
			.address_u	= address_mode::repeat,
			.address_v	= address_mode::repeat,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_shadow_2d()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 0.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_compare | sampler_flags::saf_min_linear | sampler_flags::saf_mag_linear | sampler_flags::saf_mip_nearest | sampler_flags::saf_border_white,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
			.compare	= compare_op::lequal,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_shadow_cube()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 0.0f,
			.lod_bias	= 0.0f,
			.flags		= sampler_flags::saf_compare | sampler_flags::saf_min_linear | sampler_flags::saf_mag_linear | sampler_flags::saf_mip_nearest | sampler_flags::saf_border_white,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
			.compare	= compare_op::lequal,
		};
	}

	color_blend_attachment_t gfx_util_t::get_blend_attachment_alpha_blending()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::src_alpha,
			.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::one,
			.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_red | ccf_green | ccf_blue | ccf_alpha,
		};
	}

}
