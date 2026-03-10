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

#include "data/vector.hpp"
#include "data/string.hpp"
#include "common/size_definitions.hpp"

namespace SFG
{
	struct monitor_info;

	enum character_mask
	{
		letter	   = 1 << 0,
		number	   = 1 << 1,
		separator  = 1 << 2,
		symbol	   = 1 << 4,
		whitespace = 1 << 5,
		control	   = 1 << 6,
		printable  = 1 << 7,
		op		   = 1 << 8,
		sign	   = 1 << 9,
	};

	class process
	{
	public:
		static void	  init();
		static void	  uninit();
		static void	  pump_os_messages();
		static void	  open_url(const char* url);
		static bool	  open_directory(const char* dir);
		static void	  message_box(const char* msg);
		static void	  get_all_monitors(vector<monitor_info>& out);
		static char	  get_character_from_key(u32 key);
		static u16	  get_character_mask_from_key(u32 key, char ch);
		static string select_folder(const char* title);
		static string select_file(const char* title, const char* extension);
		static void	  select_files(const char* title, const char* extension, vector<string>& out_files);
		static string save_file(const char* title, const char* extension);
		static string get_clipboard();
		static void	  push_clipboard(const char* cp);

	private:
	};

} // namespace SFG
