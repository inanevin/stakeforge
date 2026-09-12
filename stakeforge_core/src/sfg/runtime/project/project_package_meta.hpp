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

#include "project_settings.hpp"

#include <sfg/data/hash_map.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

	struct world_meta_t
	{
		sid_t sid		= NULL_SID;
		sid_t name_hash = NULL_SID;
	};

	struct project_package_meta_t
	{
		static inline constexpr const char* FILE_NAME					 = "project_meta.sfg_bin";
		static inline constexpr const char* RESOURCE_FILE_NAME			 = "resources.sfg_bin";
		static inline constexpr u32			WIRE_MAGIC					 = make_resource_wire_magic('P', 'M', 'E', 'T');
		static inline constexpr u32			WIRE_VERSION				 = 9;
		static inline constexpr u32			RESOURCE_STREAM_WIRE_MAGIC	 = make_resource_wire_magic('R', 'S', 'T', 'R');
		static inline constexpr u32			RESOURCE_STREAM_WIRE_VERSION = 4;

		hash_map_t<sid_t, resource_map_info_t> resource_map			= {};
		project_settings_t					   project_settings		= {};
		vector_t<world_meta_t>				   worlds				= {};
		string_t							   script_assembly_name = {};
		world_meta_t						   main_world			= {};
		vec2u16_t							   window_resolution	= {1920, 1080};
		window_style_e						   window_style			= window_style_e::app_window;
		bool								   is_fullscreen		= false;

		bool serialize(ostream_t& stream) const;
		bool deserialize(istream_t& stream);
	};
}
