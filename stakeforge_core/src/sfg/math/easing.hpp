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
