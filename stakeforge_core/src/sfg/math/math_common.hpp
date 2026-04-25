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

#include <limits>

namespace sfg
{
	namespace math
	{
#define DEG_2_RAD								0.0174533f
#define RAD_2_DEG								57.2958f
#define MATH_PI									3.1415926535897932f
#define MATH_TWO_PI								6.28318530717959f
#define MATH_HALF_PI							1.57079632679f
#define MATH_R_PI								0.31830988618f
#define MATH_R_TWO_PI							0.159154943091895f
#define MATH_R_HALF_PI							0.636619772367581f
#define MATH_E									2.71828182845904523536f
#define MATH_R_LN_2								1.44269504088896f
#define MATH_EPS								0.00001f
#define MATH_INF_F								std::numeric_limits<f32>::infinity()
#define MATH_NAN								std::numeric_limits<f32>::quiet_NaN()
#define ALIGN_SIZE_POW(sizeToAlign, PowerOfTwo) (((sizeToAlign) + (PowerOfTwo) - 1) & ~((PowerOfTwo) - 1))
#define ALIGN_UP(size, alignment)				(size + alignment - 1) & ~(alignment - 1)
#define SET_BIT(value, bit)						value | (1 << bit)
#define CHECK_BIT(value, bit)					(value & (1 << bit)) != 0
#define UNSET_BIT(value, bit)					value & ~(1 << bit)
#define IS_POW(value)							value != 0 && (value & (value - 1)) == 0

	}
}
