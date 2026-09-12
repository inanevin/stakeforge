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

#include "vector.hpp"
#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	namespace vector_util
	{

		template <typename T, class Predicate> inline bool find(const vector_t<T>& vec, T& outValue, Predicate pred)
		{
			auto it = std::find_if(vec.begin(), vec.end(), pred);
			if (it != vec.end())
			{
				outValue = *it;
				return true;
			}

			return false;
		}

		template <typename T, class Predicate> inline typename vector_t<T>::const_iterator find_if(const vector_t<T>& vec, Predicate pred)
		{
			return std::find_if(vec.cbegin(), vec.cend(), pred);
		}

		template <typename T, class Predicate> inline typename vector_t<T>::iterator find_if(vector_t<T>& vec, Predicate pred)
		{
			return std::find_if(vec.begin(), vec.end(), pred);
		}

		template <typename T, class Predicate> inline u32 erase_if(vector_t<T>& vec, Predicate pred)
		{
			return static_cast<u32>(std::erase_if(vec, pred));
		}

		template <typename T> inline typename vector_t<T>::iterator remove(vector_t<T>& vec, T& value)
		{
			return vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
		}

		template <typename T> inline i32 index_of(const vector_t<T>& vec, const T& value)
		{
			const i32 sz = static_cast<i32>(vec.size());

			for (i32 i = 0; i < sz; ++i)
			{
				if (vec[i] == value)
					return i;
			}

			return -1;
		}

		template <typename T> inline i32 find_next_index_if_removed(const vector_t<T>& vec, const T& value)
		{
			const i32 currentIndex = index_of(vec, value);

			if (currentIndex == 0)
			{
				if (vec.size() == 1)
					return -1;

				return 0;
			}

			return currentIndex - 1;
		}

		template <typename T> inline void place_after(vector_t<T>& vec, T& src, T& target)
		{
			auto itSrc	  = std::find_if(vec.begin(), vec.end(), [src](const T& child) { return child == src; });
			auto itTarget = std::find_if(vec.begin(), vec.end(), [target](const T& child) { return child == target; });
			vec.erase(itSrc);
			vec.insert(itTarget + 1, *itSrc);
		}

		template <typename T> inline void place_before(vector_t<T>& vec, const T& src, const T& target)
		{
			auto itSrc	  = std::find_if(vec.begin(), vec.end(), [src](const T& child) { return child == src; });
			auto itTarget = std::find_if(vec.begin(), vec.end(), [target](const T& child) { return child == target; });

			vec.insert(itTarget, *itSrc);
			if (itSrc < itTarget)
				vec.erase(itSrc);
			else
				vec.erase(itSrc + 1);
		}

		template <typename T> inline bool contains(const vector_t<T>& vec, const T& data)
		{
			for (const auto& v : vec)
			{
				if (v == data)
					return true;
			}

			return false;
		}

	} // namespace Utilvector
}
