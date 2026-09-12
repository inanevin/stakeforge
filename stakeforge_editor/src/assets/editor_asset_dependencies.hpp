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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/resource_type.hpp>

namespace sfg
{
	struct editor_asset_t;

	struct editor_asset_dependency_t
	{
		sid_t			sid	 = NULL_SID;
		resource_type_e type = resource_type_e::invalid;
	};

	class editor_asset_dependencies_t final
	{
	public:
		editor_asset_dependencies_t()											   = delete;
		~editor_asset_dependencies_t()											   = delete;
		editor_asset_dependencies_t(const editor_asset_dependencies_t&)			   = delete;
		editor_asset_dependencies_t& operator=(const editor_asset_dependencies_t&) = delete;

		static bool fetch_dependencies(const editor_asset_t& asset, vector_t<editor_asset_dependency_t>& out_dependencies);
	};
}
