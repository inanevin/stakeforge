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

#include "tools/tools_exports.hpp"

#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/string.hpp>
#include <sfg/core/data/vector.hpp>

namespace sfg
{
	class resource_manager_t;

	class SFG_TOOLS_API editor_resource_preloader_t final
	{
	public:
		editor_resource_preloader_t()													 = default;
		~editor_resource_preloader_t()													 = default;
		editor_resource_preloader_t(const editor_resource_preloader_t& other)			 = delete;
		editor_resource_preloader_t& operator=(const editor_resource_preloader_t& other) = delete;

		struct init_params_t
		{
			string_t manifest_path;
			string_t assets_dir;
			string_t cache_dir;
		};

		bool init(resource_manager_t& mgr, const init_params_t& params);
		void uninit();

	private:
		vector_t<sid_t>		_loaded;
		resource_manager_t* _mgr = nullptr;
	};
}
