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

#include "tools/editor_project_cooker.hpp"
#include "tools/assets/editor_asset.hpp"
#include "tools/assets/editor_asset_dependencies.hpp"
#include "tools/assets/editor_asset_manager.hpp"
#include "tools/assets/editor_asset_path.hpp"
#include "tools/assets/editor_mesh_generator.hpp"
#include "tools/editor_app.hpp"
#include "tools/editor_directories.hpp"
#include "tools/editor_project.hpp"
#include "tools/editor_project_cook_options.hpp"
#include "tools/editor_settings.hpp"
#include "tools/editor_surface_controller.hpp"
#include "tools/scripting/editor_native_manager.hpp"
#include "tools/scripting/native_compiler.hpp"
#include "tools/ui/editor_modal_progress_bar.hpp"
#include "tools/ui/editor_modal_project_cooker.hpp"

#include <sfg/core/data/hash_map.hpp>
#include <sfg/core/data/istream.hpp>
#include <sfg/core/data/ostream.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/runtime/project/project_package_meta.hpp>
#include <sfg/core/runtime/resources/resource_manifest.hpp>
#include <sfg/core/runtime/scripting/native_component_schema.hpp>
#include <sfg/core/serialization/serialization.hpp>

namespace sfg
{
#define PROJECT_COOK_PRIMITIVE_COUNT 5

	editor_project_cooker_t::editor_project_cooker_t()	= default;
	editor_project_cooker_t::~editor_project_cooker_t() = default;

	void editor_project_cooker_t::init()
	{
		_cook_options	= new editor_project_cook_options_t{};
		_package_meta	= new project_package_meta_t{};
		_options_modal	= new editor_modal_project_cooker_t{};
		_progress_modal = new editor_modal_progress_bar_t{};

		_work_handle = {};
		_cook_state	 = cook_state_e::idle;
	}

	void editor_project_cooker_t::uninit()
	{
		SFG_ASSERT(_work_handle.is_null());

		delete _progress_modal;
		delete _options_modal;
		delete _package_meta;
		delete _cook_options;

		_progress_modal = nullptr;
		_options_modal	= nullptr;
		_package_meta	= nullptr;
		_cook_options	= nullptr;

		_cook_failure_reason.resize(0);
		_packaged_plugin_path.resize(0);
		_target_path.resize(0);
		_cook_state = cook_state_e::idle;
	}

	void editor_project_cooker_t::request_cook()
	{
		SFG_ASSERT(_cook_state == cook_state_e::idle);

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		_options_modal->request(modal, editor_settings_t::get().project_cook);
	}

