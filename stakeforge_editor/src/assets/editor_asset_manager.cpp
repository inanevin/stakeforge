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

#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_modal_controller.hpp"

#include <sfg/data/frame_string.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/runtime/resources/resource_cache.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <utility>

#include <sfg/platform/time.hpp>
#include <string>

namespace sfg
{
	namespace
	{
		editor_asset_manager_t* s_instance = nullptr;
	}

	bool editor_asset_manager_t::init()
	{
		SFG_ASSERT(s_instance == nullptr);
		s_instance = this;
		_asset_descriptors.clear();
		_asset_descriptors.reserve(static_cast<size_t>(editor_asset_type_e::count) - 1);
		editor_asset_loader_audio_t::register_type();
		editor_asset_loader_font_t::register_type();
		editor_asset_loader_mesh_t::register_type();
		editor_asset_loader_skeleton_t::register_type();
		editor_asset_loader_animation_t::register_type();
		editor_asset_loader_material_t::register_type();
		editor_asset_loader_shader_t::register_type();
		editor_asset_loader_texture_t::register_type();
		editor_asset_loader_texture_sampler_t::register_type();
		editor_asset_loader_physical_material_t::register_type();
		editor_asset_loader_prefab_t::register_type();
		editor_asset_loader_animation_state_machine_t::register_type();
		clear();
		return true;
	}

	void editor_asset_manager_t::uninit()
	{
		SFG_ASSERT(s_instance == this);
		if (_cook_in_progress)
			job_system_t::get().wait_for_all();
		clear();
		_cook_assets.clear();
		_asset_descriptors.clear();
		_cooked_count.store(0, std::memory_order_relaxed);
		_cook_finished.store(false, std::memory_order_relaxed);
		_total_cook_count = 0;
		_cook_in_progress = false;
		s_instance		  = nullptr;
	}

	void editor_asset_manager_t::tick()
	{
		if (!_cook_in_progress)
			return;

		const u32				   cooked	= _cooked_count.load(std::memory_order_relaxed);
		const f32				   progress = _total_cook_count != 0 ? static_cast<f32>(cooked) / static_cast<f32>(_total_cook_count) : 1.0f;
		editor_modal_controller_t& modal	= *editor_app_t::get().get_main_surface().modal_controller;
		_cook_progress_modal.set_progress(progress);

		if (cooked != _total_cook_count || !_cook_finished.load(std::memory_order_acquire))
			return;

		modal.close_modal();
		_cook_assets.resize(0);
		_total_cook_count = 0;
		_cook_in_progress = false;
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
		_root_node				 = _asset_tree.emplace(editor_asset_node_t{.name = root_name, .type = editor_asset_node_type_e::folder, .flags = editor_asset_node_flag_promoted});

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
				if (!read_asset(entry.path.c_str(), asset))
					continue;

				const u64 asset_id = asset.guid;
				if (asset_id == 0)
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
				const editor_asset_node_handle_t asset_node = _asset_tree.emplace(editor_asset_node_t{.asset_id = asset_id, .name = name, .type = editor_asset_node_type_e::asset});
				_asset_tree.attach(parent, asset_node);
			}
			else
			{
				const editor_asset_node_handle_t file_node = _asset_tree.emplace(editor_asset_node_t{.name = parts.back(), .type = editor_asset_node_type_e::file});
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
		if (extension.empty())
			return nullptr;

		string_t target = extension;
		if (!target.empty() && target[0] == '.')
			target.erase(target.begin());
		string_util::to_lower(target);

		for (const auto& asset_descriptor : _asset_descriptors)
		{
			const editor_asset_descriptor_t& descriptor = asset_descriptor.second;
			size_t							 start		= 0;
			while (start < descriptor.extension.size())
			{
				while (start < descriptor.extension.size() && (descriptor.extension[start] == '.' || descriptor.extension[start] == ';' || descriptor.extension[start] == ',' || descriptor.extension[start] == ' '))
					++start;

				size_t end = start;
				while (end < descriptor.extension.size() && descriptor.extension[end] != ';' && descriptor.extension[end] != ',' && descriptor.extension[end] != ' ')
					++end;

				if (end > start)
				{
					string_t current = descriptor.extension.substr(start, end - start);
					string_util::to_lower(current);
					if (current == target)
						return &descriptor;
				}

				start = end;
			}
		}

		return nullptr;
	}

	void editor_asset_manager_t::cook_assets(editor_asset_t* assets, u8 size)
	{
		SFG_ASSERT(!_cook_in_progress);

		if (size == 0)
			return;

		_cook_assets.resize(0);
		_cook_assets.reserve(size);
		for (u8 i = 0; i < size; ++i)
			_cook_assets.push_back(assets[i]);

		_cooked_count.store(0, std::memory_order_relaxed);
		_cook_finished.store(false, std::memory_order_relaxed);
		_total_cook_count = size;
		_cook_in_progress = true;

		editor_modal_controller_t& modal = *editor_app_t::get().get_main_surface().modal_controller;
		_cook_progress_modal.set_progress(0.0f);
		editor_modal_content_desc_t progress_content = _cook_progress_modal.get_content_desc();
		modal.request_modal("Cooking Assets", "Preparing asset cache.", false, nullptr, 0, &progress_content);

		const string_t cache_dir = editor_project_t::get()._runtime.cache_path;
		job_system_t::get().silent_async([this, cache_dir]() {
			for (const editor_asset_t& asset : _cook_assets)
			{
				const auto descriptor_it = _asset_descriptors.find(asset.asset_type);
				SFG_ASSERT(descriptor_it != _asset_descriptors.end());
				if (descriptor_it->second.cook != nullptr)
				{
					ostream_t	   stream;
					const string_t cache_name = std::to_string(asset.guid);
					if (!descriptor_it->second.cook(asset, stream) || !resource_cache_t::save(cache_dir.c_str(), cache_name.c_str(), stream))
						SFG_ERR("failed cooking asset {0}", asset.guid);
				}
				_cooked_count.fetch_add(1, std::memory_order_relaxed);
			}
			_cook_finished.store(true, std::memory_order_release);
		});
	}

	bool editor_asset_manager_t::read_asset(const char* path, editor_asset_t& out_asset) const
	{
		const string_t		 json_text = file_system_t::read_file_as_string(path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);
		if (doc.is_discarded())
		{
			SFG_ERR("failed to parse asset {0}", path);
			return false;
		}

		doc.get_to(out_asset);
		return true;
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

		const u8						 flags	= !name.empty() && name[0] == '_' ? editor_asset_node_flag_hidden : 0;
		const editor_asset_node_handle_t folder = _asset_tree.emplace(editor_asset_node_t{.name = name, .type = editor_asset_node_type_e::folder, .flags = flags});
		_asset_tree.attach(parent, folder);
		return folder;
	}
}
