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

#include "assets/editor_asset.hpp"
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_asset_write_file_desc_t
	{
		const nlohmann::json* cook_options			   = nullptr;
		const nlohmann::json* embedded_source		   = nullptr;
		const char*			  parent_path			   = nullptr;
		const char*			  name					   = nullptr;
		const char*			  source_name			   = nullptr;
		const char*			  source_extension		   = nullptr;
		const char*			  source_template_relative = nullptr;
		sid_t				  guid					   = NULL_SID;
		editor_asset_type_e	  asset_type			   = editor_asset_type_e::invalid;
		u8					  sub_type				   = 0;
		bool				  allow_overwrite		   = false;
	};

	struct editor_asset_write_existing_file_desc_t
	{
		const nlohmann::json*	   cook_options		= nullptr;
		const char*				   parent_path		= nullptr;
		const char*				   name				= nullptr;
		const char*				   source_full_path = nullptr;
		sid_t					   guid				= NULL_SID;
		editor_asset_type_e		   asset_type		= editor_asset_type_e::invalid;
		editor_asset_source_type_e source_type		= editor_asset_source_type_e::file;
		u8						   sub_type			= 0;
		bool					   allow_overwrite	= true;
	};

	struct editor_asset_write_embedded_desc_t
	{
		const nlohmann::json* embedded_source = nullptr;
		const char*			  parent_path	  = nullptr;
		const char*			  name			  = nullptr;
		sid_t				  guid			  = NULL_SID;
		editor_asset_type_e	  asset_type	  = editor_asset_type_e::invalid;
		u8					  sub_type		  = 0;
		bool				  allow_overwrite = false;
	};

	struct editor_asset_write_none_desc_t
	{
		const char*			parent_path		= nullptr;
		const char*			name			= nullptr;
		sid_t				guid			= NULL_SID;
		editor_asset_type_e asset_type		= editor_asset_type_e::invalid;
		u8					sub_type		= 0;
		bool				allow_overwrite = false;
	};

	class editor_asset_writer_t final
	{
	public:
		editor_asset_writer_t()										   = delete;
		~editor_asset_writer_t()									   = delete;
		editor_asset_writer_t(const editor_asset_writer_t&)			   = delete;
		editor_asset_writer_t& operator=(const editor_asset_writer_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool write_file_asset(const editor_asset_write_file_desc_t& desc, editor_asset_t* out_asset = nullptr, string_t* out_asset_path = nullptr);
		static bool write_existing_file_asset(const editor_asset_write_existing_file_desc_t& desc, editor_asset_t* out_asset = nullptr, string_t* out_asset_path = nullptr);
		static bool write_embedded_asset(const editor_asset_write_embedded_desc_t& desc, editor_asset_t* out_asset = nullptr, string_t* out_asset_path = nullptr);
		static bool write_none_source_asset(const editor_asset_write_none_desc_t& desc, editor_asset_t* out_asset = nullptr, string_t* out_asset_path = nullptr);
		static bool read_embedded_source(const char* asset_relative_path, nlohmann::json& out_embedded_source);
		static bool read_cook_options(const char* asset_relative_path, nlohmann::json& out_cook_options);
	};
}
