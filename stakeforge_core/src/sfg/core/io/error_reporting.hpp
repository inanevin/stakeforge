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

#include "core/common/sfg_exports.hpp"
#include "core/data/mutex.hpp"
#include "core/data/string.hpp"
#include "core/common/size_definitions.hpp"

namespace sfg
{
	enum class log_source_e : u8;
	enum class log_level;

	class SFG_API error_reporting_t final
	{
	public:
		error_reporting_t()									   = default;
		~error_reporting_t()								   = default;
		error_reporting_t(const error_reporting_t&)			   = delete;
		error_reporting_t& operator=(const error_reporting_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(const char* application);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool			  set_project_name(const char* project_name);
		[[noreturn]] void report_crash(const char* reason) noexcept;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		static error_reporting_t& get()
		{
			static error_reporting_t instance = {};

			return instance;
		}

		const string_t& get_log_path() const
		{
			return _log_path;
		}

	private:
		static void on_log(log_source_e source, log_level level, const char* message, void* user_data);
		bool		write_line(const char* level, const char* message) noexcept;
		void		close_file();

	private:
		mutex_t	 _mutex		= {};
		string_t _filename	= {};
		string_t _log_path	= {};
		void*	 _file		= nullptr;
		bool	 _io_failed = false;
	};
}
