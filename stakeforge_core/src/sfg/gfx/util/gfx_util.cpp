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
#include <sfg/math/vec4f.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>

namespace sfg
{
	gfx_handle_t gfx_util_t::create_bind_layout_global(bool is_compute)
	{
		gfx_backend& backend = gfx_backend::get();

		gfx_handle_t layout = backend.create_empty_bind_layout();
		backend.bind_layout_add_constant(layout, constant_max, 0, 0, shader_stage_e::all);

		const shader_stage_e stg = is_compute ? shader_stage_e::compute : shader_stage_e::fragment;

		backend.bind_layout_add_immutable_sampler(layout, 0, 0, gfx_util_t::get_sampler_desc_anisotropic(), stg);
		backend.bind_layout_add_immutable_sampler(layout, 0, 1, gfx_util_t::get_sampler_desc_anisotropic_repeat(), stg);
		backend.bind_layout_add_immutable_sampler(layout, 0, 2, gfx_util_t::get_sampler_desc_linear(), stg);
		backend.bind_layout_add_immutable_sampler(layout, 0, 3, gfx_util_t::get_sampler_desc_linear_repeat(), stg);
		backend.bind_layout_add_immutable_sampler(layout, 0, 4, gfx_util_t::get_sampler_desc_nearest(), stg);
		backend.bind_layout_add_immutable_sampler(layout, 0, 5, gfx_util_t::get_sampler_desc_nearest_repeat(), stg);

		if (!is_compute)
		{
			backend.bind_layout_add_immutable_sampler(layout, 0, 8, gfx_util_t::get_sampler_desc_shadow_2d(), stg);
			backend.bind_layout_add_immutable_sampler(layout, 0, 9, gfx_util_t::get_sampler_desc_shadow_cube(), stg);
		}

		backend.finalize_bind_layout(layout, is_compute, true, "global_layout");

		return layout;
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_anisotropic()
	{
		return {
			.anisotropy = 6,
			.min_lod	= 0.0f,
			.max_lod	= 16.0f,
			.lod_bias	= 0.0f,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
			.min_filter = sampler_filter_e::anisotropic,
			.mag_filter = sampler_filter_e::anisotropic,
			.mip_filter = sampler_filter_e::linear,
			.border		= sampler_border_e::transparent,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_anisotropic_repeat()
	{
		return {
			.anisotropy = 8,
			.min_lod	= 0.0f,
			.max_lod	= 16.0f,
			.lod_bias	= 0.0f,
			.address_u	= address_mode::repeat,
			.address_v	= address_mode::repeat,
			.min_filter = sampler_filter_e::anisotropic,
			.mag_filter = sampler_filter_e::anisotropic,
			.mip_filter = sampler_filter_e::linear,
			.border		= sampler_border_e::transparent,
		};
	}
	sampler_desc_t gfx_util_t::get_sampler_desc_linear()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 16.0f,
			.lod_bias	= 0.0f,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
			.min_filter = sampler_filter_e::linear,
			.mag_filter = sampler_filter_e::linear,
			.mip_filter = sampler_filter_e::linear,
			.border		= sampler_border_e::transparent,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_linear_repeat()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 16.0f,
			.lod_bias	= 0.0f,
			.address_u	= address_mode::repeat,
			.address_v	= address_mode::repeat,
			.min_filter = sampler_filter_e::linear,
			.mag_filter = sampler_filter_e::linear,
			.mip_filter = sampler_filter_e::linear,
			.border		= sampler_border_e::transparent,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_nearest()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 16.0f,
			.lod_bias	= 0.0f,
			.address_u	= address_mode::clamp,
			.address_v	= address_mode::clamp,
			.min_filter = sampler_filter_e::nearest,
			.mag_filter = sampler_filter_e::nearest,
			.mip_filter = sampler_filter_e::nearest,
			.border		= sampler_border_e::transparent,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_nearest_repeat()
	{
		return {
			.anisotropy = 0,
			.min_lod	= 0.0f,
			.max_lod	= 16.0f,
			.lod_bias	= 0.0f,
			.address_u	= address_mode::repeat,
			.address_v	= address_mode::repeat,
			.min_filter = sampler_filter_e::nearest,
			.mag_filter = sampler_filter_e::nearest,
			.mip_filter = sampler_filter_e::nearest,
			.border		= sampler_border_e::transparent,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_shadow_2d()
	{
		return {
			.anisotropy	 = 0,
			.min_lod	 = 0.0f,
			.max_lod	 = 0.0f,
			.lod_bias	 = 0.0f,
			.address_u	 = address_mode::clamp,
			.address_v	 = address_mode::clamp,
			.min_filter	 = sampler_filter_e::linear,
			.mag_filter	 = sampler_filter_e::linear,
			.mip_filter	 = sampler_filter_e::nearest,
			.border		 = sampler_border_e::white,
			.compare	 = compare_op::lequal,
			.use_compare = true,
		};
	}

	sampler_desc_t gfx_util_t::get_sampler_desc_shadow_cube()
	{
		return {
			.anisotropy	 = 0,
			.min_lod	 = 0.0f,
			.max_lod	 = 0.0f,
			.lod_bias	 = 0.0f,
			.address_u	 = address_mode::clamp,
			.address_v	 = address_mode::clamp,
			.min_filter	 = sampler_filter_e::linear,
			.mag_filter	 = sampler_filter_e::linear,
			.mip_filter	 = sampler_filter_e::nearest,
			.border		 = sampler_border_e::white,
			.compare	 = compare_op::lequal,
			.use_compare = true,
		};
	}

}
