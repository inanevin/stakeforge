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

#include "easing.hpp"
#include "math.hpp"

namespace sfg
{
	f32 easing_t::smooth_damp(f32 current, f32 target, f32* current_velocity, f32 smooth_time, f32 maxSpeed, f32 dt)
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

	f32 easing_t::lerp(f32 val1, f32 val2, f32 amt)
	{
		return (val1 * (1.0f - amt) + val2 * amt);
	}

	f32 easing_t::cubic_lerp(f32 val1, f32 val2, f32 amt)
	{
		return lerp(val1, val2, 3 * amt * amt - 2 * amt * amt * amt);
	}

	f32 easing_t::cubic_interp(f32 val0, f32 val1, f32 val2, f32 val3, f32 amt)
	{
		f32 amt2 = amt * amt;
		return ((val3 * (1.0f / 2.0f) - val2 * (3.0f / 2.0f) - val0 * (1.0f / 2.0f) + val1 * (3.0f / 2.0f)) * amt * amt2 + (val0 - val1 * (5.0f / 2.0f) + val2 * 2.0f - val3 * (1.0f / 2.0f)) * amt2 + (val2 * (1.0f / 2.0f) - val0 * (1.0f / 2.0f)) * amt + val1);
	}

	f32 easing_t::cubic_interp_tangents(f32 val1, f32 tan1, f32 val2, f32 tan2, f32 amt)
	{
		f32 amt2 = amt * amt;
		return ((tan2 - val2 * 2.0f + tan1 + val1 * (2.0f)) * amt * amt2 + (tan1 * 2.0f - val1 * 3.0f + val2 * 3.0f - tan2 * 2.0f) * amt2 + tan1 * amt + val1);
	}

	f32 easing_t::bilerp(f32 val00, f32 val10, f32 val01, f32 val11, f32 amtX, f32 amtY)
	{
		return lerp(lerp(val00, val10, amtX), lerp(val01, val11, amtX), amtY);
	}

	f32 easing_t::step(f32 edge, f32 x)
	{
		return x < edge ? 0.0f : 1.0f;
	}

	f32 easing_t::ease_in(f32 start, f32 end, f32 alpha)
	{
		return lerp(start, end, alpha * alpha);
	}

	f32 easing_t::ease_out(f32 start, f32 end, f32 alpha)
	{
		return lerp(start, end, 1.0f - (1.0f - alpha) * (1.0f - alpha));
	}

	f32 easing_t::ease_in_out(f32 start, f32 end, f32 alpha)
	{
		if (alpha < 0.5f)
			return lerp(start, end, 2.0f * alpha * alpha);

		const f32 remaining = 1.0f - alpha;

		return lerp(start, end, 1.0f - 2.0f * remaining * remaining);
	}

	f32 easing_t::cubic(f32 start, f32 end, f32 alpha)
	{
		return lerp(start, end, alpha * alpha * alpha);
	}

	f32 easing_t::exponential(f32 start, f32 end, f32 alpha)
	{
		if (alpha < 0.001f && alpha > -0.001f)
			return 0.0f;

		return lerp(start, end, math::fast_pow(2.0f, 10.0f * alpha - 10.0f));
	}

	// Robert Penner - https://robertpenner.com/easing/
	f32 easing_t::bounce(f32 start, f32 end, f32 alpha)
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

	f32 easing_t::sinusodial(f32 start, f32 end, f32 alpha)
	{
		return lerp(start, end, -math::cos(alpha * MATH_PI) / 2.0f + 0.5f);
	}
}
