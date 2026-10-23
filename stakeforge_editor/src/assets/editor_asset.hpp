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

#include "assets/editor_asset_node.hpp"
#include "assets/editor_asset_type.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	enum class editor_asset_source_type_e : u8
	{
		none,
		file,
		file_blob,
		embedded,
	};

	enum class editor_asset_status_e : u8
	{
		ok,
		missing_dependency,
		missing_embedded_data,
		missing_file_source,
	};

	struct editor_asset_t
	{
		static constexpr u32 VERSION = 0;

		string_t				   embedded_source = {};
		string_t				   cook_options	   = "{}";
		string_t				   source_relative = {};
		u32						   version		   = 0;
		sid_t					   guid			   = NULL_SID;
		sid_t					   thumbnail_guid  = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		editor_asset_source_type_e source_type	   = editor_asset_source_type_e::file;
		editor_asset_status_e	   status		   = editor_asset_status_e::ok;
		u8						   sub_type		   = 0;
	};

	struct editor_asset_descriptor_t
	{
		vector_t<string_t>	extensions	 = {};
		string_t			display_name = {};
		vec4f_t				color		 = {};
		editor_asset_type_e asset_type	 = editor_asset_type_e::invalid;
	};

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t);
	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t);
	void to_json(nlohmann::json& j, const editor_asset_t& asset);
	void from_json(const nlohmann::json& j, editor_asset_t& asset);
}
