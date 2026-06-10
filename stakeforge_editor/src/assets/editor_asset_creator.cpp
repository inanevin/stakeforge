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

#include "assets/editor_asset_creator.hpp"

#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/runtime/resources/shader_types.hpp>

namespace sfg
{
	namespace
	{
		using scaffold_embedded_source_fn = bool (*)(u8 sub_type, nlohmann::json& out_embedded_source);

		void scaffold_shader_cook_options(editor_asset_t& asset)
		{
			const shader_type_e shader_type	   = static_cast<shader_type_e>(asset.sub_type);
			asset.cook_options["schema"]	   = "sfg.schema.shader";
			asset.cook_options["include_dirs"] = {"editor_scaffold/shaders", "editor_scaffold/shaders/world"};
			switch (shader_type)
			{
			case shader_type_e::opaque_shader:
				asset.cook_options["type"] = "opaque_shader";
				break;
			case shader_type_e::transparent_shader:
				asset.cook_options["type"] = "transparent_shader";
				break;
			case shader_type_e::post_process_shader:
				asset.cook_options["type"] = "post_process_shader";
				break;
			case shader_type_e::ui_shader:
				asset.cook_options["type"] = "ui_shader";
				break;
			case shader_type_e::ui_text_shader:
				asset.cook_options["type"] = "ui_text_shader";
				break;
			default:
				asset.cook_options["type"] = "opaque_shader";
				break;
			}
		}

		bool scaffold_material_embedded_source_by_sub_type(u8 sub_type, nlohmann::json& out_embedded_source)
		{
			const editor_material_type_e material_type = static_cast<editor_material_type_e>(sub_type);
			return editor_asset_creator_t::scaffold_material_embedded_source(material_type, out_embedded_source);
		}

		bool scaffold_physical_material_embedded_source_by_sub_type(u8, nlohmann::json& out_embedded_source)
		{
			return editor_asset_creator_t::scaffold_physical_material_embedded_source(out_embedded_source);
		}

		bool scaffold_texture_sampler_embedded_source_by_sub_type(u8, nlohmann::json& out_embedded_source)
		{
			return editor_asset_creator_t::scaffold_texture_sampler_embedded_source(out_embedded_source);
		}

		bool create_embedded_asset(const editor_asset_create_desc_t& desc, editor_asset_type_e asset_type, scaffold_embedded_source_fn scaffold_fn, editor_asset_t* out_asset)
		{
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			SFG_ASSERT(!desc.parent_node.is_null());
			SFG_ASSERT(tree.is_valid(desc.parent_node));
			const editor_asset_node_t& parent_node = tree.value(desc.parent_node);
			SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
			SFG_ASSERT(!parent_node.full_path.empty());

			if (!editor_directories_t::is_valid_asset_name(desc.name))
				return false;

			const string_t asset_path = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), desc.name);
			if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
				return false;

			editor_asset_t asset = {};
			asset.version		 = editor_asset_t::VERSION;
			asset.guid			 = editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type	 = asset_type;
			asset.source_type	 = editor_asset_source_type_e::embedded;
			asset.sub_type		 = desc.sub_type;

