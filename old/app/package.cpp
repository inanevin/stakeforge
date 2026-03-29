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

#include "package.hpp"
#include "serialization/serialization.hpp"
#include "data/ostream.hpp"
#include "io/log.hpp"

namespace SFG
{
	package::~package()
	{
		close();
		if (_write_data.get_size() != 0)
			_write_data.destroy();
	}

	void package::open(const char* package_file)
	{
		close();
		_read_data = serialization::load_from_file(package_file);
		_header.deserialize(_read_data);
		_read_header_size = _read_data.tellg();
		_read_data.seek(0);
	}

	void package::close()
	{
		if (_read_data.get_size() == 0)
			return;

		_header = {};
		_read_data.destroy();
	}

	void package::start_writing()
	{
		SFG_INFO("beginning package");
		_header = {};
		if (_write_data.get_size() != 0)
			_write_data.destroy();
	}

	void package::write_resource(string_id id)
	{
		_header.resource_table[id] = {
			.offset = static_cast<u32>(_write_data.get_size()),
		};
	}

	void package::write_resource(const char* path)
	{
		SFG_INFO("wrote resource into package: {0}", path);
		write_resource(TO_SID(path));
	}

	void package::close_writing(const char* output_path)
	{
		SFG_INFO("saving package to: {0}", output_path);

		ostream package_content;
		_header.serialize(package_content);

		package_content.write_raw(_write_data.get_raw(), _write_data.get_size());
		_write_data.destroy();

		serialization::save_to_file(output_path, package_content);
		package_content.destroy();
	}

	ostream& package::get_write_stream()
	{
		return _write_data;
	}

	const package_entry& package::get_entry(const char* relative)
	{
		return get_entry(TO_SID(relative));
	}

	const package_entry& package::get_entry(string_id sid)
	{
		return _header.resource_table.at(sid);
	}

	bool package::get_stream(const char* entry_begin_relative, istream& out)
	{
		const string_id sid = TO_SID(entry_begin_relative);
		return get_stream(sid, out);
	}

	bool package::get_stream(string_id sid, istream& out)
	{
		auto it = _header.resource_table.find(sid);
		if (it == _header.resource_table.end())
			return false;
		const size_t off = _read_header_size + static_cast<size_t>(it->second.offset);

		if (off > _read_data.get_size())
			return false;

		out = istream(_read_data.get_raw() + off, _read_data.get_size() - off);
		return true;
	}

	void package_header::serialize(ostream& stream) const
	{
		const u32 entry_count = static_cast<u32>(resource_table.size());
		stream << entry_count;

		for (const auto& [sid, e] : resource_table)
		{
			stream << sid;
			stream << e.offset;
		}
	}

	void package_header::deserialize(istream& stream)
	{
		resource_table.clear();

		u32 entry_count = 0;
		stream >> entry_count;

		for (u32 i = 0; i < entry_count; i++)
		{
			package_entry e	  = {};
			string_id	  sid = 0;
			stream >> sid;
			stream >> e.offset;
			resource_table[sid] = e;
		}
	}
}
