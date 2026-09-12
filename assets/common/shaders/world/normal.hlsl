// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Stakeforge is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  Stakeforge is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.
//
//  As an additional permission under section 7 of GPLv3, the copyright
//  holders grant the Stakeforge Game Linking Exception, version 1.0,
//  in GAME-LINKING-EXCEPTION.md.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Krzysztof Narkowicz - https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
float2 oct_encode(float3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z) + 1e-8);
    float2 enc = (n.z >= 0.0) ? n.xy : ((1.0 - abs(n.yx)) * (float2(n.x >= 0 ? 1 : -1, n.y >= 0 ? 1 : -1)));
    return enc * 0.5 + 0.5;
}

// Rune Stubbe's optimized decode - https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
float3 oct_decode(float2 e) {
    // [0,1] -> [-1,1]
    float2 f = e * 2.0 - 1.0;
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = saturate(-n.z);
    // fold the lower hemisphere
    n.x += (n.x >= 0 ? -t : t);
    n.y += (n.y >= 0 ? -t : t);
    return normalize(n);
}

