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
#include <sfg/gfx/common/descriptions.hpp>

namespace sfg
{
	enum gpu_constant_e : u8
	{
		constant_global0 = 0,
		constant_global1,
		constant_global2,
		constant_global3,
		constant_rp0,
		constant_rp1,
		constant_rp2,
		constant_rp3,
		constant_rp4,
		constant_rp5,
		constant_rp6,
		constant_rp7,
		constant_rp8,
		constant_rp9,
		constant_rp10,
		constant_rp11,
		constant_rp12,
		constant_rp13,
		constant_rp14,
		constant_rp15,
		constant_mat0,
		constant_mat1,
		constant_mat2,
		constant_mat3,
		constant_mat4,
		constant_mat5,
		constant_mat6,
		constant_mat7,
		constant_mat8,
		constant_mat9,
		constant_mat10,
		constant_mat11,
		constant_mat12,
		constant_mat13,
		constant_mat14,
		constant_mat15,
		constant_mat16,
		constant_obj0,
		constant_obj1,
		constant_obj2,
		constant_obj3,
		constant_obj4,
		constant_obj5,
		constant_obj6,
		constant_obj7,
		constant_obj8,
		constant_obj9,
		constant_obj10,
		constant_obj11,
		constant_obj12,
		constant_obj13,
		constant_max,
	};

	class gfx_util_t
	{
	public:
		static gfx_handle_t	  create_bind_layout_global(bool is_compute);
		static sampler_desc_t get_sampler_desc_anisotropic();
		static sampler_desc_t get_sampler_desc_anisotropic_repeat();
		static sampler_desc_t get_sampler_desc_linear();
		static sampler_desc_t get_sampler_desc_linear_repeat();
		static sampler_desc_t get_sampler_desc_nearest();
		static sampler_desc_t get_sampler_desc_nearest_repeat();
		static sampler_desc_t get_sampler_desc_shadow_2d();
		static sampler_desc_t get_sampler_desc_shadow_cube();
	};
}
