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

#include <cstddef>
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	class color_t;

	class color_utils_t final
	{
	public:
		static color_t	lerp(const color_t& c1, const color_t& c2, f32 a);
		static color_t	from_hex(const string_t& hex);
		static string_t to_hex(const color_t& color_t);
		static void		to_hex(const color_t& color_t, char* out, size_t capacity);
		static color_t	hs_to_srgb(const color_t& color_t);
		static color_t	srgb_to_hsv(const color_t& color_t);
		static color_t	hsv_to_srgb(const color_t& color_t);
		static color_t	srgb_to_linear(const color_t& color_t);
		static color_t	linear_to_srgb(const color_t& color_t);
		static color_t	brighten(const color_t& color_t, f32 amt);
		static color_t	darken(const color_t& color_t, f32 amt);
	};

} // namespace sfg
