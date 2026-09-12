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

namespace sfg
{
	class easing_t
	{
	public:
		static f32 smooth_damp(f32 current, f32 target, f32* currentVelocity, f32 smoothTime, f32 maxSpeed, f32 deltaTime);
		static f32 lerp(f32 val1, f32 val2, f32 amt);
		static f32 cubic_lerp(f32 val1, f32 val2, f32 amt);
		static f32 cubic_interp(f32 val0, f32 val1, f32 val2, f32 val3, f32 amt);
		static f32 cubic_interp_tangents(f32 val1, f32 tan1, f32 val2, f32 tan2, f32 amt);
		static f32 bilerp(f32 val00, f32 val10, f32 val01, f32 val11, f32 amtX, f32 amtY);
		static f32 step(f32 edge, f32 x);
		static f32 ease_in(f32 start, f32 end, f32 alpha);
		static f32 ease_out(f32 start, f32 end, f32 alpha);
		static f32 ease_in_out(f32 start, f32 end, f32 alpha);
		static f32 cubic(f32 start, f32 end, f32 alpha);
		static f32 exponential(f32 start, f32 end, f32 alpha);
		static f32 bounce(f32 start, f32 end, f32 alpha);
		static f32 sinusodial(f32 start, f32 end, f32 alpha);
	};
}
