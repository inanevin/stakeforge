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

#include <sfg/audio/audio_engine.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

namespace sfg
{
	struct engine_global_config_t
	{
		resource_manager_config_t resource_manager = {};
		audio_engine_config_t	  audio			   = {};
	};

	struct engine_backend_config_t
	{
		gfx_backend_config_t	  gfx			   = {};
		render_resources_config_t render_resources = {};
		ui::glyph_atlas_config_t  glyph_atlas	   = {};
	};

	struct engine_runtime_config_t
	{
		engine_global_config_t	global	= {};
		engine_backend_config_t backend = {};
	};
}
