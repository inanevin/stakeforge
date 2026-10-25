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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <chrono>

#include "core/data/vector.hpp"
#include "core/data/fixed_vector.hpp"
#include "core/data/hash_map.hpp"
#include "core/data/string.hpp"
#include "core/data/mutex.hpp"
#include "core/data/string_view.hpp"
#include "core/data/frame_vector.hpp"
#include "core/data/frame_string.hpp"

#include "core/gfx/common/descriptions.hpp"
#include "core/gfx/common/shader_description.hpp"
#include "core/gfx/backend/backend.hpp"

#include "core/io/log.hpp"
#include "core/io/file_system.hpp"

#include "core/vendor/nhlohmann/json.hpp"
