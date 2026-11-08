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

#include "world_renderer.hpp"
#include "core/runtime/render/world_render_context.hpp"

namespace sfg
{
	class SFG_API stock_world_renderer_t final : public world_renderer_t
	{
	public:
		stock_world_renderer_t()											   = default;
		~stock_world_renderer_t() override									   = default;
		stock_world_renderer_t(const stock_world_renderer_t& other)			   = delete;
		stock_world_renderer_t& operator=(const stock_world_renderer_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config) override;
		void uninit() override;

	private:
		world_render_context_t _render_context = {};
	};
}
