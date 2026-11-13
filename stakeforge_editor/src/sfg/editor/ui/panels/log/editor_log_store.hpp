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
#pragma once

#include "editor/editor_exports.hpp"
#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/mutex.hpp>

namespace sfg
{
	enum class log_level;
	enum class log_source_e : u8;

	struct editor_log_record_t
	{
		u64			 sequence	= 0;
		i64			 timestamp	= 0;
		u64			 text_begin = 0;
		u32			 text_size	= 0;
		log_level	 level		= {};
		log_source_e source		= {};
		bool		 truncated	= false;
	};

	class SFG_EDITOR_API editor_log_store_t final
	{
	public:
		editor_log_store_t()										   = default;
		~editor_log_store_t()										   = default;
		editor_log_store_t(const editor_log_store_t& other)			   = delete;
		editor_log_store_t& operator=(const editor_log_store_t& other) = delete;

		static constexpr u32 RECORD_CAPACITY = 192;
		static constexpr u32 TEXT_CAPACITY	 = 64 * 1024;
		static constexpr u32 MAX_TEXT_LENGTH = 4095;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void drain();
		void clear();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		static editor_log_store_t& get()
		{
			static editor_log_store_t instance = {};
			return instance;
		}

		u32 get_record_count() const
		{
			return _history.count;
		}
		u64 get_revision() const
		{
			return _revision;
		}
		u64 get_dropped_count() const
		{
			return _dropped_count;
		}
		u64 get_truncated_count() const
		{
			return _truncated_count;
		}
		const editor_log_record_t& get_record(u32 index) const
		{
			return _history.records[(_history.head + index) % RECORD_CAPACITY];
		}
		const char* get_text(const editor_log_record_t& record) const
		{
			return _history.text + record.text_begin % TEXT_CAPACITY;
		}

	private:
		struct buffer_t
		{
			editor_log_record_t records[RECORD_CAPACITY] = {};
			char				text[TEXT_CAPACITY]		 = {};
			u64					text_end				 = 0;
			u32					head					 = 0;
			u32					count					 = 0;

			u32	 append(editor_log_record_t record, const char* message);
			void reset();
		};

		static void on_log(log_source_e source, log_level level, const char* message, void* user_data);

	private:
		mutex_t	 _mutex					  = {};
		buffer_t _pending[2]			  = {};
		buffer_t _history				  = {};
		u64		 _next_sequence			  = 1;
		u64		 _revision				  = 0;
		u64		 _pending_dropped_count	  = 0;
		u64		 _pending_truncated_count = 0;
		u64		 _dropped_count			  = 0;
		u64		 _truncated_count		  = 0;
		u32		 _write_buffer			  = 0;
	};
}
