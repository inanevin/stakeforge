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
