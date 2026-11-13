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
#include "editor_log_store.hpp"
#include <sfg/core/io/log.hpp>
#include <sfg/core/memory/memory.hpp>

namespace sfg
{
#define EDITOR_LOG_LISTENER_ID 0x0E100001u

	u32 editor_log_store_t::buffer_t::append(editor_log_record_t record, const char* message)
	{
		const u32 bytes	 = record.text_size + 1;
		u64		  begin	 = text_end;
		const u32 offset = static_cast<u32>(begin % TEXT_CAPACITY);
		if (offset + bytes > TEXT_CAPACITY)
			begin += TEXT_CAPACITY - offset;

		const u64 end	  = begin + bytes;
		u32		  evicted = 0;
		while (count != 0)
		{
			if (count < RECORD_CAPACITY && end - records[head].text_begin <= TEXT_CAPACITY)
				break;
			head = (head + 1) % RECORD_CAPACITY;
			--count;
			++evicted;
		}

		record.text_begin						  = begin;
		records[(head + count) % RECORD_CAPACITY] = record;
		char* destination						  = text + begin % TEXT_CAPACITY;
		SFG_MEMCPY(destination, message, record.text_size);
		destination[record.text_size] = '\0';
		text_end					  = end;
		++count;
		return evicted;
	}

	void editor_log_store_t::buffer_t::reset()
	{
		text_end = 0;
		head	 = 0;
		count	 = 0;
	}

	void editor_log_store_t::init()
	{
		log_t::instance().add_listener(EDITOR_LOG_LISTENER_ID, on_log, this);
	}

	void editor_log_store_t::uninit()
	{
		log_t::instance().remove_listener(EDITOR_LOG_LISTENER_ID);
		clear();
	}

	void editor_log_store_t::on_log(log_source_e source, log_level level, const char* message, void* user_data)
	{
		editor_log_store_t& store  = *static_cast<editor_log_store_t*>(user_data);
		u32					length = 0;
		while (length < MAX_TEXT_LENGTH && message[length] != '\0')
			++length;
		const bool truncated = message[length] != '\0';
		if (truncated)
		{
			while (length != 0 && (static_cast<u8>(message[length]) & 0xC0) == 0x80)
				--length;
		}

		const editor_log_record_t record{
			.timestamp = static_cast<i64>(std::time(nullptr)),
			.text_size = length,
			.level	   = level,
			.source	   = source,
			.truncated = truncated,
		};
		LOCK_GUARD(store._mutex);
		store._pending_dropped_count += store._pending[store._write_buffer].append(record, message);
		store._pending_truncated_count += truncated ? 1 : 0;
	}

	void editor_log_store_t::drain()
	{
		u32 read_buffer = 0;
		{
			LOCK_GUARD(_mutex);
			if (_pending[_write_buffer].count == 0)
				return;
			read_buffer = _write_buffer;
			_write_buffer ^= 1;
			_dropped_count += _pending_dropped_count;
			_truncated_count += _pending_truncated_count;
			_pending_dropped_count	 = 0;
			_pending_truncated_count = 0;
		}

		buffer_t& pending = _pending[read_buffer];
		for (u32 i = 0; i < pending.count; ++i)
		{
			editor_log_record_t record = pending.records[(pending.head + i) % RECORD_CAPACITY];
			record.sequence			   = _next_sequence++;
			_history.append(record, pending.text + record.text_begin % TEXT_CAPACITY);
		}
		pending.reset();
		++_revision;
	}

	void editor_log_store_t::clear()
	{
		{
			LOCK_GUARD(_mutex);
			_pending[_write_buffer].reset();
			_pending_dropped_count	 = 0;
			_pending_truncated_count = 0;
		}

		_history.reset();
		_dropped_count	 = 0;
		_truncated_count = 0;
		++_revision;
	}
}
