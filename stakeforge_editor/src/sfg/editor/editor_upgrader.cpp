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

#include "editor/editor_upgrader.hpp"

#include "editor/editor_directories.hpp"
#include "editor/editor_project.hpp"
#include "editor/editor_settings.hpp"
#include "editor/assets/editor_asset.hpp"

#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/runtime/project/project_package_meta.hpp>
#include <sfg/core/runtime/resources/audio.hpp>
#include <sfg/core/runtime/resources/font.hpp>
#include <sfg/core/runtime/resources/mesh.hpp>
#include <sfg/core/runtime/resources/skeleton.hpp>
#include <sfg/core/runtime/resources/animation.hpp>
#include <sfg/core/runtime/resources/material.hpp>
#include <sfg/core/runtime/resources/shader.hpp>
#include <sfg/core/runtime/resources/texture.hpp>
#include <sfg/core/runtime/resources/texture_sampler.hpp>
#include <sfg/core/runtime/resources/physical_material.hpp>
#include <sfg/core/runtime/resources/prefab.hpp>
#include <sfg/core/runtime/resources/cubemap.hpp>
#include <sfg/core/runtime/resources/physics_collision_mesh.hpp>
#include <sfg/core/runtime/resources/sprite.hpp>
#include <sfg/core/runtime/resources/curve.hpp>
#include <sfg/core/runtime/resources/ragdoll.hpp>
#include <sfg/core/runtime/resources/animation_library.hpp>
#include <sfg/core/runtime/resources/texture_cook.hpp>
#include <sfg/core/runtime/resources/mesh_cook.hpp>
#include <sfg/core/runtime/resources/animation_cook.hpp>
#include <sfg/core/runtime/resources/physics_collision_mesh_cook.hpp>
#include <sfg/core/runtime/resources/world_cook.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool editor_upgrader_t::validate_editor_settings()
	{
		const string_t settings = editor_directories_t::get_editor_settings();

		if (!file_system_t::exists(settings.c_str()))
		{
			editor_settings_t defaults = {};

			if (!defaults.save())
				return false;

			editor_settings_t::get() = std::move(defaults);

			return true;
		}

		const string_t json_text = file_system_t::read_file_as_string(settings.c_str());
		nlohmann::json json		 = nlohmann::json::parse(json_text, nullptr, false);

		if (json.is_discarded())
		{
			SFG_ERR("failed to parse editor settings {0}", settings);
			return false;
		}

		editor_settings_t loaded = {};

		try
		{
			const u32 wire_from_json = json.value<u32>("wire_version", 0);

			if (editor_settings_t::WIRE_VERSION != wire_from_json)
			{
			}

			json.get_to(loaded);
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to deserialize editor settings {0}: {1}", settings, error.what());
			return false;
		}

		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
		{
			const editor_panel_window_size_t& size	= loaded.panel_window_sizes[i];
			const bool						  unset = size.width == 0.0f && size.height == 0.0f;
			const bool						  valid = size.width > 0.0f && size.width <= 65535.0f && size.height > 0.0f && size.height <= 65535.0f;

			if (!unset && !valid)
			{
				SFG_ERR("invalid window size for panel {0} in editor settings {1}", editor_panel_type_to_string(static_cast<editor_panel_type_e>(i)), settings);
				return false;
			}
		}

		editor_settings_t::get() = std::move(loaded);

		return true;
	}

	void editor_upgrader_t::validate_project_file(nlohmann::json& json)
	{
		const u32 wire_from_json = json.value<u32>("wire_version", 0);

		if (editor_project_t::WIRE_VERSION != wire_from_json)
		{
		}
	}

	bool editor_upgrader_t::validate_project_assets()
	{
		const string_t& path	 = editor_project_t::get()._runtime.path;
		const string_t	contents = file_system_t::read_file_as_string(path.c_str());
		nlohmann::json	json	 = nlohmann::json::parse(contents, nullptr, false);

		if (json.is_discarded())
		{
			SFG_ERR("failed to parse project versions {0}", path);
			return false;
		}

		try
		{
			const u32 asset_wire_version = json.value<u32>("asset_wire_version", 0);

			if (editor_asset_t::WIRE_VERSION != asset_wire_version)
			{
			}

			const u32 audio_wire_version = json.value<u32>("audio_wire_version", 0);

			if (audio_loader_t::WIRE_VERSION != audio_wire_version)
			{
			}

			const u32 font_wire_version = json.value<u32>("font_wire_version", 0);

			if (font_loader_t::WIRE_VERSION != font_wire_version)
			{
			}

			const u32 mesh_wire_version = json.value<u32>("mesh_wire_version", 0);

			if (mesh_loader_t::WIRE_VERSION != mesh_wire_version)
			{
			}

			const u32 skeleton_wire_version = json.value<u32>("skeleton_wire_version", 0);

			if (skeleton_loader_t::WIRE_VERSION != skeleton_wire_version)
			{
			}

			const u32 animation_wire_version = json.value<u32>("animation_wire_version", 0);

			if (animation_loader_t::WIRE_VERSION != animation_wire_version)
			{
			}

			const u32 material_wire_version = json.value<u32>("material_wire_version", 0);

			if (material_loader_t::WIRE_VERSION != material_wire_version)
			{
			}

			const u32 shader_wire_version = json.value<u32>("shader_wire_version", 0);

			if (shader_loader_t::WIRE_VERSION != shader_wire_version)
			{
			}

			const u32 texture_wire_version = json.value<u32>("texture_wire_version", 0);

			if (texture_loader_t::WIRE_VERSION != texture_wire_version)
			{
			}

			const u32 texture_sampler_wire_version = json.value<u32>("texture_sampler_wire_version", 0);

			if (texture_sampler_loader_t::WIRE_VERSION != texture_sampler_wire_version)
			{
			}

			const u32 physical_material_wire_version = json.value<u32>("physical_material_wire_version", 0);

			if (physical_material_loader_t::WIRE_VERSION != physical_material_wire_version)
			{
			}

			const u32 prefab_wire_version = json.value<u32>("prefab_wire_version", 0);

			if (prefab_loader_t::WIRE_VERSION != prefab_wire_version)
			{
			}

			const u32 cubemap_wire_version = json.value<u32>("cubemap_wire_version", 0);

			if (cubemap_loader_t::WIRE_VERSION != cubemap_wire_version)
			{
			}

			const u32 physics_collision_mesh_wire_version = json.value<u32>("physics_collision_mesh_wire_version", 0);

			if (physics_collision_mesh_loader_t::WIRE_VERSION != physics_collision_mesh_wire_version)
			{
			}

			const u32 sprite_wire_version = json.value<u32>("sprite_wire_version", 0);

			if (sprite_loader_t::WIRE_VERSION != sprite_wire_version)
			{
			}

			const u32 curve_wire_version = json.value<u32>("curve_wire_version", 0);

			if (curve_loader_t::WIRE_VERSION != curve_wire_version)
			{
			}

			const u32 ragdoll_wire_version = json.value<u32>("ragdoll_wire_version", 0);

			if (ragdoll_loader_t::WIRE_VERSION != ragdoll_wire_version)
			{
			}

			const u32 animation_library_wire_version = json.value<u32>("animation_library_wire_version", 0);

			if (animation_library_loader_t::WIRE_VERSION != animation_library_wire_version)
			{
			}

			const u32 world_wire_version = json.value<u32>("world_wire_version", 0);

			if (world_cooker_t::WIRE_VERSION != world_wire_version)
			{
			}

			const u32 texture_blob_wire_version = json.value<u32>("texture_blob_wire_version", 0);

			if (texture_cooker::BLOB_WIRE_VERSION != texture_blob_wire_version)
			{
			}

			const u32 mesh_blob_wire_version = json.value<u32>("mesh_blob_wire_version", 0);

			if (mesh_cooker::BLOB_WIRE_VERSION != mesh_blob_wire_version)
			{
			}

			const u32 animation_blob_wire_version = json.value<u32>("animation_blob_wire_version", 0);

			if (animation_cooker::BLOB_WIRE_VERSION != animation_blob_wire_version)
			{
			}

			const u32 physics_collision_mesh_blob_wire_version = json.value<u32>("physics_collision_mesh_blob_wire_version", 0);

			if (physics_collision_mesh_cooker::BLOB_WIRE_VERSION != physics_collision_mesh_blob_wire_version)
			{
			}

			const u32 project_package_wire_version = json.value<u32>("project_package_wire_version", 0);

			if (project_package_meta_t::WIRE_VERSION != project_package_wire_version)
			{
			}

			const u32 resource_stream_wire_version = json.value<u32>("resource_stream_wire_version", 0);

			if (project_package_meta_t::RESOURCE_STREAM_WIRE_VERSION != resource_stream_wire_version)
			{
			}
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to deserialize project versions {0}: {1}", path, error.what());
			return false;
		}

		return true;
	}

	void editor_upgrader_t::write_project_wire_versions(nlohmann::json& json)
	{
		json["wire_version"]							 = editor_project_t::WIRE_VERSION;
		json["asset_wire_version"]						 = editor_asset_t::WIRE_VERSION;
		json["audio_wire_version"]						 = audio_loader_t::WIRE_VERSION;
		json["font_wire_version"]						 = font_loader_t::WIRE_VERSION;
		json["mesh_wire_version"]						 = mesh_loader_t::WIRE_VERSION;
		json["skeleton_wire_version"]					 = skeleton_loader_t::WIRE_VERSION;
		json["animation_wire_version"]					 = animation_loader_t::WIRE_VERSION;
		json["material_wire_version"]					 = material_loader_t::WIRE_VERSION;
		json["shader_wire_version"]						 = shader_loader_t::WIRE_VERSION;
		json["texture_wire_version"]					 = texture_loader_t::WIRE_VERSION;
		json["texture_sampler_wire_version"]			 = texture_sampler_loader_t::WIRE_VERSION;
		json["physical_material_wire_version"]			 = physical_material_loader_t::WIRE_VERSION;
		json["prefab_wire_version"]						 = prefab_loader_t::WIRE_VERSION;
		json["cubemap_wire_version"]					 = cubemap_loader_t::WIRE_VERSION;
		json["physics_collision_mesh_wire_version"]		 = physics_collision_mesh_loader_t::WIRE_VERSION;
		json["sprite_wire_version"]						 = sprite_loader_t::WIRE_VERSION;
		json["curve_wire_version"]						 = curve_loader_t::WIRE_VERSION;
		json["ragdoll_wire_version"]					 = ragdoll_loader_t::WIRE_VERSION;
		json["animation_library_wire_version"]			 = animation_library_loader_t::WIRE_VERSION;
		json["world_wire_version"]						 = world_cooker_t::WIRE_VERSION;
		json["texture_blob_wire_version"]				 = texture_cooker::BLOB_WIRE_VERSION;
		json["mesh_blob_wire_version"]					 = mesh_cooker::BLOB_WIRE_VERSION;
		json["animation_blob_wire_version"]				 = animation_cooker::BLOB_WIRE_VERSION;
		json["physics_collision_mesh_blob_wire_version"] = physics_collision_mesh_cooker::BLOB_WIRE_VERSION;
		json["project_package_wire_version"]			 = project_package_meta_t::WIRE_VERSION;
		json["resource_stream_wire_version"]			 = project_package_meta_t::RESOURCE_STREAM_WIRE_VERSION;
	}
}
