/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "assets/editor_asset.hpp"

namespace sfg
{
	struct editor_asset_write_file_desc_t
	{
		const nlohmann::json*	   cook_options				= nullptr;
		editor_asset_node_handle_t parent_node				= {};
		const char*				   name						= nullptr;
		const char*				   source_name				= nullptr;
		const char*				   source_extension			= nullptr;
		const char*				   source_template_relative = nullptr;
		sid_t					   guid						= NULL_SID;
		editor_asset_type_e		   asset_type				= editor_asset_type_e::invalid;
		u8						   sub_type					= 0;
		bool					   allow_overwrite			= false;
	};

	struct editor_asset_write_embedded_desc_t
	{
		const nlohmann::json*	   embedded_source = nullptr;
		editor_asset_node_handle_t parent_node	   = {};
		const char*				   name			   = nullptr;
		sid_t					   guid			   = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		u8						   sub_type		   = 0;
		bool					   allow_overwrite = false;
	};

	struct editor_asset_write_none_desc_t
	{
		editor_asset_node_handle_t parent_node	   = {};
		const char*				   name			   = nullptr;
		sid_t					   guid			   = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		u8						   sub_type		   = 0;
		bool					   allow_overwrite = false;
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

		static bool write_file_asset(const editor_asset_write_file_desc_t& desc, editor_asset_t* out_asset = nullptr);
		static bool write_embedded_asset(const editor_asset_write_embedded_desc_t& desc, editor_asset_t* out_asset = nullptr);
		static bool write_none_source_asset(const editor_asset_write_none_desc_t& desc, editor_asset_t* out_asset = nullptr);
		static bool read_embedded_source(const char* asset_relative_path, nlohmann::json& out_embedded_source);
		static bool read_cook_options(const char* asset_relative_path, nlohmann::json& out_cook_options);

	private:
		static const editor_asset_node_t& get_parent_folder(editor_asset_node_handle_t parent_node);
	};
}
