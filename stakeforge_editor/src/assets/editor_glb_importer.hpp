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

#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/texture_payload_type.hpp>

namespace sfg
{
	struct editor_asset_t;
	struct editor_asset_import_context_t;

	enum class glb_axis_e : u8
	{
		positive_x,
		negative_x,
		positive_y,
		negative_y,
		positive_z,
		negative_z,
	};

	struct glb_cook_config_t
	{
		texture_payload_type_e	   texture_payload_type = texture_payload_type_e::ktx2_uastc;
		texture_ktx2_compression_e ktx2_compression		= texture_ktx2_compression_e::default_quality;
		glb_axis_e				   source_up_axis		= glb_axis_e::positive_y;
		glb_axis_e				   source_forward_axis	= glb_axis_e::positive_z;
		bool					   import_textures		= true;
		bool					   import_materials		= true;
		bool					   import_animations	= true;
		bool					   import_meshes		= true;
		bool					   import_collisions	= true;
		bool					   combine_meshes		= false;
		bool					   generate_mipmaps		= false;

		bool is_basis_valid() const;
	};

	class editor_glb_importer_t final
	{
	public:
		editor_glb_importer_t()										   = delete;
		~editor_glb_importer_t()									   = delete;
		editor_glb_importer_t(const editor_glb_importer_t&)			   = delete;
		editor_glb_importer_t& operator=(const editor_glb_importer_t&) = delete;

		static bool import_glb(const char* target_directory, const char* source_full_path, const glb_cook_config_t& cook_config, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets, vector_t<string_t>& out_asset_paths);
	};

	SFG_DEFINE_TYPE_ID(glb_axis_e);
	SFG_DEFINE_TYPE_ID(glb_cook_config_t);

	struct glb_axis_reflection_t
	{
		glb_axis_reflection_t();
	};

	struct glb_cook_config_reflection_t
	{
		glb_cook_config_reflection_t();
	};

	inline glb_axis_reflection_t		g_reflect_glb_axis;
	inline glb_cook_config_reflection_t g_reflect_glb_cook_config;
}
