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
