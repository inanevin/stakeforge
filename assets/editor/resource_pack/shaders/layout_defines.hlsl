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

#define static_sampler_ani register(s0, space0)
#define static_sampler_ani_repeat register(s1, space0)
#define static_sampler_linear register(s2, space0)
#define static_sampler_linear_repeat register(s3, space0)
#define static_sampler_nearest register(s4, space0)
#define static_sampler_nearest_repeat register(s5, space0)

struct sfg_globals
{
    float sfg_global_delta;
    float sfg_global_elapsed;
};

cbuffer sfg_constants : register(b0, space0)
{
   uint sfg_constant_global0;
   uint sfg_constant_global1;
   uint sfg_constant_global2;
   uint sfg_constant_global3;
   uint sfg_constant_rp0;
   uint sfg_constant_rp1;
   uint sfg_constant_rp2;
   uint sfg_constant_rp3;
   uint sfg_constant_rp4;
   uint sfg_constant_rp5;
   uint sfg_constant_rp6;
   uint sfg_constant_rp7;
   uint sfg_constant_rp8;
   uint sfg_constant_rp9;
   uint sfg_constant_rp10;
   uint sfg_constant_rp11;
   uint sfg_constant_rp12;
   uint sfg_constant_rp13;
   uint sfg_constant_rp14;
   uint sfg_constant_rp15;
   uint sfg_constant_mat0;
   uint sfg_constant_mat1;
   uint sfg_constant_mat2;
   uint sfg_constant_mat3;
   uint sfg_constant_mat4;
   uint sfg_constant_mat5;
   uint sfg_constant_mat6;
   uint sfg_constant_mat7;
   uint sfg_constant_mat8;
   uint sfg_constant_mat9;
   uint sfg_constant_mat10;
   uint sfg_constant_mat11;
   uint sfg_constant_mat12;
   uint sfg_constant_mat13;
   uint sfg_constant_mat14;
   uint sfg_constant_mat15;
   uint sfg_constant_mat16;
   uint sfg_constant_obj0;
   uint sfg_constant_obj1;
   uint sfg_constant_obj2;
   uint sfg_constant_obj3;
   uint sfg_constant_obj4;
   uint sfg_constant_obj5;
   uint sfg_constant_obj6;
   uint sfg_constant_obj7;
   uint sfg_constant_obj8;
   uint sfg_constant_obj9;
   uint sfg_constant_obj10;
   uint sfg_constant_obj11;
   uint sfg_constant_obj12;
   uint sfg_constant_obj13;
}


template<typename T>
ConstantBuffer<T> sfg_get_cbv(uint index)
{
    ConstantBuffer<T> b = ResourceDescriptorHeap[index];
    return b;
}

template<typename T>
StructuredBuffer<T> sfg_get_ssbo(uint index)
{
    StructuredBuffer<T> b = ResourceDescriptorHeap[index];
    return b;
}

template<typename T>
RWStructuredBuffer<T> sfg_get_rws_buffer(uint index)
{
    RWStructuredBuffer<T> b = ResourceDescriptorHeap[index];
    return b;
}

RWByteAddressBuffer sfg_get_rwb_buffer(uint index)
{
    RWByteAddressBuffer b = ResourceDescriptorHeap[index];
    return b;
}

template<typename T>
T sfg_get_texture(uint index)
{
    T txt = ResourceDescriptorHeap[index];
    return txt;
}

SamplerState sfg_get_sampler_state(uint index)
{
    SamplerState ss = SamplerDescriptorHeap[index];
    return ss;
}

