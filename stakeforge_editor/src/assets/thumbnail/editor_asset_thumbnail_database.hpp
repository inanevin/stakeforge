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

#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/moodycamel/readerwriterqueue.h>

namespace sfg
{
	struct editor_asset_t;

	class editor_asset_thumbnail_database_t final
	{
	public:
		editor_asset_thumbnail_database_t()													   = default;
		~editor_asset_thumbnail_database_t()												   = default;
		editor_asset_thumbnail_database_t(const editor_asset_thumbnail_database_t&)			   = delete;
		editor_asset_thumbnail_database_t& operator=(const editor_asset_thumbnail_database_t&) = delete;

		static inline editor_asset_thumbnail_database_t& get()
		{
			static editor_asset_thumbnail_database_t s_instance;
			return s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick();
		void load_all_ready();
		void request_generated(sid_t asset_guid);
		void request_render(sid_t asset_guid);

	private:
		enum class request_kind_e : u8
		{
			generated,
			render,
		};

		struct request_t
		{
			sid_t		   asset_guid = NULL_SID;
			request_kind_e kind		  = request_kind_e::generated;
		};

		struct pending_t
		{
			const editor_asset_t* asset			 = nullptr;
			sid_t				  thumbnail_guid = NULL_SID;
			request_kind_e		  kind			 = request_kind_e::generated;
		};

	private:
		void push_request(request_t request);
		void load_thumbnail(const editor_asset_t& asset);

	private:
		moodycamel::ReaderWriterQueue<request_t> _requests;
	};
}
