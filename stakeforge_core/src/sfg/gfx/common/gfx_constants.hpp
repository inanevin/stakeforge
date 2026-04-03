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

#define NOMINMAX
#include <limits>

namespace sfg
{
#define MAX_RESOURCES		   1024
#define MAX_TEXTURES		   1024
#define MAX_SAMPLERS		   128
#define MAX_SEMAPHORES		   64
#define MAX_SHADERS			   2048
#define MAX_PIPELINE_LAYOUTS   256
#define MAX_SWAPCHAINS		   8
#define MAX_BIND_GROUPS		   512
#define MAX_BIND_LAYOUTS	   128
#define MAX_COMMAND_BUFFERS	   256
#define MAX_QUEUES			   8
#define MAX_DESCRIPTOR_HANDLES 1024
#define COMMANDS_MAX_TID	   25
#define BACK_BUFFER_COUNT	   3
#define FRAME_LATENCY		   2
#define GPU_DEVICE			   0 // 0 discrete, 1 integratd

	typedef unsigned short gfx_id;
	typedef unsigned short primitive_index;
	typedef unsigned int   gpu_index;

#define NULL_GFX_ID	   (unsigned short)0xFFFF
#define NULL_GPU_INDEX (unsigned int)0xFFFFFFFF
}