	void editor_project_cooker_t::cook_project(const editor_project_cook_options_t& options)
	{
		SFG_ASSERT(_cook_state == cook_state_e::idle);

		editor_modal_controller_t&		 modal	   = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		const editor_modal_button_desc_t buttons[] = {
			{.text = "Close"},
		};

		if (options.main_world == NULL_SID || options.main_world == 0)
		{
			modal.request_modal("Cook Project", "Select a main world.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		const editor_native_manager_t& native_manager = editor_native_manager_t::get();

		if (!native_manager.is_loaded() || !native_manager.is_idle())
		{
			modal.request_modal("Cook Project", "Load the native game plugin and wait for its build to finish before cooking.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		editor_world_controller_t& world_controller = editor_app_t::get().get_world_controller();

		if (world_controller.get_play_session().is_playing() || world_controller.get_play_session().is_play_requested())
		{
			modal.request_modal("Cook Project", "Stop play mode before cooking.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		if (!world_controller.get_main_world_handle().is_null() && !world_controller.save_main_world())
		{
			modal.request_modal("Cook Project", "The main world could not be saved.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		*_cook_options = options;
		_cook_failure_reason.resize(0);
		_packaged_plugin_path.resize(0);
		_target_path.resize(0);

		_cook_state = cook_state_e::compiling_native;

		_progress_modal->set_progress(0.0f);
		const editor_modal_content_desc_t content = _progress_modal->get_content_desc();
		modal.request_modal("Cooking Project", "Compiling the packaged C++ plugin.", false, nullptr, 0, &content);

		_work_handle = editor_app_t::get().get_work_controller().submit_work({
			.fn				= [](editor_work_context_t& context, void* user_data) { return static_cast<editor_project_cooker_t*>(user_data)->compile_native_worker(); },
			.completed		= [](editor_work_handle_t handle, editor_work_state_e state, void* user_data) { static_cast<editor_project_cooker_t*>(user_data)->complete_work(handle, state == editor_work_state_e::succeeded); },
			.user_data		= this,
			.initial_status = "Compiling the packaged C++ plugin",
		});
	}

	bool editor_project_cooker_t::compile_native_worker()
	{
		const editor_project_runtime_t& project = editor_project_t::get()._runtime;

		_packaged_plugin_path = project.native_library_path + "Packaged/" + SFG_CONFIGURATION + "/stakeforge_game.dll";

		native_compile_result_t result = native_compiler_t::compile(project.native_intermediate_path.c_str(), _packaged_plugin_path.c_str(), "Packaged");

		if (!result.success)
		{
			_cook_failure_reason = "Packaged C++ plugin compilation failed.\n\n" + result.diagnostics;
			return false;
		}

		return true;
	}

	bool editor_project_cooker_t::cook_project_worker()
	{
		if (!file_system_t::ensure_directory(editor_project_t::get()._runtime.cook_path.c_str()))
		{
			_cook_failure_reason = "Failed to create the cooked project directory.";
			SFG_ERR("failed to create the cooked project directory.");
			return false;
		}

		// load the engine resources that always ship
		resource_manifest_t assets_manifest = {};

		if (!assets_manifest.load_from_file(editor_directories_t::get_assets_manifest().c_str()))
		{
			_cook_failure_reason = "Failed to load the engine resource manifest.";
			return false;
		}

		assets_manifest.resources.erase(std::remove_if(assets_manifest.resources.begin(), assets_manifest.resources.end(), [](const resource_manifest_entry_t& entry) { return entry.editor_only; }), assets_manifest.resources.end());

		// build package metadata from the cook options
		*_package_meta					 = {};
		_package_meta->project_name		 = editor_project_t::get()._runtime.name;
		_package_meta->project_settings	 = editor_project_t::get().settings.project_settings;
		_package_meta->window_resolution = _cook_options->resolution;
		_package_meta->window_style		 = _cook_options->is_borderless ? window_style_e::borderless : window_style_e::app_window;
		_package_meta->is_fullscreen	 = _cook_options->is_fullscreen;
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
		SFG_TRACE("writing cooked resource \"Cube\" ({0}, {1} bytes).", DEFAULT_MESH_CUBE_GUID, primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_sphere({}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the sphere primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_SPHERE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());
		SFG_TRACE("writing cooked resource \"Sphere\" ({0}, {1} bytes).", DEFAULT_MESH_SPHERE_GUID, primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_cylinder({}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the cylinder primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_CYLINDER_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());
		SFG_TRACE("writing cooked resource \"Cylinder\" ({0}, {1} bytes).", DEFAULT_MESH_CYLINDER_GUID, primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_capsule({}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the capsule primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_CAPSULE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());
		SFG_TRACE("writing cooked resource \"Capsule\" ({0}, {1} bytes).", DEFAULT_MESH_CAPSULE_GUID, primitive_stream.get_size());

		if (!editor_mesh_generator_t::generate_plane({.size = vec2f_t::one}, primitive_stream))
		{
			_cook_failure_reason = "Failed to generate the plane primitive.";
			return false;
		}

		resource_stream << static_cast<sid_t>(DEFAULT_MESH_PLANE_GUID);
		resource_stream << static_cast<u64>(primitive_stream.get_size());
		resource_stream.write_raw(primitive_stream.get_raw(), primitive_stream.get_size());
		SFG_TRACE("writing cooked resource \"Plane\" ({0}, {1} bytes).", DEFAULT_MESH_PLANE_GUID, primitive_stream.get_size());

		// track unique resources and their expected types
		vector_t<editor_asset_dependency_t> project_resources = {};
		hash_map_t<sid_t, resource_type_e>	resource_types	  = {};
		resource_types.reserve(editor_asset_manager_t::get().get_assets().size() + assets_manifest.resources.size() + _package_meta->worlds.size() + _cook_options->extra_resources.size() + PROJECT_COOK_PRIMITIVE_COUNT);
		resource_types.emplace(DEFAULT_MESH_CUBE_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_SPHERE_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_CYLINDER_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_CAPSULE_GUID, resource_type_e::mesh);
		resource_types.emplace(DEFAULT_MESH_PLANE_GUID, resource_type_e::mesh);

		// write every engine manifest resource
		resource_stream << static_cast<u32>(assets_manifest.resources.size());

		for (const resource_manifest_entry_t& entry : assets_manifest.resources)
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
			SFG_TRACE("writing cooked resource \"{0}\" ({1}, {2} bytes).", entry.name, sid, cached_file.get_size());

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

			const editor_asset_node_t* node = editor_asset_manager_t::get().find_asset_node(world.sid);
			SFG_ASSERT(node != nullptr);

			resource_stream.write_raw(reinterpret_cast<const u8*>(world_json.data()), world_json.size());
			SFG_TRACE("writing cooked resource \"{0}\" ({1}, {2} bytes).", node->name, world.sid, world_json.size());

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

			const editor_asset_node_t* node = editor_asset_manager_t::get().find_asset_node(resource.sid);
			SFG_ASSERT(node != nullptr);

			resource_stream.write_raw(cached_file.get_raw(), cached_file.get_size());
			SFG_TRACE("writing cooked resource \"{0}\" ({1}, {2} bytes).", node->name, resource.sid, cached_file.get_size());
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

		if (!serializer_t::save_to_file_atomic(_target_path.c_str(), meta_stream))
		{
			_cook_failure_reason = "Failed to write project package metadata.";
			return false;
		}

		if (!publish_game_files())
			return false;

		return true;
	}

	void editor_project_cooker_t::complete_work(editor_work_handle_t work_handle, bool succeeded)
	{
		SFG_ASSERT(work_handle == _work_handle);
		SFG_ASSERT(_cook_state == cook_state_e::compiling_native || _cook_state == cook_state_e::cooking);

		_work_handle = {};

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		if (_cook_state == cook_state_e::compiling_native && succeeded)
		{
			_progress_modal->set_progress(0.2f);
			modal.set_body_text("Validating the packaged native plugin.");

			plugin_runtime_t& plugin = plugin_runtime_t::get();

			succeeded = plugin.stage(_packaged_plugin_path.c_str(), _cook_failure_reason);

			if (succeeded)
			{
				const native_component_schema_delta_t delta = plugin.get_schema().compare(plugin.get_staged_schema());

				succeeded = plugin.discard_staged(_cook_failure_reason);

				if (succeeded && (!delta.added.empty() || !delta.removed.empty() || !delta.layout_changed.empty()))
				{
					_cook_failure_reason = "The packaged plugin components differ from the loaded editor plugin. Recompile the C++ project in the editor before cooking, and keep component definitions consistent across build configurations.";
					succeeded			 = false;
				}
			}

			if (succeeded)
			{
				_progress_modal->set_progress(0.3f);
				modal.set_body_text("Preparing the project package.");

				_target_path = editor_project_t::get()._runtime.cook_path + project_package_meta_t::FILE_NAME;
				_cook_state	 = cook_state_e::cooking;
				_work_handle = editor_app_t::get().get_work_controller().submit_work({
					.fn				= [](editor_work_context_t& context, void* user_data) { return static_cast<editor_project_cooker_t*>(user_data)->cook_project_worker(); },
					.completed		= [](editor_work_handle_t handle, editor_work_state_e state, void* user_data) { static_cast<editor_project_cooker_t*>(user_data)->complete_work(handle, state == editor_work_state_e::succeeded); },
					.user_data		= this,
					.initial_status = "Preparing the project package",
				});
				return;
			}

			succeeded = false;
		}

		_cook_state = cook_state_e::idle;
		modal.close_modal();

		if (!succeeded)
		{
			const editor_modal_button_desc_t buttons[] = {
				{.text = "Close"},
			};

			modal.request_modal("Cook Project Failed", _cook_failure_reason.c_str(), buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
		}
	}

	bool editor_project_cooker_t::publish_game_files()
	{
		const editor_project_runtime_t& project_runtime = editor_project_t::get()._runtime;
		const string_t					running_path	= file_system_t::get_running_directory();
		struct file_copy_t
		{
			const char* source_name = nullptr;
			const char* target_name = nullptr;
		};

		const file_copy_t runtime_files[] = {
			{.source_name = "stakeforge_game.exe", .target_name = nullptr},
			{.source_name = "stakeforge_core.dll", .target_name = "stakeforge_core.dll"},
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

		const string_t plugin_path = project_runtime.cook_path + "stakeforge_game.dll";

		if (!file_system_t::copy_file(_packaged_plugin_path.c_str(), plugin_path.c_str()))
		{
			_cook_failure_reason = "Failed to publish the native game plugin.";
			return false;
		}

		return true;
	}
}
