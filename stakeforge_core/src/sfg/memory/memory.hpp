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

#ifdef SFG_COMPILER_MSVC
#include <malloc.h>
#define SFG_ALIGNED_MALLOC(ALIGNMENT, SIZE) _aligned_malloc(SIZE, ALIGNMENT)
#define SFG_ALIGNED_FREE(...)				_aligned_free(__VA_ARGS__)
#else
#include <cstdlib>
#define SFG_ALIGNED_MALLOC(...) std::aligned_alloc(__VA_ARGS__)
#define SFG_ALIGNED_FREE(...)	std::free(__VA_ARGS__)
#endif

#include <memory>
#define SFG_MEMCPY(...)	 memcpy(__VA_ARGS__)
#define SFG_MEMMOVE(...) memmove(__VA_ARGS__)
#define SFG_MEMSET(...)	 memset(__VA_ARGS__)
#define SFG_MALLOC(...)	 malloc(__VA_ARGS__)
#define SFG_FREE(...)	 free(__VA_ARGS__)

namespace sfg
{

}
