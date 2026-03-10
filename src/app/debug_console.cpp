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

#include "debug_console.hpp"

namespace SFG
{
	debug_console* debug_console::s_instance = nullptr;

	void debug_console::parse_console_command(const char* cmd)
	{
		string str = cmd;
		string_util::remove_whitespace(str);

		const size_t eq = str.find_first_of("=");

		if (eq != string::npos)
		{
			const string	command = str.substr(0, eq);
			const string_id sid		= TO_SID(command.c_str());
			auto			it		= _console_entries.find(sid);
			if (it == _console_entries.end())
			{
				SFG_ERR("debug_console::parse_console_command() -> command seems to be a variable, but it can't be found!");
				return;
			}
			const string args = str.substr(eq + 1, str.size() - eq - 1);
			it->second->execute(args);
			return;
		}

		const size_t bracket0 = str.find_first_of("(");
		const size_t bracket1 = str.find_first_of(")");

		if (bracket0 == string::npos || bracket1 == string::npos)
		{
			SFG_ERR("debug_console::parse_console_command() -> command is not a variable, and can't be recognize as a function because of missing bracket(s)");
			return;
		}

		const string command = str.substr(0, bracket0);

		const string_id sid = TO_SID(command.c_str());
		auto			it	= _console_entries.find(sid);
		if (it == _console_entries.end())
		{
			SFG_ERR("debug_console::parse_console_command() -> command seems to be a function, but can't be found!");
			return;
		}

		const string args = str.substr(bracket0 + 1, bracket1 - bracket0 - 1);
		it->second->execute(args);
	}

	void debug_console::unregister_console_function(const char* name)
	{
		const string_id sid = TO_SID(name);
		auto			it	= _console_entries.find(sid);
		if (it == _console_entries.end())
		{
			SFG_ERR("debug_console::unregister_console_function() -> can't find function! {0}", name);
			return;
		}

		delete it->second;
		_console_entries.erase(it);
	}

	void debug_console::init()
	{
		s_instance = new debug_console();
		s_instance->_console_entries.reserve(256);
	}

	void debug_console::uninit()
	{
		for (auto [sid, ptr] : s_instance->_console_entries)
			delete ptr;
		s_instance->_console_entries.clear();
		delete s_instance;
		s_instance = nullptr;
	}
}