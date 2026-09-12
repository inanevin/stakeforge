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

#include "math.hpp"

namespace sfg
{
	unsigned int math::floor_log2(unsigned int val)
	{
		unsigned int pos = 0;
		if (val >= 1 << 16)
		{
			val >>= 16;
			pos += 16;
		}
		if (val >= 1 << 8)
		{
			val >>= 8;
			pos += 8;
		}
		if (val >= 1 << 4)
		{
			val >>= 4;
			pos += 4;
		}
		if (val >= 1 << 2)
		{
			val >>= 2;
			pos += 2;
		}
		if (val >= 1 << 1)
		{
			pos += 1;
		}
		return (val == 0) ? 0 : pos;
	}

	// Martin Ankerl - https://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp/
	double math::fast_pow(double a, double b)
	{
		union {
			double d;
			int	   x[2];
		} u = {a};

		u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
		u.x[0] = 0;

		return u.d;
	}

}
