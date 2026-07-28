// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//     1. Redistributions of source code must retain the above copyright notice, this
//        list of conditions and the following disclaimer.
//  
//     2. Redistributions in binary form must reproduce the above copyright notice,
//        this list of conditions and the following disclaimer in the documentation
//        and/or other materials provided with the distribution.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
//  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define static_sampler_ani register(s0, space0)
#define static_sampler_ani_repeat register(s1, space0)
#define static_sampler_linear register(s2, space0)
#define static_sampler_linear_repeat register(s3, space0)
#define static_sampler_nearest register(s4, space0)
#define static_sampler_nearest_repeat register(s5, space0)
#define static_sampler_shadow_2d register(s8, space0)
#define static_sampler_shadow_cube register(s9, space0)

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

template<typename T>
T sfg_get_texture_non_uniform(uint index)
{
    T txt = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    return txt;
}

SamplerState sfg_get_sampler_state(uint index)
{
    SamplerState ss = SamplerDescriptorHeap[index];
    return ss;
}

