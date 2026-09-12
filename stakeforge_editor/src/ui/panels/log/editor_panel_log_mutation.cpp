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
#include "ui/panels/log/editor_panel_log.hpp"
#include "ui/panels/log/editor_panel_log_internal.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_log_t::request_clear_logs()
	{
		clear_logs();
	}

	void editor_panel_log_t::request_collapse_rows()
	{
		collapse_existing_rows();
	}

}
