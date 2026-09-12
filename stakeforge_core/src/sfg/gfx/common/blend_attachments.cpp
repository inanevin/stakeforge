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

#include "blend_attachments.hpp"
#include <sfg/gfx/common/shader_description.hpp>

namespace sfg
{
	color_blend_attachment_t blend_attachments_t::get_none()
	{
		return {
			.blend_enabled	  = false,
			.color_comp_flags = ccf_rgba,
		};
	}

	color_blend_attachment_t blend_attachments_t::get_alpha_blend()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::src_alpha,
			.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::one,
			.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_rgba,
		};
	}

	color_blend_attachment_t blend_attachments_t::get_lcd_text()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::one,
			.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::one,
			.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_rgba,
		};
	}

	color_blend_attachment_t blend_attachments_t::get_premultiplied_alpha()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::one,
			.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::one,
			.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_rgba,
		};
	}

	color_blend_attachment_t blend_attachments_t::get_additive()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::src_alpha,
			.dst_color_blend_factor = blend_factor::one,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::zero,
			.dst_alpha_blend_factor = blend_factor::one,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_rgba,
		};
	}

	color_blend_attachment_t blend_attachments_t::get_multiply()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::dst_color,
			.dst_color_blend_factor = blend_factor::zero,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::dst_alpha,
			.dst_alpha_blend_factor = blend_factor::zero,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_rgba,
		};
	}

	color_blend_attachment_t blend_attachments_t::get_screen()
	{
		return {
			.blend_enabled			= true,
			.src_color_blend_factor = blend_factor::one,
			.dst_color_blend_factor = blend_factor::one_minus_src_color,
			.color_blend_op			= blend_op::add,
			.src_alpha_blend_factor = blend_factor::one,
			.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha,
			.alpha_blend_op			= blend_op::add,
			.color_comp_flags		= ccf_rgba,
		};
	}
}
