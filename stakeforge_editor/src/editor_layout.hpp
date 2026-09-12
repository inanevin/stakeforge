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

#include <sfg/vendor/nhlohmann/json_fwd.hpp>

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	struct editor_surface_t;

	struct editor_layout_window_t
	{
		vec2i16_t pos		 = {64, 64};
		vec2u16_t size		 = {1920, 1080};
		bool	  is_primary = false;
		bool	  maximized	 = false;
		string_t  dock_layout;
		string_t  main_toolbar;
	};

	struct editor_layout_t
	{
		vector_t<editor_layout_window_t> windows;

		static void load_surface_default_layout(editor_surface_t& surface);
	};

	void to_json(nlohmann::json& j, const editor_layout_window_t& window);
	void from_json(const nlohmann::json& j, editor_layout_window_t& window);
	void to_json(nlohmann::json& j, const editor_layout_t& layout);
	void from_json(const nlohmann::json& j, editor_layout_t& layout);
}
