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

#include "game_plugin_render.hpp"

#include <sfg/core/runtime/plugin/game_plugin_api.hpp>
#include <sfg/core/runtime/plugin/game_plugin_render_context.hpp>

namespace game
{
	sfg::game_plugin_render_snapshot_config_t game_plugin_render_t::get_render_snapshot_config()
	{
		return {};
	}

	void game_plugin_render_t::produce_render_snapshot(sfg::world_t& world, void* snapshot_data)
	{
	}

	void game_plugin_render_t::render_after_gbuffer(const sfg::game_plugin_render_context_t& context, const sfg::game_plugin_gbuffer_pass_t& targets)
	{
	}

	void game_plugin_render_t::render_before_forward(const sfg::game_plugin_render_context_t& context, const sfg::game_plugin_color_pass_t& targets)
	{
	}

	void game_plugin_render_t::render_before_post_process(const sfg::game_plugin_render_context_t& context, const sfg::game_plugin_color_pass_t& targets)
	{
	}

	void game_plugin_render_t::render_before_tonemap(const sfg::game_plugin_render_context_t& context, const sfg::game_plugin_color_pass_t& targets)
	{
	}

	void game_plugin_render_t::render_after_post_process(const sfg::game_plugin_render_context_t& context, const sfg::game_plugin_color_pass_t& targets)
	{
	}
}
