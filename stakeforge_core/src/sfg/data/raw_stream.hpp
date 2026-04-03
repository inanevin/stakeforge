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

#include "common/size_definitions.hpp"
#include "span.hpp"

namespace sfg
{
	class ostream_t;
	class istream_t;

	class raw_stream_t
	{
	public:
		raw_stream_t() : _data({}){};
		~raw_stream_t() = default;

		raw_stream_t(const raw_stream_t& other)			   = delete;
		raw_stream_t& operator=(const raw_stream_t& other) = delete;

		void create(ostream_t& stream);
		void create(u8* data, size_t size);
		void destroy();
		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline span_t<u8> get_span()
		{
			return _data;
		}

		inline u8* get_raw() const
		{
			return _data.data;
		}

		inline size_t get_size() const
		{
			return _data.size;
		}

		bool is_empty() const
		{
			return _data.size == 0;
		}

	private:
		span_t<u8> _data;
	};

}
