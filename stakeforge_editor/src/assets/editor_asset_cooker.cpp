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
#include "editor_project.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/material_cook.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/mesh_cook.hpp>
#include <sfg/runtime/resources/physical_material_cook.hpp>
#include <sfg/runtime/resources/physical_material_def.hpp>
#include <sfg/runtime/resources/shader_cook.hpp>
#include <sfg/runtime/resources/skeleton_cook.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/resources/skybox_hdr_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/texture_sampler_cook.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
	namespace
	{
		bool save_cooked_asset(const editor_asset_t& asset, const resource_header_t& header, const ostream_t& payload)
		{
			const string_t cache_dir  = editor_project_t::get()._runtime.cache_path;
			const string_t cache_path = editor_asset_util_t::get_cache_path_for_asset(asset);
			if (!file_system_t::ensure_directory(cache_dir.c_str()))
				return false;

			ostream_t stream = make_resource_stream(header, payload);
			return serializer_t::save_to_file(cache_path.c_str(), stream);
		}
	}

	bool editor_asset_cooker_t::cook_asset(const editor_asset_t& asset)
	{
		switch (asset.asset_type)
		{
		case editor_asset_type_e::audio:
			return cook_audio(asset);
		case editor_asset_type_e::material:
			return cook_material(asset);
		case editor_asset_type_e::mesh:
			return cook_mesh(asset);
		case editor_asset_type_e::shader:
			return cook_shader(asset);
		case editor_asset_type_e::skeleton:
			return cook_skeleton(asset);
		case editor_asset_type_e::texture:
			return cook_texture(asset);
		case editor_asset_type_e::texture_sampler:
			return cook_texture_sampler(asset);
		case editor_asset_type_e::physical_material:
			return cook_physical_material(asset);
		case editor_asset_type_e::hdr_skybox:
			return cook_hdr_skybox(asset);
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
		case editor_asset_type_e::shader:
		case editor_asset_type_e::skeleton:
		case editor_asset_type_e::texture:
		case editor_asset_type_e::texture_sampler:
		case editor_asset_type_e::physical_material:
		case editor_asset_type_e::hdr_skybox:
			return true;
		default:
			return false;
		}
	}

	bool editor_asset_cooker_t::is_asset_cooked(const editor_asset_t& asset)
	{
		if (!is_cookable(asset.asset_type))
			return false;

		const string_t cache_path = editor_asset_util_t::get_cache_path_for_asset(asset);
		if (!file_system_t::exists(cache_path.c_str()))
			return false;

		istream_t					stream		  = serializer_t::load_from_file_slice(cache_path.c_str(), 0, sizeof(resource_header_t));
		const resource_type_desc_t* resource_desc = find_resource_type_desc(static_cast<resource_type_e>(asset.asset_type));
		SFG_ASSERT(resource_desc != nullptr);
		if (stream.empty())
			return false;

		resource_header_t header = {};
		header.deserialize(stream);
		return header.magic == resource_desc->wire_magic && header.version == resource_desc->wire_version;
	}

	bool editor_asset_cooker_t::cook_audio(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::audio);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		audio_cook_config_t config = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<audio_cook_config_t>::value, &config, asset.cook_options))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!audio_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_shader(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::shader);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		shader_cook_config_t config = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<shader_cook_config_t>::value, &config, asset.cook_options))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!shader_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_material(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::material);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		material_def_t def = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<material_def_t>::value, &def, asset.embedded_source))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		if (!material_cooker::cook_from_def(def, header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_texture_sampler(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::texture_sampler);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		sampler_desc_t desc = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<sampler_desc_t>::value, &desc, asset.embedded_source))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		if (!texture_sampler_cooker::cook_from_desc(desc, header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_physical_material(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::physical_material);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		physical_material_def_t def = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<physical_material_def_t>::value, &def, asset.embedded_source))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		if (!physical_material_cooker::cook_from_def(def, header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_animation_state_machine(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::animation_state_machine);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::none);
		SFG_ASSERT(false);
		return false;
	}

	bool editor_asset_cooker_t::cook_texture(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::texture);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		texture_cook_config_t config = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<texture_cook_config_t>::value, &config, asset.cook_options))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;

		const string_t source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);

		if (!texture_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_skeleton(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::skeleton);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::embedded);

		skeleton_def_t def = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<skeleton_def_t>::value, &def, asset.embedded_source))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		if (!skeleton_cooker::cook_from_def(def, header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_mesh(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::mesh);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file_blob);

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!mesh_cooker::cook_from_file(source_full_path.c_str(), header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}

	bool editor_asset_cooker_t::cook_hdr_skybox(const editor_asset_t& asset)
	{
		SFG_ASSERT(asset.asset_type == editor_asset_type_e::hdr_skybox);
		SFG_ASSERT(asset.source_type == editor_asset_source_type_e::file);

		skybox_hdr_cook_config_t config = {};
		if (!reflection_registry_t::get().deserialize_from_json(type_id_t<skybox_hdr_cook_config_t>::value, &config, asset.cook_options))
			return false;

		resource_header_t header = {};
		ostream_t		  stream;
		const string_t	  source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		if (!skybox_hdr_cooker::cook_from_file(config, source_full_path.c_str(), header, stream))
			return false;
		return save_cooked_asset(asset, header, stream);
	}
}
