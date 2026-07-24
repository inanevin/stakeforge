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

#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation_cook.hpp>
#include <sfg/runtime/resources/animation_def.hpp>
#include <sfg/runtime/resources/animation_graph_cook.hpp>
#include <sfg/runtime/resources/animation_graph_def.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/material_cook.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/mesh_cook.hpp>
#include <sfg/runtime/resources/font_cook.hpp>
#include <sfg/runtime/resources/physical_material_cook.hpp>
#include <sfg/runtime/resources/physical_material_def.hpp>
#include <sfg/runtime/resources/physics_collision_mesh_cook.hpp>
#include <sfg/runtime/resources/prefab.hpp>
#include <sfg/runtime/resources/shader_cook.hpp>
#include <sfg/runtime/resources/skeleton_cook.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/resources/skybox_hdr_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/texture_sampler_cook.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		string_t resolve_asset_name(const char* asset_name)
		{
			string_t resolved = asset_name;
			file_system_t::fix_path(resolved);
			if (resolved.find('/') != string_t::npos)
				resolved = file_system_t::get_filename_from_path(resolved);
			else if (file_system_t::get_file_extension(resolved) == "sfg_asset")
				resolved = file_system_t::remove_extensions_from_path(resolved);
			return resolved;
		}

		bool save_cooked_asset(const editor_asset_t& asset, resource_header_t header, const ostream_t& payload, const char* asset_name)
		{
			const string_t cache_dir  = editor_project_t::get()._runtime.cache_path;
			const string_t cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.guid);
			if (!file_system_t::ensure_directory(cache_dir.c_str()))
			{
				SFG_ERR("failed to create asset cache directory {0}", cache_dir.c_str());
				return false;
			}

			string_t resolved_asset_name = {};
			if (asset_name != nullptr && asset_name[0] != '\0')
				resolved_asset_name = resolve_asset_name(asset_name);
			else if (const char* display_name = editor_asset_util_t::find_asset_display_name(asset.guid); display_name != nullptr && display_name[0] != '\0')
				resolved_asset_name = display_name;

			header.set_debug_name(resolved_asset_name.c_str());
			if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
			{
				const string_t source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
				header.file_source_ticks		= file_system_t::get_last_modified_ticks(source_full_path.c_str());
			}

			ostream_t stream = header.make_stream(payload);
			if (!serializer_t::save_to_file(cache_path.c_str(), stream))
			{
				SFG_ERR("failed to save cooked asset {0}", cache_path.c_str());
				return false;
			}

			editor_asset_thumbnailer_t::generate_thumbnail(asset, resolved_asset_name.c_str());

			return true;
		}
	}

	bool editor_asset_cooker_t::cook_asset(const editor_asset_t& asset, const char* asset_name)
	{
		switch (asset.asset_type)
		{
		case editor_asset_type_e::audio:
			return cook_audio(asset, asset_name);
		case editor_asset_type_e::material:
			return cook_material(asset, asset_name);
		case editor_asset_type_e::mesh:
			return cook_mesh(asset, asset_name);
		case editor_asset_type_e::physics_collision_mesh:
			return cook_physics_collision_mesh(asset, asset_name);
		case editor_asset_type_e::shader:
			return cook_shader(asset, asset_name);
		case editor_asset_type_e::skeleton:
			return cook_skeleton(asset, asset_name);
		case editor_asset_type_e::animation:
			return cook_animation(asset, asset_name);
		case editor_asset_type_e::texture:
			return cook_texture(asset, asset_name);
		case editor_asset_type_e::texture_sampler:
			return cook_texture_sampler(asset, asset_name);
		case editor_asset_type_e::physical_material:
			return cook_physical_material(asset, asset_name);
		case editor_asset_type_e::animation_graph:
			return cook_animation_graph(asset, asset_name);
		case editor_asset_type_e::hdr_skybox:
			return cook_hdr_skybox(asset, asset_name);
		case editor_asset_type_e::font:
			return cook_font(asset, asset_name);
		case editor_asset_type_e::prefab:
			return cook_prefab(asset, asset_name);
		default:
			SFG_ASSERT(false);
			return false;
		}
	}

	bool editor_asset_cooker_t::is_cookable(editor_asset_type_e asset_type)
	{
		switch (asset_type)
		{
		case editor_asset_type_e::audio:
		case editor_asset_type_e::material:
		case editor_asset_type_e::mesh:
		case editor_asset_type_e::physics_collision_mesh:
		case editor_asset_type_e::shader:
		case editor_asset_type_e::skeleton:
		case editor_asset_type_e::animation:
		case editor_asset_type_e::texture:
		case editor_asset_type_e::texture_sampler:
		case editor_asset_type_e::physical_material:
		case editor_asset_type_e::animation_graph:
		case editor_asset_type_e::hdr_skybox:
		case editor_asset_type_e::font:
		case editor_asset_type_e::prefab:
			return true;
		default:
			return false;
		}
	}

	bool editor_asset_cooker_t::is_asset_cooked(const editor_asset_t& asset)
	{
		if (!is_cookable(asset.asset_type))
			return false;

		const string_t cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.guid);
		if (!file_system_t::exists(cache_path.c_str()))
			return false;

		istream_t					stream		  = serializer_t::load_from_file_slice(cache_path.c_str(), 0, sizeof(resource_header_t));
		const resource_type_desc_t* resource_desc = find_resource_type_desc(static_cast<resource_type_e>(asset.asset_type));
		if (stream.empty())
			return false;

		resource_header_t header = {};
		header.deserialize(stream);
		if (header.type != resource_desc->type || header.magic != resource_desc->wire_magic || header.version != resource_desc->wire_version)
			return false;

		if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
		{
			const string_t source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
			return header.file_source_ticks == file_system_t::get_last_modified_ticks(source_full_path.c_str());
		}

		return true;
	}

	bool editor_asset_cooker_t::cook_audio(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::audio);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		audio_cook_config_t	 config		  = {};
		const nlohmann::json cook_options = editor_asset_io_t::get_cook_options_json(asset);
		if (!reflection_registry_t::get().type_from_json(type_id_t<audio_cook_config_t>::value, &config, nullptr, cook_options))
		{
			SFG_ERR("failed to deserialize audio cook options for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!audio_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
		{
			SFG_ERR("failed to cook audio asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_shader(const editor_asset_t& asset, const char* asset_name, shader_data_definition_t* out_definition)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::shader);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		shader_cook_config_t config		  = {};
		const nlohmann::json cook_options = editor_asset_io_t::get_cook_options_json(asset);
		if (!reflection_registry_t::get().type_from_json(type_id_t<shader_cook_config_t>::value, &config, nullptr, cook_options))
		{
			SFG_ERR("failed to deserialize shader cook options for asset {0}", asset.guid);
			return false;
		}

		resource_header_t		 header = {};
		ostream_t				 stream;
		const string_t			 source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		shader_data_definition_t definition		  = {};
		if (!shader_cooker::cook_from_file(config, source_full_path.c_str(), header, stream, definition))
		{
			SFG_ERR("failed to cook shader asset {0}", asset.guid);
			return false;
		}
		if (out_definition != nullptr)
			*out_definition = definition;

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_material(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::material);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		material_def_t		 def			 = {};
		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);
		embedded_source.get_to(def);

		resource_header_t header = {};
		ostream_t		  stream;
		if (!material_cooker::cook_from_def(def, header, stream))
		{
			SFG_ERR("failed to cook material asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_texture_sampler(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::texture_sampler);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		sampler_desc_t		 desc			 = {};
		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);
		if (!reflection_registry_t::get().type_from_json(type_id_t<sampler_desc_t>::value, &desc, nullptr, embedded_source))
		{
			SFG_ERR("failed to deserialize texture sampler description for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream;
		if (!texture_sampler_cooker::cook_from_desc(desc, header, stream))
		{
			SFG_ERR("failed to cook texture sampler asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_physical_material(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::physical_material);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		physical_material_def_t def				= {};
		const nlohmann::json	embedded_source = editor_asset_io_t::get_embedded_source_json(asset);
		if (!reflection_registry_t::get().type_from_json(type_id_t<physical_material_def_t>::value, &def, nullptr, embedded_source))
		{
			SFG_ERR("failed to deserialize physical material definition for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream;
		if (!physical_material_cooker::cook_from_def(def, header, stream))
		{
			SFG_ERR("failed to cook physical material asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_animation_graph(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::animation_graph);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		animation_graph_def_t def			  = {};
		const nlohmann::json  embedded_source = editor_asset_io_t::get_embedded_source_json(asset);

		if (!reflection_registry_t::get().type_from_json(type_id_t<animation_graph_def_t>::value, &def, nullptr, embedded_source))
		{
			SFG_ERR("failed to deserialize animation graph definition for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream = {};

		if (!animation_graph_cooker::cook_from_def(def, header, stream))
		{
			SFG_ERR("failed to cook animation graph asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_texture(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::texture);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob);

		texture_cook_config_t config	   = {};
		const nlohmann::json  cook_options = editor_asset_io_t::get_cook_options_json(asset);
		if (!reflection_registry_t::get().type_from_json(type_id_t<texture_cook_config_t>::value, &config, nullptr, cook_options))
		{
			SFG_ERR("failed to deserialize texture cook options for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream;

		const string_t source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);

		if (!texture_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
		{
			SFG_ERR("failed to cook texture asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_font(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::font);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		resource_header_t header = {};
		ostream_t		  stream;

		const string_t source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);

		if (!font_cooker::cook_from_file({}, source_full_path.c_str(), header, stream))
		{
			SFG_ERR("failed to cook font asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_skeleton(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::skeleton);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		skeleton_def_t		 def			 = {};
		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);

		if (!reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &def, nullptr, embedded_source))
		{
			SFG_ERR("failed to deserialize skeleton definition for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream;
		if (!skeleton_cooker::cook_from_def(def, header, stream))
		{
			SFG_ERR("failed to cook skeleton asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_animation(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::animation);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded || asset.source_type == editor_asset_source_type_e::file_blob);

		resource_header_t header = {};
		ostream_t		  stream = {};

		if (asset.source_type == editor_asset_source_type_e::file_blob)
		{
			const string_t source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);

			if (!animation_cooker::cook_from_file(source_full_path.c_str(), header, stream))
			{
				SFG_ERR("failed to cook animation asset {0}", asset.guid);
				return false;
			}
		}
		else
		{
			animation_def_t		 def			 = {};
			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);

			if (!reflection_registry_t::get().type_from_json(type_id_t<animation_def_t>::value, &def, nullptr, embedded_source))
			{
				SFG_ERR("failed to deserialize animation definition for asset {0}", asset.guid);
				return false;
			}

			if (!animation_cooker::cook_from_def(def, header, stream))
			{
				SFG_ERR("failed to cook animation asset {0}", asset.guid);
				return false;
			}
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_mesh(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::mesh);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file_blob);

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!mesh_cooker::cook_from_file(source_full_path.c_str(), header, stream))
		{
			SFG_ERR("failed to cook mesh asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_physics_collision_mesh(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::physics_collision_mesh);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file_blob);

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!physics_collision_mesh_cooker::cook_from_file(source_full_path.c_str(), header, stream))
		{
			SFG_ERR("failed to cook physics collision mesh asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_hdr_skybox(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::hdr_skybox);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		skybox_hdr_cook_config_t config		  = {};
		const nlohmann::json	 cook_options = editor_asset_io_t::get_cook_options_json(asset);
		if (!reflection_registry_t::get().type_from_json(type_id_t<skybox_hdr_cook_config_t>::value, &config, nullptr, cook_options))
		{
			SFG_ERR("failed to deserialize HDR skybox cook options for asset {0}", asset.guid);
			return false;
		}

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!skybox_hdr_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
		{
			SFG_ERR("failed to cook HDR skybox asset {0}", asset.guid);
			return false;
		}

		return save_cooked_asset(asset, header, stream, asset_name);
	}

	bool editor_asset_cooker_t::cook_prefab(const editor_asset_t& asset, const char* asset_name)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::prefab);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		const string_t	  prefab_source = asset.embedded_source;
		resource_header_t header		= {
			.type		 = resource_type_e::prefab,
			.magic		 = prefab_loader_t::WIRE_MAGIC,
			.version	 = prefab_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(reinterpret_cast<const u8*>(prefab_source.data()), prefab_source.size()),
		};

		ostream_t stream;
		stream << prefab_source;
		return save_cooked_asset(asset, header, stream, asset_name);
	}
}
