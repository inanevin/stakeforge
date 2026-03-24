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

#include "easing.hpp"
#include "math.hpp"

namespace SFG
{
	namespace easing
	{

		f32 smooth_damp(f32 current, f32 target, f32* current_velocity, f32 smooth_time, f32 maxSpeed, f32 dt)
		{
			smooth_time		  = math::max(0.0001f, smooth_time);
			f32 num			  = 2.0f / smooth_time;
			f32 num2		  = num * dt;
			f32 num3		  = 1.0f / (1.0f + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
			f32 num4		  = current - target;
			f32 num5		  = target;
			f32 num6		  = maxSpeed * smooth_time;
			num4			  = math::clamp(num4, -num6, num6);
			target			  = current - num4;
			f32 num7		  = (*current_velocity + num * num4) * dt;
			*current_velocity = (*current_velocity - num * num7) * num3;
			f32 num8		  = target + (num4 + num7) * num3;
			if (num5 - current > 0.0f == num8 > num5)
			{
				num8			  = num5;
				*current_velocity = (num8 - num5) / dt;
			}
			return num8;
		}

		f32 lerp(f32 val1, f32 val2, f32 amt)
		{
			return (val1 * (1.0f - amt) + val2 * amt);
		}

		f32 cubic_lerp(f32 val1, f32 val2, f32 amt)
		{
			return lerp(val1, val2, 3 * amt * amt - 2 * amt * amt * amt);
		}

		f32 cubic_interp(f32 val0, f32 val1, f32 val2, f32 val3, f32 amt)
		{
			f32 amt2 = amt * amt;
			return ((val3 * (1.0f / 2.0f) - val2 * (3.0f / 2.0f) - val0 * (1.0f / 2.0f) + val1 * (3.0f / 2.0f)) * amt * amt2 + (val0 - val1 * (5.0f / 2.0f) + val2 * 2.0f - val3 * (1.0f / 2.0f)) * amt2 + (val2 * (1.0f / 2.0f) - val0 * (1.0f / 2.0f)) * amt +
					val1);
		}

		f32 cubic_interp_tangents(f32 val1, f32 tan1, f32 val2, f32 tan2, f32 amt)
		{
			f32 amt2 = amt * amt;
			return ((tan2 - val2 * 2.0f + tan1 + val1 * (2.0f)) * amt * amt2 + (tan1 * 2.0f - val1 * 3.0f + val2 * 3.0f - tan2 * 2.0f) * amt2 + tan1 * amt + val1);
		}

		f32 bilerp(f32 val00, f32 val10, f32 val01, f32 val11, f32 amtX, f32 amtY)
		{
			return lerp(lerp(val00, val10, amtX), lerp(val01, val11, amtX), amtY);
		}

		f32 step(f32 edge, f32 x)
		{
			return x < edge ? 0.0f : 1.0f;
		}

		f32 ease_in(f32 start, f32 end, f32 alpha)
		{
			return lerp(start, end, alpha * alpha);
		}

		f32 ease_out(f32 start, f32 end, f32 alpha)
		{
			return lerp(start, end, 1.0f - (1.0f - alpha) * (1.0f - alpha));
		}

		f32 ease_in_out(f32 start, f32 end, f32 alpha)
		{
			if (alpha < 0.5f)
				return lerp(start, end, 2.0f * alpha * alpha);
			return lerp(start, end, 1.0f - math::fast_pow(-2.0f * alpha + 2.0f, 2.0f) / 2.0f);
		}

		f32 cubic(f32 start, f32 end, f32 alpha)
		{
			return lerp(start, end, alpha * alpha * alpha);
		}

		f32 exponential(f32 start, f32 end, f32 alpha)
		{
			if (alpha < 0.001f && alpha > -0.001f)
				return 0.0f;

			return lerp(start, end, math::fast_pow(2.0f, 10.0f * alpha - 10.0f));
		}

		f32 bounce(f32 start, f32 end, f32 alpha)
		{
			if (alpha < (1.0f / 2.75f))
			{
				return lerp(start, end, 7.5625f * alpha * alpha);
			}
			else if (alpha < (2.0f / 2.75f))
			{
				alpha -= (1.5f / 2.75f);
				return lerp(start, end, 7.5625f * alpha * alpha + 0.75f);
			}
			else if (alpha < (2.5f / 2.75f))
			{
				alpha -= (2.25f / 2.75f);
				return lerp(start, end, 7.5625f * alpha * alpha + 0.9375f);
			}
			else
			{
				alpha -= (2.625f / 2.75f);
				return lerp(start, end, 7.5625f * alpha * alpha + 0.984375f);
			}
		}

		f32 sinusodial(f32 start, f32 end, f32 alpha)
		{
			return lerp(start, end, -math::cos(alpha * MATH_PI) / 2.0f + 0.5f);
		};
	}

}
