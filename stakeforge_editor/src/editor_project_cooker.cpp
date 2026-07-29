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

#include "editor_project_cooker.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_dependencies.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_mesh_generator.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "editor_project_cook_options.hpp"
#include "editor_settings.hpp"
#include "editor_surface_controller.hpp"
#include "scripting/editor_script_manager.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/editor_modal_project_cooker.hpp"

#include <sfg/data/hash_map.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/project/project_package_meta.hpp>
#include <sfg/runtime/resources/resource_manifest.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/taskflow/taskflow.hpp>

namespace sfg
{
#define PROJECT_COOK_PRIMITIVE_COUNT 5

	editor_project_cooker_t::editor_project_cooker_t()	= default;
	editor_project_cooker_t::~editor_project_cooker_t() = default;

	void editor_project_cooker_t::init()
	{
		_cook_options	= make_unique<editor_project_cook_options_t>();
		_package_meta	= make_unique<project_package_meta_t>();
		_options_modal	= make_unique<editor_modal_project_cooker_t>();
		_progress_modal = make_unique<editor_modal_progress_bar_t>();
	}

	void editor_project_cooker_t::uninit()
	{
		if (_cook_state.load(std::memory_order_acquire) == cook_state_e::cooking)
			editor_app_t::get().get_editor_work_executor().wait_for_all();

		_progress_modal.reset();
		_options_modal.reset();
		_package_meta.reset();
		_cook_options.reset();

		_cook_failure_reason.resize(0);
		_cook_state.store(cook_state_e::idle, std::memory_order_relaxed);
	}

