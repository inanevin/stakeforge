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

#include <sfg/data/string.hpp>
#include <sfg/common/size_definitions.hpp>
#include <sfg/io/log.hpp>
#include <sfg/io/assert.hpp>
#include <stdexcept>

namespace sfg
{
	inline string_t HrToString(HRESULT hr)
	{
		char s_str[64] = {};
		sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
		return string_t(s_str);
	}

	class HrException : public std::runtime_error
	{
	public:
		HrException(HRESULT hr) : std::runtime_error(HrToString(hr).c_str()), m_hr(hr)
		{
		}
		HRESULT Error() const
		{
			return m_hr;
		}

	private:
		const HRESULT m_hr;
	};

	inline void throw_if_failed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			SFG_FATAL("DX12 Exception! {0}", hr);
			DBG_BRK;
			throw HrException(hr);
		}
	}

}
