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

#include "assets/editor_asset_manager.hpp"

#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_creator.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_modal_controller.hpp"

#include <sfg/data/frame_string.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/math/color.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/serialization/serialization.hpp>
#include <algorithm>
#include <iterator>
#include <utility>

#include <sfg/platform/time.hpp>

namespace sfg
{
#define EDITOR_ASSET_COLOR(R, G, B) color_t::from255(R, G, B, 255.0f).srgb_to_linear().to_vector()

	namespace
	{
		editor_asset_manager_t* s_instance = nullptr;

		struct default_asset_desc_t
		{
			const char*	  asset_name;
			const char*	  source_base_name;
			sid_t		  guid;
			shader_type_e shader_type;
		};

		struct default_texture_asset_desc_t
		{
			const char* asset_name;
			sid_t		guid;
			u8			color[4];
			bool		is_linear;
		};

		struct default_create_asset_desc_t
		{
			const char*			asset_name;
			sid_t				guid;
			editor_asset_type_e asset_type;
			u8					sub_type;
		};

		span_t<u8> make_default_texture_data(const u8 color[4])
		{
			u8* data = static_cast<u8*>(SFG_MALLOC(4));
			SFG_MEMCPY(data, color, 4);
			return {.data = data, .size = 4};
		}
	}

	bool editor_asset_manager_t::init()
	{
		SFG_ASSERT(s_instance == nullptr);
		s_instance = this;
		_asset_descriptors.clear();
		_asset_descriptors.reserve(static_cast<size_t>(editor_asset_type_e::count) - 1);
		register_descriptor({.extensions = {"mp3"}, .display_name = "Audio", .color = EDITOR_ASSET_COLOR(64.0f, 177.0f, 255.0f), .asset_type = editor_asset_type_e::audio});
		register_descriptor({.extensions = {"ttf"}, .display_name = "Font", .color = EDITOR_ASSET_COLOR(245.0f, 194.0f, 82.0f), .asset_type = editor_asset_type_e::font});
		register_descriptor({.extensions = {"glb"}, .display_name = "Mesh", .color = EDITOR_ASSET_COLOR(158.0f, 120.0f, 255.0f), .asset_type = editor_asset_type_e::mesh});
		register_descriptor({.display_name = "Skeleton", .color = EDITOR_ASSET_COLOR(184.0f, 155.0f, 255.0f), .asset_type = editor_asset_type_e::skeleton});
		register_descriptor({.display_name = "Animation", .color = EDITOR_ASSET_COLOR(255.0f, 129.0f, 80.0f), .asset_type = editor_asset_type_e::animation});
		register_descriptor({.display_name = "Material", .color = EDITOR_ASSET_COLOR(255.0f, 102.0f, 0.0f), .asset_type = editor_asset_type_e::material});
		register_descriptor({.extensions = {"hlsl"}, .display_name = "Shader", .color = EDITOR_ASSET_COLOR(90.0f, 190.0f, 255.0f), .asset_type = editor_asset_type_e::shader});
		register_descriptor({.extensions = {"png", "jpg", "jpeg"}, .display_name = "Texture", .color = EDITOR_ASSET_COLOR(151.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture});
		register_descriptor({.display_name = "Texture Sampler", .color = EDITOR_ASSET_COLOR(180.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture_sampler});
		register_descriptor({.display_name = "Physical Material", .color = EDITOR_ASSET_COLOR(214.0f, 65.0f, 57.0f), .asset_type = editor_asset_type_e::physical_material});
		register_descriptor({.display_name = "Prefab", .color = EDITOR_ASSET_COLOR(107.0f, 210.0f, 132.0f), .asset_type = editor_asset_type_e::prefab});
		register_descriptor({.display_name = "State Machine", .color = EDITOR_ASSET_COLOR(245.0f, 118.0f, 182.0f), .asset_type = editor_asset_type_e::animation_state_machine});
		clear();
		return true;
	}

	void editor_asset_manager_t::uninit()
	{
		SFG_ASSERT(s_instance == this);
		if (_import_in_progress)
			job_system_t::get().wait_for_all();
		clear();
		_import_paths.clear();
		_import_options.clear();
		import_status_texts.clear();
		_asset_descriptors.clear();
		_imported_count.store(0, std::memory_order_relaxed);
		_import_finished.store(false, std::memory_order_relaxed);
		_total_import_count		  = 0;
		_last_import_status_index = 0;
		_import_in_progress		  = false;
		s_instance				  = nullptr;
	}

	void editor_asset_manager_t::tick()
	{
		if (!_import_in_progress)
			return;

		const u32				   cooked	= _imported_count.load(std::memory_order_relaxed);
		const f32				   progress = _total_import_count != 0 ? static_cast<f32>(cooked) / static_cast<f32>(_total_import_count) : 1.0f;
		editor_modal_controller_t& modal	= *editor_app_t::get().get_main_surface().modal_controller;
		_cook_progress_modal.set_progress(progress);
		const u32 status_index = cooked < _total_import_count ? cooked : _total_import_count - 1;
		if (status_index != _last_import_status_index)
		{
			_last_import_status_index = status_index;
			modal.set_body_text(import_status_texts[status_index].c_str());
		}

		if (cooked != _total_import_count || !_import_finished.load(std::memory_order_acquire))
			return;

		modal.close_modal();
		rescan(editor_project_t::get()._runtime.assets_path);
		_import_paths.resize(0);
		_import_options.resize(0);
		import_status_texts.resize(0);
		_import_directory_node	  = {};
		_total_import_count		  = 0;
		_last_import_status_index = 0;
		_import_in_progress		  = false;
	}

	void editor_asset_manager_t::clear()
	{
		_asset_tree.clear();
		_assets.clear();
		_root_node = {};
		_generation++;
	}

	void editor_asset_manager_t::rescan(const string_t& assets_dir)
	{
		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(assets_dir.c_str(), entries);

		_asset_tree.resize_zero();
		_asset_tree.reserve(static_cast<u32>(entries.size() + 1));
		hash_map_t<u64, editor_asset_t> found_assets;
		found_assets.reserve(entries.size());

		const string_t root_name = file_system_t::get_last_folder_from_path(assets_dir.c_str());
		const string_t root_path = assets_dir;
		_root_node				 = _asset_tree.emplace(editor_asset_node_t{.name = root_name, .full_path = root_path, .type = editor_asset_node_type_e::folder});

		vector_t<string_t> parts;
		for (const file_system_entry_t& entry : entries)
		{
			const string_t relative = file_system_t::get_relative(assets_dir.c_str(), entry.path.c_str());
			parts.resize(0);
			string_util::split(parts, relative, "/");

			editor_asset_node_handle_t parent			 = _root_node;
			const size_t			   folder_part_count = entry.type == file_system_entry_type_e::directory ? parts.size() : parts.size() - 1;
			for (size_t i = 0; i < folder_part_count; ++i)
				parent = get_or_create_child_folder(parent, parts[i]);

			if (entry.type == file_system_entry_type_e::directory)
				continue;

			editor_asset_t asset = {};
			if (file_system_t::get_file_extension(entry.path) == "sfg_asset")
			{
				if (!editor_asset_util_t::read_asset(entry.path.c_str(), asset))
					continue;

				const u64 asset_id = asset.guid;
				if (asset_id == NULL_SID)
				{
					SFG_ERR("asset {0} has an invalid guid", entry.path.c_str());
					continue;
				}
				if (found_assets.find(asset_id) != found_assets.end())
				{
					SFG_ERR("asset {0} has a duplicate guid", entry.path.c_str());
					continue;
				}

				found_assets.emplace(asset_id, std::move(asset));

				const string_t					 name		= file_system_t::remove_extensions_from_path(parts.back());
				const editor_asset_node_handle_t asset_node = _asset_tree.emplace(editor_asset_node_t{.asset_id = asset_id, .name = name, .full_path = entry.path, .type = editor_asset_node_type_e::asset});
				_asset_tree.attach(parent, asset_node);
			}
			else
			{
				const editor_asset_node_handle_t file_node = _asset_tree.emplace(editor_asset_node_t{.name = parts.back(), .full_path = entry.path, .type = editor_asset_node_type_e::file});
				_asset_tree.attach(parent, file_node);
			}
		}

		for (auto it = _assets.begin(); it != _assets.end();)
		{
			const u64 asset_id = it->first;
			++it;
			if (found_assets.find(asset_id) == found_assets.end())
				_assets.erase(asset_id);
		}
		for (auto& found : found_assets)
			_assets[found.first] = std::move(found.second);

		ensure_integrity();
		_generation++;
	}

	void editor_asset_manager_t::register_descriptor(const editor_asset_descriptor_t& desc)
	{
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::count);
		_asset_descriptors[desc.asset_type] = desc;
	}

	const editor_asset_descriptor_t* editor_asset_manager_t::find_asset_descriptor(const string_t& extension) const
	{
		const auto descriptor_it = std::find_if(_asset_descriptors.begin(), _asset_descriptors.end(), [&](const auto& asset_descriptor) {
			const editor_asset_descriptor_t& descriptor	  = asset_descriptor.second;
			const auto						 extension_it = std::find_if(descriptor.extensions.begin(), descriptor.extensions.end(), [&](const string_t& descriptor_extension) { return descriptor_extension == extension; });
			return extension_it != descriptor.extensions.end();
		});
		return descriptor_it != _asset_descriptors.end() ? &descriptor_it->second : nullptr;
	}

	void editor_asset_manager_t::ensure_default_assets(const char* default_assets_dir)
	{
		SFG_ASSERT(default_assets_dir != nullptr);
		SFG_ASSERT(default_assets_dir[0] != '\0');

		frame_string_t<char> default_assets_path = default_assets_dir;

		if (!file_system_t::exists(default_assets_path.c_str()))
			file_system_t::create_directory(default_assets_path.c_str());

		const string_t& assets_path = editor_project_t::get()._runtime.assets_path;
		rescan(assets_path);
		const string_t					 default_folder_name = file_system_t::get_last_folder_from_path(default_assets_path.c_str());
		const editor_asset_node_handle_t default_assets_node = find_child_folder(_root_node, default_folder_name);
		SFG_ASSERT(!default_assets_node.is_null());

		const default_asset_desc_t default_shader_assets[] = {
			{.asset_name = "shader_gbuffer", .source_base_name = "gbuffer", .guid = DEFAULT_GBUFFER_SHADER_ASSET_GUID, .shader_type = shader_type_e::opaque_shader},
			{.asset_name = "shader_forward", .source_base_name = "forward", .guid = DEFAULT_FORWARD_SHADER_ASSET_GUID, .shader_type = shader_type_e::transparent_shader},
		};
		const default_texture_asset_desc_t default_texture_assets[] = {
			{.asset_name = "texture_albedo", .guid = DEFAULT_ALBEDO_TEXTURE_ASSET_GUID, .color = {255, 255, 255, 255}, .is_linear = false},
			{.asset_name = "texture_orm", .guid = DEFAULT_ORM_TEXTURE_ASSET_GUID, .color = {255, 255, 0, 255}, .is_linear = true},
			{.asset_name = "texture_normal", .guid = DEFAULT_NORMAL_TEXTURE_ASSET_GUID, .color = {128, 128, 128, 255}, .is_linear = true},
			{.asset_name = "texture_emissive", .guid = DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID, .color = {0, 0, 0, 255}, .is_linear = false},
		};
		const default_create_asset_desc_t default_created_assets[] = {
			{.asset_name = "material_gbuffer", .guid = DEFAULT_GBUFFER_MATERIAL_ASSET_GUID, .asset_type = editor_asset_type_e::material, .sub_type = static_cast<u8>(editor_material_type_e::gbuffer)},
			{.asset_name = "material_forward", .guid = DEFAULT_FORWARD_MATERIAL_ASSET_GUID, .asset_type = editor_asset_type_e::material, .sub_type = static_cast<u8>(editor_material_type_e::forward)},
			{.asset_name = "physical_material", .guid = DEFAULT_PHYSICAL_MATERIAL_ASSET_GUID, .asset_type = editor_asset_type_e::physical_material},
			{.asset_name = "sampler_linear", .guid = DEFAULT_LINEAR_SAMPLER_ASSET_GUID, .asset_type = editor_asset_type_e::texture_sampler, .sub_type = static_cast<u8>(editor_texture_sampler_type_e::linear)},
			{.asset_name = "sampler_nearest", .guid = DEFAULT_NEAREST_SAMPLER_ASSET_GUID, .asset_type = editor_asset_type_e::texture_sampler, .sub_type = static_cast<u8>(editor_texture_sampler_type_e::nearest)},
			{.asset_name = "sampler_anisotropic", .guid = DEFAULT_ANISOTROPIC_SAMPLER_ASSET_GUID, .asset_type = editor_asset_type_e::texture_sampler, .sub_type = static_cast<u8>(editor_texture_sampler_type_e::anisotropic)},
		};

		for (const default_asset_desc_t& desc : default_shader_assets)
		{
			editor_asset_creator_t::create_asset(
				{.parent_node = default_assets_node, .name = desc.asset_name, .source_name = desc.source_base_name, .guid = desc.guid, .asset_type = editor_asset_type_e::shader, .sub_type = static_cast<u8>(desc.shader_type), .allow_overwrite = true});
		}

		for (const default_texture_asset_desc_t& desc : default_texture_assets)
		{
			texture_cook_config_t texture_config = {
				.size			  = {.x = 1, .y = 1},
				.payload_type	  = texture_payload_type_e::ktx2_uastc,
				.generate_mipmaps = false,
				.is_linear		  = desc.is_linear,
			};
			editor_asset_t asset = {};
			asset.version		 = editor_asset_t::VERSION;
			asset.guid			 = desc.guid;
			asset.asset_type	 = editor_asset_type_e::texture;
			asset.source_type	 = editor_asset_source_type_e::data;

			nlohmann::json json_data = nlohmann::json::object();
			SFG_ASSERT(reflection_registry_t::get().serialize_to_json(type_id_t<texture_cook_config_t>::value, &texture_config, json_data));
			asset.cook_options = json_data;

			frame_string_t<char> asset_path;
			asset_path.assign(default_assets_path.c_str(), default_assets_path.size());
			asset_path += desc.asset_name;
			asset_path += ".sfg_asset";
			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				SFG_ERR("failed to write default asset {0}", asset_path.c_str());
		}

		for (const default_create_asset_desc_t& desc : default_created_assets)
		{
			editor_asset_creator_t::create_asset({.parent_node = default_assets_node, .name = desc.asset_name, .guid = desc.guid, .asset_type = desc.asset_type, .sub_type = desc.sub_type, .allow_overwrite = true});
		}

		rescan(assets_path);

		for (const default_texture_asset_desc_t& desc : default_texture_assets)
		{
			auto asset_it = _assets.find(desc.guid);
			SFG_ASSERT(asset_it != _assets.end());
			if (file_system_t::exists(editor_asset_util_t::get_cache_path_for_asset(asset_it->second).c_str()))
				continue;

			asset_it->second._transient_data = make_default_texture_data(desc.color);
		}
	}

	void editor_asset_manager_t::ensure_integrity()
	{
		const string_t&								assets_path = editor_project_t::get()._runtime.assets_path;
		hash_map_t<u64, const editor_asset_node_t*> asset_nodes;
		asset_nodes.reserve(_assets.size());
		for (auto it = _asset_tree.begin_handle(); it != _asset_tree.end_handle(); ++it)
		{
			const editor_asset_node_t& node = _asset_tree.value(*it);
			if (node.type == editor_asset_node_type_e::asset)
				asset_nodes[node.asset_id] = &node;
		}

		for (auto& asset_pair : _assets)
		{
			editor_asset_t& asset	 = asset_pair.second;
			asset.status			 = editor_asset_status_e::ok;
			const auto asset_node_it = asset_nodes.find(asset.guid);
			SFG_ASSERT(asset_node_it != asset_nodes.end());
			const editor_asset_node_t* asset_node	  = asset_node_it->second;
			const auto				   descriptor_it  = _asset_descriptors.find(asset.asset_type);
			const char*				   asset_type_str = descriptor_it != _asset_descriptors.end() && !descriptor_it->second.display_name.empty() ? descriptor_it->second.display_name.c_str() : "Unknown";

			if (asset.source_type == editor_asset_source_type_e::embedded && asset.embedded_source.is_null())
			{
				asset.status = editor_asset_status_e::missing_embedded_data;
				SFG_ERR("asset {0}, {1}, {2} has missing embedded data", asset_node->full_path.c_str(), asset.guid, asset_type_str);
			}
			else if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
			{
				string_t source_path = file_system_t::get_absolute_path(assets_path.c_str());
				source_path += asset.source_relative;
				if (asset.source_relative.empty() || !file_system_t::exists(source_path.c_str()))
				{
					asset.status = editor_asset_status_e::missing_file_source;
					SFG_ERR("asset {0}, {1}, {2} has missing file source {3}", asset_node->full_path.c_str(), asset.guid, asset_type_str, asset.source_relative.c_str());
				}
			}

			frame_vector_t<sid_t> dependencies;
			editor_asset_util_t::fetch_dependencies(asset, dependencies);
			for (const sid_t dependency : dependencies)
			{
				if (_assets.find(dependency) != _assets.end())
					continue;

				if (asset.status == editor_asset_status_e::ok)
					asset.status = editor_asset_status_e::missing_dependency;
				SFG_ERR("asset {0}, {1}, {2} has missing dependency {3}", asset_node->full_path.c_str(), asset.guid, asset_type_str, dependency);
			}
		}
	}

	void editor_asset_manager_t::ensure_cook()
	{
		SFG_ASSERT(!_import_in_progress);

		for (auto& asset_pair : _assets)
		{
			editor_asset_t& asset = asset_pair.second;
			SFG_ASSERT(_asset_descriptors.find(asset.asset_type) != _asset_descriptors.end());
			if (asset.status != editor_asset_status_e::ok)
				continue;
			if (!editor_asset_cooker_t::is_cookable(asset.asset_type))
				continue;

			bool		   requires_cook = false;
			const string_t cache_path	 = editor_asset_util_t::get_cache_path_for_asset(asset);
			if (!file_system_t::exists(cache_path.c_str()))
			{
				requires_cook = true;
			}
			else
			{
				istream_t					stream		  = serializer_t::load_from_file(cache_path.c_str());
				const resource_type_desc_t* resource_desc = find_resource_type_desc(static_cast<resource_type_e>(asset.asset_type));
				SFG_ASSERT(resource_desc != nullptr);
				if (stream.empty())
				{
					requires_cook = true;
				}
				else
				{
					resource_header_t header = {};
					header.deserialize(stream);
					requires_cook = header.magic != resource_desc->wire_magic || header.version != resource_desc->wire_version;
				}
			}

			if (!requires_cook)
				continue;

			const editor_asset_t asset_to_cook = asset;
			asset._transient_data			   = {};
			if (!editor_asset_cooker_t::cook_asset(asset_to_cook))
				SFG_ERR("failed cooking asset {0}", asset.guid);
		}
	}

	void editor_asset_manager_t::import_assets(editor_asset_node_handle_t directory_node, const frame_vector_t<string_t>& paths, const frame_vector_t<editor_asset_import_options_t>& import_options)
	{
		SFG_ASSERT(!_import_in_progress);
		SFG_ASSERT(!directory_node.is_null());
		SFG_ASSERT(_asset_tree.is_valid(directory_node));
		SFG_ASSERT(!paths.empty());
		SFG_ASSERT(!import_options.empty());

		_import_directory_node = directory_node;
		_import_paths.resize(0);
		_import_paths.reserve(paths.size());
		_import_options.resize(0);
		_import_options.reserve(import_options.size());
		import_status_texts.resize(0);
		import_status_texts.reserve(paths.size());
		for (const string_t& path : paths)
		{
			_import_paths.push_back(path);
			import_status_texts.push_back(path);
		}
		for (const editor_asset_import_options_t& option : import_options)
			_import_options.push_back(option);

		_imported_count.store(0, std::memory_order_relaxed);
		_import_finished.store(false, std::memory_order_relaxed);
		_total_import_count		  = static_cast<u32>(_import_paths.size());
		_last_import_status_index = 0;
		_import_in_progress		  = true;

		editor_modal_controller_t& modal = *editor_app_t::get().get_main_surface().modal_controller;
		_cook_progress_modal.set_progress(0.0f);
		editor_modal_content_desc_t progress_content = _cook_progress_modal.get_content_desc();
		modal.request_modal("Cooking Assets", import_status_texts[0].c_str(), false, nullptr, 0, &progress_content);

		job_system_t::get().silent_async([this]() {
			vector_t<editor_asset_t>						  imported_assets;
			const span_t<const editor_asset_import_options_t> options = {
				.data = _import_options.data(),
				.size = _import_options.size(),
			};
			for (const string_t& path : _import_paths)
			{
				if (!editor_asset_importer_t::import_asset(_import_directory_node, path.c_str(), options, imported_assets))
					SFG_ERR("failed importing asset {0}", path.c_str());
				_imported_count.fetch_add(1, std::memory_order_relaxed);
			}
			_import_finished.store(true, std::memory_order_release);
		});
	}

	editor_asset_manager_t& editor_asset_manager_t::get()
	{
		SFG_ASSERT(s_instance != nullptr);
		return *s_instance;
	}

	editor_asset_node_handle_t editor_asset_manager_t::find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const
	{
		editor_asset_node_handle_t child = _asset_tree.first_child(parent);
		while (!child.is_null())
		{
			const editor_asset_node_t& node = _asset_tree.value(child);
			if (node.type == editor_asset_node_type_e::folder && node.name == name)
				return child;

			child = _asset_tree.next_sibling(child);
		}
		return {};
	}

	editor_asset_node_handle_t editor_asset_manager_t::get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name)
	{
		const editor_asset_node_handle_t existing = find_child_folder(parent, name);
		if (!existing.is_null())
			return existing;

		const u8						 flags	   = !name.empty() && name[0] == '_' ? editor_asset_node_flag_hidden : 0;
		const string_t					 full_path = _asset_tree.value(parent).full_path + name;
		const editor_asset_node_handle_t folder	   = _asset_tree.emplace(editor_asset_node_t{.name = name, .full_path = full_path, .type = editor_asset_node_type_e::folder, .flags = flags});
		_asset_tree.attach(parent, folder);
		return folder;
	}
}
