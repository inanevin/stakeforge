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

#include "assets/editor_asset_builtin_types.hpp"
#include "assets/editor_asset_node.hpp"
#include "assets/editor_asset_type.hpp"

namespace sfg
{
	enum class shader_type_e : u8;
	struct editor_asset_t;

	enum class editor_script_template_e : u8
	{
		invalid,
		component,
		world_script,
		class_script,
	};

	struct editor_asset_create_desc_t
	{
		editor_asset_node_handle_t		parent_node			   = {};
		const char*						name				   = nullptr;
		const char*						source_name			   = nullptr;
		const char*						embedded_data		   = nullptr;
		sid_t							guid				   = NULL_SID;
		editor_asset_type_e				asset_type			   = editor_asset_type_e::invalid;
		editor_object_shader_template_e object_shader_template = editor_object_shader_template_e::lit;
		u8								sub_type			   = 0;
		bool							allow_overwrite		   = false;
	};

	struct editor_script_create_desc_t
	{
		editor_asset_node_handle_t parent_node	   = {};
		const char*				   name			   = nullptr;
		editor_script_template_e   script_template = editor_script_template_e::invalid;
		bool					   allow_overwrite = false;
	};

	class editor_asset_creator_t final
	{
	public:
		editor_asset_creator_t()										 = delete;
		~editor_asset_creator_t()										 = delete;
		editor_asset_creator_t(const editor_asset_creator_t&)			 = delete;
		editor_asset_creator_t& operator=(const editor_asset_creator_t&) = delete;

		static bool create_asset(const editor_asset_create_desc_t& desc, editor_asset_t* out_asset = nullptr);
		static bool create_script(const editor_script_create_desc_t& desc, string_t* out_path = nullptr);
	};
}
