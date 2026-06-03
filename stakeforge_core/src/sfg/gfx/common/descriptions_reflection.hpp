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

#include "descriptions.hpp"
#include <sfg/common/hashing.hpp>

namespace sfg
{
	struct sampler_filter_reflection_t
	{
		static constexpr sid_t TYPE_ID = "sampler_filter_e"_hs;

		sampler_filter_reflection_t();
	};

	struct sampler_border_reflection_t
	{
		static constexpr sid_t TYPE_ID = "sampler_border_e"_hs;

		sampler_border_reflection_t();
	};

	struct address_mode_reflection_t
	{
		static constexpr sid_t TYPE_ID = "address_mode"_hs;

		address_mode_reflection_t();
	};

	struct sampler_desc_reflection_t
	{
		static constexpr sid_t TYPE_ID = "sampler_desc_t"_hs;

		sampler_desc_reflection_t();
	};

	inline sampler_filter_reflection_t g_reflect_sampler_filter;
	inline sampler_border_reflection_t g_reflect_sampler_border;
	inline address_mode_reflection_t   g_reflect_address_mode;
	inline sampler_desc_reflection_t   g_reflect_sampler_desc;
}