			if (!scaffold_fn(desc.sub_type, asset.embedded_source))
				return false;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (out_asset != nullptr)
				*out_asset = asset;
			return true;
		}

		bool create_shader_asset(const editor_asset_create_desc_t& desc, editor_asset_t* out_asset)
		{
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			SFG_ASSERT(!desc.parent_node.is_null());
			SFG_ASSERT(tree.is_valid(desc.parent_node));
			const editor_asset_node_t& parent_node = tree.value(desc.parent_node);
			SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
			SFG_ASSERT(!parent_node.full_path.empty());

			if (!editor_directories_t::is_valid_asset_name(desc.name))
				return false;

			const string_t asset_path = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), desc.name);
			if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
				return false;

			editor_asset_t asset = {};
			asset.version		 = editor_asset_t::VERSION;
			asset.guid			 = editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type	 = editor_asset_type_e::shader;
			asset.source_type	 = editor_asset_source_type_e::file;
			asset.sub_type		 = desc.sub_type;

			if (!editor_asset_creator_t::scaffold_shader_source(asset, parent_node.full_path.c_str(), desc.name))
				return false;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (out_asset != nullptr)
				*out_asset = asset;
			return true;
		}

		bool create_none_source_asset(const editor_asset_create_desc_t& desc, editor_asset_type_e asset_type, editor_asset_t* out_asset)
		{
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			SFG_ASSERT(!desc.parent_node.is_null());
			SFG_ASSERT(tree.is_valid(desc.parent_node));
			const editor_asset_node_t& parent_node = tree.value(desc.parent_node);
			SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
			SFG_ASSERT(!parent_node.full_path.empty());

			if (!editor_directories_t::is_valid_asset_name(desc.name))
				return false;

			const string_t asset_path = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), desc.name);
			if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
				return false;

			editor_asset_t asset = {};
			asset.version		 = editor_asset_t::VERSION;
			asset.guid			 = editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type	 = asset_type;
			asset.source_type	 = editor_asset_source_type_e::none;
			asset.sub_type		 = desc.sub_type;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (out_asset != nullptr)
				*out_asset = asset;
			return true;
		}
	}

	const char* editor_asset_creator_t::get_material_scaffold_relative(editor_material_type_e material_type)
	{
		switch (material_type)
		{
		case editor_material_type_e::forward:
			return "editor_scaffold/materials/material_forward.sfg_asset";
		default:
			return "editor_scaffold/materials/material_gbuffer.sfg_asset";
		}
	}

	const char* editor_asset_creator_t::get_physical_material_scaffold_relative()
	{
		return "editor_scaffold/materials/physical_material.sfg_asset";
	}

	const char* editor_asset_creator_t::get_shader_scaffold_relative(shader_type_e shader_type)
	{
		switch (shader_type)
		{
		case shader_type_e::opaque_shader:
			return "editor_scaffold/shaders/world/gbuffer_lit.hlsl";
		case shader_type_e::transparent_shader:
			return "editor_scaffold/shaders/world/forward.hlsl";
		case shader_type_e::post_process_shader:
			return "editor_scaffold/shaders/world/forward.hlsl";
		case shader_type_e::ui_shader:
			return "editor_scaffold/shaders/world/forward.hlsl";
		case shader_type_e::ui_text_shader:
			return "editor_scaffold/shaders/world/forward.hlsl";
		default:
			return "editor_scaffold/shaders/world/gbuffer_lit.hlsl";
		}
	}

	const char* editor_asset_creator_t::get_texture_sampler_scaffold_relative()
	{
		return "editor_scaffold/samplers/sampler_linear.sfg_asset";
	}

	bool editor_asset_creator_t::create_asset(const editor_asset_create_desc_t& desc, editor_asset_t* out_asset)
	{
		editor_asset_t asset  = {};
		bool		   result = false;

		switch (desc.asset_type)
		{
		case editor_asset_type_e::shader:
			result = create_shader_asset(desc, &asset) && editor_asset_cooker_t::cook_shader(asset);
			break;
		case editor_asset_type_e::material:
			result = create_embedded_asset(desc, editor_asset_type_e::material, scaffold_material_embedded_source_by_sub_type, &asset) && editor_asset_cooker_t::cook_material(asset);
			break;
		case editor_asset_type_e::texture_sampler:
			result = create_embedded_asset(desc, editor_asset_type_e::texture_sampler, scaffold_texture_sampler_embedded_source_by_sub_type, &asset) && editor_asset_cooker_t::cook_texture_sampler(asset);
			break;
		case editor_asset_type_e::physical_material:
			result = create_embedded_asset(desc, editor_asset_type_e::physical_material, scaffold_physical_material_embedded_source_by_sub_type, &asset) && editor_asset_cooker_t::cook_physical_material(asset);
			break;
		case editor_asset_type_e::animation_state_machine:
			result = create_none_source_asset(desc, editor_asset_type_e::animation_state_machine, &asset) && editor_asset_cooker_t::cook_animation_state_machine(asset);
			break;
		default:
			SFG_ASSERT(false);
			return false;
		}

		if (result && out_asset != nullptr)
			*out_asset = asset;
		return result;
	}

	bool editor_asset_creator_t::scaffold_material_embedded_source(editor_material_type_e material_type, nlohmann::json& out_embedded_source)
	{
		string_t scaffold_path = file_system_t::get_running_directory();
		scaffold_path += get_material_scaffold_relative(material_type);
		SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

		editor_asset_t scaffold_asset = {};
		const bool	   read_result	  = editor_asset_util_t::read_asset(scaffold_path.c_str(), scaffold_asset);
		SFG_ASSERT(read_result);
		if (!read_result)
			return false;

		out_embedded_source = scaffold_asset.embedded_source;
		return true;
	}

	bool editor_asset_creator_t::scaffold_physical_material_embedded_source(nlohmann::json& out_embedded_source)
	{
		string_t scaffold_path = file_system_t::get_running_directory();
		scaffold_path += get_physical_material_scaffold_relative();
		SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

		editor_asset_t scaffold_asset = {};
		const bool	   read_result	  = editor_asset_util_t::read_asset(scaffold_path.c_str(), scaffold_asset);
		SFG_ASSERT(read_result);
		if (!read_result)
			return false;

		out_embedded_source = scaffold_asset.embedded_source;
		return true;
	}

	bool editor_asset_creator_t::scaffold_texture_sampler_embedded_source(nlohmann::json& out_embedded_source)
	{
		string_t scaffold_path = file_system_t::get_running_directory();
		scaffold_path += get_texture_sampler_scaffold_relative();
		SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

		editor_asset_t scaffold_asset = {};
		const bool	   read_result	  = editor_asset_util_t::read_asset(scaffold_path.c_str(), scaffold_asset);
		SFG_ASSERT(read_result);
		if (!read_result)
			return false;

		out_embedded_source = scaffold_asset.embedded_source;
		return true;
	}

	bool editor_asset_creator_t::scaffold_shader_source(editor_asset_t& asset, const char* directory, const char* file_name)
	{
		const shader_type_e shader_type = static_cast<shader_type_e>(asset.sub_type);
		scaffold_shader_cook_options(asset);

		if (asset.source_relative.empty())
		{
			SFG_ASSERT(directory != nullptr);
			SFG_ASSERT(directory[0] != '\0');
			SFG_ASSERT(file_name != nullptr);
			SFG_ASSERT(file_name[0] != '\0');

			string_t scaffold_path = file_system_t::get_running_directory();
			scaffold_path += get_shader_scaffold_relative(shader_type);
			SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

			const string_t source_path = editor_asset_util_t::make_unique_source_path(directory, file_name, "hlsl");
			if (!file_system_t::copy_file(scaffold_path.c_str(), source_path.c_str()))
				return false;

			SFG_ASSERT(file_system_t::exists(source_path.c_str()));
			asset.source_relative = editor_asset_util_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());
			SFG_ASSERT(!asset.source_relative.empty());
		}
		return true;
	}
}
