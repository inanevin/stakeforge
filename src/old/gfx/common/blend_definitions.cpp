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

#include "blend_definitions.hpp"
#include "gfx/common/shader_description.hpp"

namespace SFG
{
	void blend_definitions::get_blend_attachment(blend_definition_style style, color_blend_attachment& out_attachment)
	{
		if (style == blend_definition_style::none)
		{
			out_attachment = {
				.blend_enabled	  = false,
				.color_comp_flags = ccf_red | ccf_green | ccf_blue | ccf_alpha,
			};
		}
		else if (style == blend_definition_style::alpha_blend)
		{
			out_attachment = {
				.blend_enabled			= true,
				.src_color_blend_factor = blend_factor::src_alpha,
				.dst_color_blend_factor = blend_factor::one_minus_src_alpha,
				.color_blend_op			= blend_op::add,
				.src_alpha_blend_factor = blend_factor::one,
				.dst_alpha_blend_factor = blend_factor::zero,
				.alpha_blend_op			= blend_op::add,
				.color_comp_flags		= ccf_red | ccf_green | ccf_blue | ccf_alpha,
			};
		}
		else if (style == blend_definition_style::additive)
		{
			out_attachment = {
				.blend_enabled = true,
				// Cout = Cs * As + Cd
				.src_color_blend_factor = blend_factor::src_alpha,
				.dst_color_blend_factor = blend_factor::one,
				.color_blend_op			= blend_op::add,

				.src_alpha_blend_factor = blend_factor::zero,
				.dst_alpha_blend_factor = blend_factor::one,
				.alpha_blend_op			= blend_op::add,

				.color_comp_flags = ccf_red | ccf_green | ccf_blue | ccf_alpha,
			};
		}
	}
}