	void editor_project_cooker_t::tick()
	{
		const cook_state_e cook_state = _cook_state.load(std::memory_order_acquire);

		if (cook_state == cook_state_e::idle || cook_state == cook_state_e::cooking)
			return;

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		modal.close_modal();
		_cook_state.store(cook_state_e::idle, std::memory_order_relaxed);

		if (cook_state == cook_state_e::failed)
		{
			const editor_modal_button_desc_t buttons[] = {
				{.text = "Close"},
			};

			modal.request_modal("Cook Project Failed", _cook_failure_reason.c_str(), buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
		}
	}

	void editor_project_cooker_t::request_cook()
	{
		SFG_ASSERT(_cook_state.load(std::memory_order_relaxed) == cook_state_e::idle);

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		_options_modal->request(modal, editor_settings_t::get().project_cook);
	}

	void editor_project_cooker_t::cook_project(const editor_project_cook_options_t& options)
	{
		SFG_ASSERT(_cook_state.load(std::memory_order_relaxed) == cook_state_e::idle);

		editor_modal_controller_t&		 modal	   = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		const editor_modal_button_desc_t buttons[] = {
			{.text = "Close"},
		};

		if (options.main_world == NULL_SID || options.main_world == 0)
		{
			modal.request_modal("Cook Project", "Select a main world.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		const editor_script_manager_t& script_manager = editor_script_manager_t::get();

		if (!script_manager.is_initial_activation_completed())
		{
			modal.request_modal("Cook Project", "C# scripts have not compiled and activated successfully.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		if (!script_manager.is_compile_idle())
		{
			modal.request_modal("Cook Project", "Wait for C# script compilation to finish.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		if (!script_manager.is_active_assembly_current())
		{
			modal.request_modal("Cook Project", "The active C# assembly does not match the published script files. Compile the scripts successfully before cooking.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		editor_app_t::get().get_editor_work_executor().wait_for_all();

		*_cook_options = options;
		_cook_failure_reason.resize(0);

		_cook_state.store(cook_state_e::cooking, std::memory_order_relaxed);

		_progress_modal->set_progress(0.0f);
		const editor_modal_content_desc_t content = _progress_modal->get_content_desc();
		modal.request_modal("Cooking Project", "Preparing the project package.", false, nullptr, 0, &content);

		const string_t target_path = editor_project_t::get()._runtime.cook_path + project_package_meta_t::FILE_NAME;

		editor_app_t::get().get_editor_work_executor().silent_async([this, target_path]() {
			const cook_state_e result = cook_project_worker(target_path.c_str()) ? cook_state_e::succeeded : cook_state_e::failed;
			_cook_state.store(result, std::memory_order_release);
		});
	}

	bool editor_project_cooker_t::cook_project_worker(const char* target_path)
	{
		// load the engine resources that always ship
		resource_manifest_t engine_manifest = {};

		if (!engine_manifest.load_from_file(editor_directories_t::get_engine_manifest().c_str()))
		{
			_cook_failure_reason = "Failed to load the engine resource manifest.";
			return false;
		}

		// build package metadata from the cook options
		*_package_meta						= {};
		_package_meta->project_settings		= editor_project_t::get().settings.project_settings;
		_package_meta->window_resolution	= _cook_options->resolution;
		_package_meta->window_style			= _cook_options->is_borderless ? window_style_e::borderless : window_style_e::app_window;
		_package_meta->is_fullscreen		= _cook_options->is_fullscreen;
		_package_meta->script_assembly_name = editor_project_t::get()._runtime.name + ".dll";
		_package_meta->worlds.reserve(_cook_options->worlds.size());

		// make sure every selected world exists and record its name hash
		for (sid_t world : _cook_options->worlds)
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(world);

			if (asset == nullptr)
			{
				_cook_failure_reason = string_t("World asset does not exist: ") + std::to_string(world);
				return false;
			}

			if (asset->asset_type != editor_asset_type_e::world)
			{
				_cook_failure_reason = string_t("Selected project world is not a world asset: ") + std::to_string(world);
				return false;
			}

			const editor_asset_node_t* node = editor_asset_manager_t::get().find_asset_node(world);
			SFG_ASSERT(node != nullptr);

			const world_meta_t world_meta{
				.sid	   = world,
				.name_hash = TO_SID(node->name),
			};

			for (const world_meta_t& existing_world : _package_meta->worlds)
			{
				if (existing_world.name_hash != world_meta.name_hash)
					continue;

				_cook_failure_reason = string_t("World asset names must be unique: ") + node->name;
				return false;
			}

			_package_meta->worlds.push_back(world_meta);

			if (world == _cook_options->main_world)
				_package_meta->main_world = world_meta;
		}

		if (_package_meta->main_world.sid == NULL_SID)
		{
			_cook_failure_reason = "Main world is not included in the project worlds.";
			return false;
		}

		// start the resource stream
		ostream_t resource_stream = {};
		resource_stream << project_package_meta_t::RESOURCE_STREAM_WIRE_MAGIC;
		resource_stream << project_package_meta_t::RESOURCE_STREAM_WIRE_VERSION;
		resource_stream << static_cast<u32>(PROJECT_COOK_PRIMITIVE_COUNT);

		// generate the built-in primitives
		ostream_t primitive_stream = {};

		if (!editor_mesh_generator_t::generate_cube({.size = vec3f_t::one}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the cube primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_CUBE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_sphere({}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the sphere primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_SPHERE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_cylinder({}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the cylinder primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_CYLINDER_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_capsule({}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the capsule primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_CAPSULE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_plane({.size = vec2f_t::one}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the plane primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_PLANE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());

		// track unique resources and their expected types
		vector_t<editor_asset_dependency_t> project_resources = {};
		hash_map_t<sid_t, resource_type_e>	resource_types	  = {};
		resource_types.reserve(editor_asset_manager_t::get().get_assets().size() + engine_manifest.resources.size() + _package_meta->worlds.size() + _cook_options->extra_resources.size() + PROJECT_COOK_PRIMITIVE_COUNT);
		resource_types.emplace(DEFAULT_MESH_CUBE_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_SPHERE_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_CYLINDER_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_CAPSULE_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_PLANE_GUID, resource_type_e::mesh);

		// write every engine manifest resource
		resource_stream << static_cast<u32>(engine_manifest.resources.size());

		for (const resource_manifest_entry_t& entry : engine_manifest.resources)
		{
			if (entry.type == resource_type_e::invalid)
			{
				_cook_failure_reason = string_t("Engine resource has an invalid type: ") + entry.path;
				return false;
			}

			const sid_t	   sid		   = TO_SID(entry.path);
			const string_t cache_path  = editor_directories_t::get_editor_resource_cache() + std::to_string(sid) + ".sfg_bin";
			istream_t	   cached_file = serializer_t::load_from_file(cache_path.c_str());

			if (cached_file.empty())
			{
				_cook_failure_reason = string_t("Failed to read cooked engine resource: ") + entry.path;
				return false;
			}

			resource_stream << sid;
			resource_stream << entry.type;
			resource_stream << static_cast<u64>(cached_file.get_size());
			resource_stream.write_raw(cached_file.get_raw(), cached_file.get_size());

			resource_types.emplace(sid, entry.type);
		}

		// add project resources without duplicates
		const auto add_project_resource = [&](const editor_asset_dependency_t& dependency) -> bool {
			if (dependency.sid == NULL_SID)
				return true;

			if (dependency.type == resource_type_e::invalid || static_cast<u8>(dependency.type) >= RESOURCE_TYPE_MAX)
			{
				_cook_failure_reason = string_t("Resource has an invalid type: ") + std::to_string(dependency.sid);
				return false;
			}

			const auto [it, inserted] = resource_types.emplace(dependency.sid, dependency.type);

			if (!inserted)
			{
				if (it->second == dependency.type)
					return true;

				_cook_failure_reason = string_t("Resource is referenced with conflicting types: ") + std::to_string(dependency.sid);
				return false;
			}

			project_resources.push_back(dependency);
			return true;
		};

		// write worlds and collect their direct resources
		resource_stream << static_cast<u32>(_package_meta->worlds.size());

		vector_t<editor_asset_dependency_t> dependencies = {};

		for (const world_meta_t& world : _package_meta->worlds)
		{
			const editor_asset_t&	  asset		 = *editor_asset_manager_t::get().find_asset(world.sid);
			const string_t&			  world_json = asset.embedded_source;
			const resource_map_info_t resource_info{
				.offset = resource_stream.get_size(),
				.size	= world_json.size(),
			};
			const bool inserted = _package_meta->resource_map.emplace(world.sid, resource_info).second;

			if (!inserted)
			{
				_cook_failure_reason = string_t("World is included more than once: ") + std::to_string(world.sid);
				return false;
			}

			resource_stream.write_raw(reinterpret_cast<const u8*>(world_json.data()), world_json.size());

			dependencies.resize(0);

			if (!editor_asset_dependencies_t::fetch_dependencies(asset, dependencies))
			{
				_cook_failure_reason = string_t("Failed to resolve world resources: ") + std::to_string(world.sid);
				return false;
			}

			for (const editor_asset_dependency_t& dependency : dependencies)
			{
				if (!add_project_resource(dependency))
					return false;
			}
		}

		// add resources selected explicitly by the user
		for (resource_handle_t extra_resource : _cook_options->extra_resources)
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(extra_resource);

			if (asset == nullptr)
			{
				_cook_failure_reason = string_t("Extra resource asset does not exist: ") + std::to_string(extra_resource);
				return false;
			}

			const editor_asset_dependency_t dependency{
				.sid  = extra_resource,
				.type = static_cast<resource_type_e>(asset->asset_type),
			};

			if (!add_project_resource(dependency))
				return false;
		}

		// expand the full transitive dependency set
		for (size_t resource_index = 0; resource_index < project_resources.size(); ++resource_index)
		{
			const editor_asset_dependency_t resource = project_resources[resource_index];
			const editor_asset_t*			asset	 = editor_asset_manager_t::get().find_asset(resource.sid);

			if (asset == nullptr)
			{
				_cook_failure_reason = string_t("Referenced resource asset does not exist: ") + std::to_string(resource.sid);
				return false;
			}

			if (static_cast<resource_type_e>(asset->asset_type) != resource.type)
			{
				_cook_failure_reason = string_t("Referenced resource asset has the wrong type: ") + std::to_string(resource.sid);
				return false;
			}

			dependencies.resize(0);

			if (!editor_asset_dependencies_t::fetch_dependencies(*asset, dependencies))
			{
				_cook_failure_reason = string_t("Failed to resolve resource dependencies: ") + std::to_string(resource.sid);
				return false;
			}

			for (const editor_asset_dependency_t& dependency : dependencies)
			{
				if (!add_project_resource(dependency))
					return false;
			}
		}

		// write project resources and record their locations
		resource_stream << static_cast<u32>(project_resources.size());

		for (const editor_asset_dependency_t& resource : project_resources)
		{
			const string_t cache_path  = editor_asset_path_t::get_cache_path_for_guid(resource.sid);
			istream_t	   cached_file = serializer_t::load_from_file(cache_path.c_str());

			if (cached_file.empty())
			{
				_cook_failure_reason = string_t("Cooked resource does not exist: ") + std::to_string(resource.sid);
				return false;
			}

			const resource_map_info_t resource_info{
				.offset = resource_stream.get_size(),
				.size	= cached_file.get_size(),
			};
			const bool inserted = _package_meta->resource_map.emplace(resource.sid, resource_info).second;

			if (!inserted)
			{
				_cook_failure_reason = string_t("Resource map already contains: ") + std::to_string(resource.sid);
				return false;
			}

			resource_stream.write_raw(cached_file.get_raw(), cached_file.get_size());
		}

		// serialize the finished package metadata
		ostream_t meta_stream = {};

		if (!_package_meta->serialize(meta_stream))
		{
			_cook_failure_reason = "Failed to serialize project package metadata.";
			SFG_ERR("failed to serialize project package metadata");
			return false;
		}

		// save both package files atomically
		const string_t resource_target_path = editor_project_t::get()._runtime.cook_path + project_package_meta_t::RESOURCE_FILE_NAME;

		if (!serializer_t::save_to_file_atomic(resource_target_path.c_str(), resource_stream))
		{
			_cook_failure_reason = "Failed to write cooked resources.";
			return false;
		}

		if (!serializer_t::save_to_file_atomic(target_path, meta_stream))
		{
			_cook_failure_reason = "Failed to write project package metadata.";
			return false;
		}

		if (!publish_game_files())
			return false;

		return true;
	}

	bool editor_project_cooker_t::publish_game_files()
	{
		const editor_project_runtime_t& project_runtime = editor_project_t::get()._runtime;
		const string_t					running_path	= file_system_t::get_running_directory();
		const string_t					managed_path	= project_runtime.cook_path + "managed/";
		const string_t					scripts_path	= project_runtime.cook_path + "scripts/";

		if (!file_system_t::ensure_directory(managed_path.c_str()) || !file_system_t::ensure_directory(scripts_path.c_str()))
		{
			_cook_failure_reason = "Failed to create the packaged managed directories.";
			return false;
		}

		struct file_copy_t
		{
			const char* source_name = nullptr;
			const char* target_name = nullptr;
		};

		const file_copy_t runtime_files[] = {
			{.source_name = "stakeforge_game.exe", .target_name = nullptr},
			{.source_name = "nethost.dll", .target_name = "nethost.dll"},
			{.source_name = "dxcompiler.dll", .target_name = "dxcompiler.dll"},
			{.source_name = "dxil.dll", .target_name = "dxil.dll"},
			{.source_name = "WinPixEventRuntime.dll", .target_name = "WinPixEventRuntime.dll"},
		};

		for (const file_copy_t& file : runtime_files)
		{
			const string_t source_path = running_path + file.source_name;
			const string_t target_name = file.target_name == nullptr ? project_runtime.name + ".exe" : string_t(file.target_name);
			const string_t target_path = project_runtime.cook_path + target_name;

			if (!file_system_t::copy_file(source_path.c_str(), target_path.c_str()))
			{
				_cook_failure_reason = string_t("Failed to publish game runtime file: ") + file.source_name;
				return false;
			}
		}

		const char* managed_files[] = {
			"Stakeforge.ScriptHost.dll",
			"Stakeforge.ScriptHost.deps.json",
			"Stakeforge.ScriptHost.runtimeconfig.json",
			"Stakeforge.Managed.dll",
		};

		for (const char* file : managed_files)
		{
			const string_t source_path = running_path + "managed/" + file;
			const string_t target_path = managed_path + file;

			if (!file_system_t::copy_file(source_path.c_str(), target_path.c_str()))
			{
				_cook_failure_reason = string_t("Failed to publish managed runtime file: ") + file;
				return false;
			}
		}

		const char* script_extensions[] = {
			".dll",
			".deps.json",
			".pdb",
		};

		for (const char* extension : script_extensions)
		{
			const string_t file_name   = project_runtime.name + extension;
			const string_t source_path = project_runtime.script_library_path + file_name;
			const string_t target_path = scripts_path + file_name;

			if (!file_system_t::copy_file(source_path.c_str(), target_path.c_str()))
			{
				_cook_failure_reason = string_t("Failed to publish C# project file: ") + file_name;
				return false;
			}
		}

		return true;
	}
}
