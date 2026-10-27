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
#include "tools/editor_project.hpp"
#include <sfg/core/data/string_util.hpp>
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/process.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/runtime/engine/engine_runtime.hpp>
#include <sfg/core/runtime/scripting/engine_component_binding_generator.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

#include <cstddef>

namespace sfg
{
	editor_project_reflection_t g_reflect_editor_project = {};

	editor_project_t editor_project_t::make_default_project(const char* path)
	{
		editor_project_t project = {};
		project.refresh_runtime(path);
		project.settings.last_world_guid = NULL_SID;
		return project;
	}

	bool editor_project_t::save(const char* path)
	{
		nlohmann::json js = nlohmann::json::object();
		if (!reflection_registry_t::get().type_to_json(type_id_t<editor_project_t>::value, this, nullptr, js))
			return false;
		const string_t contents = js.dump(4);
		return serializer_t::write_to_file(contents, path);
	}

	bool editor_project_t::try_load(const char* path)
	{
		if (!file_system_t::exists(path))
			return false;

		const string_t&		 contents = file_system_t::read_file_as_string(path);
		const nlohmann::json js		  = nlohmann::json::parse(contents, nullptr, false);
		if (js.is_discarded())
			return false;

		editor_project_t project = {};
		if (!reflection_registry_t::get().type_from_json(type_id_t<editor_project_t>::value, &project, nullptr, js))
			return false;

		*this = project;
		settings.project_settings.normalize();
		refresh_runtime(path);
		engine_runtime_t::get().update_project_settings(settings.project_settings);
		return true;
	}

	bool editor_project_t::ensure_script_project()
	{
		if (!file_system_t::ensure_directory(_runtime.csharp_intermediate_path.c_str()))
			return false;

		if (!file_system_t::ensure_directory(_runtime.script_library_path.c_str()))
			return false;

		const string_t engine_component_bindings_path = _runtime.csharp_intermediate_path + "EngineComponents.g.cs";

		if (!engine_component_binding_generator_t::generate(engine_component_bindings_path.c_str()))
			return false;

		string_t managed_assembly_path = file_system_t::get_running_directory() + "managed/Stakeforge.Managed.dll";
		file_system_t::fix_path(managed_assembly_path);
		string_util::replace_all(managed_assembly_path, "&", "&amp;");
		string_util::replace_all(managed_assembly_path, "\"", "&quot;");
		string_util::replace_all(managed_assembly_path, "<", "&lt;");
		string_util::replace_all(managed_assembly_path, ">", "&gt;");

		string_t contents = R"(<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
    <OutputPath>Build/$(Configuration)/</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <AppendRuntimeIdentifierToOutputPath>false</AppendRuntimeIdentifierToOutputPath>
  </PropertyGroup>
  <ItemGroup>
    <Compile Include="EngineComponents.g.cs" />
    <Compile Include="../../assets/**/*.cs" Exclude="../../assets/_cache/**;../../assets/_sfg_assets/**;../../assets/_game_assets/**" />
    <Reference Include="Stakeforge.Managed">
      <HintPath>)";
		contents += managed_assembly_path;
		contents += R"(</HintPath>
      <Private>false</Private>
    </Reference>
  </ItemGroup>
</Project>
)";

		if (file_system_t::exists(_runtime.script_project_path.c_str()))
		{
			const string_t existing_contents = file_system_t::read_file_as_string(_runtime.script_project_path.c_str());

			if (existing_contents == contents)
				return true;
		}

		return serializer_t::write_to_file(contents, _runtime.script_project_path.c_str());
	}

	bool editor_project_t::ensure_native_project()
	{
		const string_t executable_path = process::get_executable_path();

		if (executable_path.empty())
			return false;

		const string_t sdk_path		 = file_system_t::get_directory_of_file(executable_path.c_str()) + "stakeforge_sdk/";
		const string_t settings_path = sdk_path + "build_settings.json";
		const string_t cmake_path	 = sdk_path + "tools/cmake/bin/cmake.exe";
		const string_t package_path	 = sdk_path + "cmake/StakeforgeConfig.cmake";

		if (!file_system_t::exists(settings_path.c_str()) || !file_system_t::exists(cmake_path.c_str()) || !file_system_t::exists(package_path.c_str()))
		{
			SFG_ERR("native project generation needs a complete SDK at {0}", sdk_path);
			return false;
		}

		const string_t		 settings_text = file_system_t::read_file_as_string(settings_path.c_str());
		const nlohmann::json settings	   = nlohmann::json::parse(settings_text, nullptr, false);

		if (settings.is_discarded() || !settings.is_object())
		{
			SFG_ERR("could not read SDK build settings at {0}", settings_path);
			return false;
		}

		const nlohmann::json build_info	  = settings.value<nlohmann::json>("build_info", nlohmann::json::object());
		const string_t		 generator	  = settings.value<string_t>("generator", "");
		const string_t		 architecture = settings.value<string_t>("architecture", "");
		const string_t		 toolset	  = settings.value<string_t>("toolset", "");
		const string_t		 windows_sdk  = settings.value<string_t>("windows_sdk", "");

		if (settings.value<i32>("sdk_format", 0) != 1 || settings.value<string_t>("configuration", "") != SFG_CONFIGURATION || build_info.value<string_t>("build_id", "") != SFG_BUILD)
		{
			SFG_ERR("SDK does not match this editor build ({0}, {1}): {2}", SFG_BUILD, SFG_CONFIGURATION, settings_path);
			return false;
		}

		if (!generator.starts_with("Visual Studio ") || architecture.empty() || toolset.empty() || windows_sdk.empty())
		{
			SFG_ERR("SDK does not contain the Visual Studio generator, architecture, toolset and Windows SDK settings needed to generate a solution: {0}", settings_path);
			return false;
		}

		const string_t source_path	= file_system_t::get_absolute_path(_runtime.native_source_path.c_str());
		const string_t build_path	= file_system_t::get_absolute_path(_runtime.native_intermediate_path.c_str());
		const string_t library_path = file_system_t::get_absolute_path(_runtime.native_library_path.c_str());

		if (source_path.empty() || build_path.empty() || library_path.empty() || !file_system_t::ensure_directory(source_path.c_str()) || !file_system_t::ensure_directory(build_path.c_str()))
		{
			SFG_ERR("could not create native project directories for {0}", _runtime.path);
			return false;
		}

		for (const char* name : {"CMakeLists.txt", "game.cpp", "game_components.hpp", "game_type_registry.hpp", "game_type_registry.cpp"})
		{
			const string_t destination = _runtime.native_source_path + name;

			if (file_system_t::exists(destination.c_str()))
				continue;

			const string_t source = sdk_path + "templates/native/" + name;

			if (!file_system_t::copy_file(source.c_str(), destination.c_str()))
			{
				SFG_ERR("could not create native project source {0}", destination);
				return false;
			}
		}

		const string_t signature_path = _runtime.native_intermediate_path + "stakeforge_generation.txt";
		const string_t signature	  = sdk_path + "\n" + source_path + "\n" + library_path + "\n" + settings_text;
		const bool	   fresh		  = !file_system_t::exists(signature_path.c_str()) || file_system_t::read_file_as_string(signature_path.c_str()) != signature;
		string_t	   command		  = "\"" + cmake_path + "\" -S \"" + source_path + "\" -B \"" + build_path + "\"";
		command += " -G \"" + generator + "\" -A \"" + architecture + ",version=" + windows_sdk + "\" -T \"" + toolset + "\"";
		command += " \"-DStakeforge_DIR=" + sdk_path + "cmake\"";
		command += " \"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=" + library_path + "\"";
		command += " \"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=" + library_path + "\"";

		if (fresh)
			command += " --fresh";

		SFG_INFO("generating native game solution in {0}", build_path);

		const process_execute_result_t result	= process::execute(command.c_str());
		const string_t				   log_path = _runtime.native_intermediate_path + "generation.log";
		const string_t				   log		= command + "\n\n" + result.output;

		if (!serializer_t::write_to_file(log, log_path.c_str()))
			SFG_ERR("could not save native project generation log to {0}", log_path);

		if (!result.started || result.exit_code != 0)
		{
			SFG_ERR("native project generation failed (exit {0}). Install the Visual Studio C++ tools ({1}) and Windows SDK {2} required by this engine, or fix the CMake errors below. Log: {3}\n{4}",
					result.exit_code,
					toolset,
					windows_sdk,
					log_path,
					result.output);
			return false;
		}

		if (!serializer_t::write_to_file(signature, signature_path.c_str()))
		{
			SFG_ERR("could not save native project generation settings to {0}", signature_path);
			return false;
		}

		SFG_INFO("native game solution ready in {0}", build_path);
		return true;
	}

	void editor_project_t::refresh_runtime(const char* path)
	{
		_runtime = {};

		_runtime.path = path;
		file_system_t::fix_path(_runtime.path);

		_runtime.name	   = file_system_t::get_filename_from_path(path);
		const string_t dir = file_system_t::get_directory_of_file(_runtime.path.c_str());

		_runtime.assets_path			  = dir + "assets/";
		_runtime.cache_path				  = _runtime.assets_path + "_cache/";
		_runtime.default_assets_path	  = _runtime.assets_path + "_sfg_assets/";
		_runtime.game_assets_path		  = _runtime.assets_path + "_game_assets/";
		_runtime.cook_path				  = dir + "Cooked/";
		_runtime.intermediate_path		  = dir + "Intermediate/";
		_runtime.csharp_intermediate_path = _runtime.intermediate_path + "CSharp/";
		_runtime.script_project_path	  = _runtime.csharp_intermediate_path + _runtime.name + ".csproj";
		_runtime.library_path			  = dir + "Library/";
		_runtime.script_library_path	  = _runtime.library_path + "Scripts/";
		_runtime.native_source_path		  = dir + "Source/";
		_runtime.native_intermediate_path = _runtime.intermediate_path + "Native/" + SFG_BUILD + "/" + SFG_CONFIGURATION + "/";
		_runtime.native_library_path	  = _runtime.library_path + "Native/" + SFG_BUILD + "/";

		if (!file_system_t::exists(_runtime.assets_path.c_str()))
			file_system_t::create_directory(_runtime.assets_path.c_str());

		if (!file_system_t::exists(_runtime.cache_path.c_str()))
			file_system_t::create_directory(_runtime.cache_path.c_str());

		if (!file_system_t::exists(_runtime.default_assets_path.c_str()))
			file_system_t::create_directory(_runtime.default_assets_path.c_str());

		if (!file_system_t::exists(_runtime.game_assets_path.c_str()))
			file_system_t::create_directory(_runtime.game_assets_path.c_str());

		if (!file_system_t::exists(_runtime.cook_path.c_str()))
			file_system_t::create_directory(_runtime.cook_path.c_str());

		if (!ensure_script_project())
			SFG_ERR("could not ensure the C# script project.");
	}

}

namespace sfg
{
	editor_project_reflection_t::editor_project_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		registry.register_type({
			.name		  = "editor_project_settings_data_t",
			.display_name = "Editor Project Settings",
			.fields =
				{
					{.name = "last_world_guid", .display_name = "Last World GUID", .offset = offsetof(editor_project_settings_data_t, last_world_guid), .size = sizeof(sid_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{
						.name		  = "world_view_settings",
						.display_name = "World View Settings",
						.sub_type_id  = type_id_t<editor_world_view_settings_t>::value,
						.offset		  = offsetof(editor_project_settings_data_t, world_view_settings),
						.size		  = sizeof(editor_world_view_settings_t),
						.flags		  = reflected_field_flag_no_ui,
						.type		  = reflected_value_type_e::object,
					},
					{.name		   = "world_view_snapping_enabled",
					 .display_name = "World View Snapping",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_snapping_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_grid_enabled",
					 .display_name = "World View Grid",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_grid_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_aabb_enabled",
					 .display_name = "World View AABB",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_aabb_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_physics_debug_enabled",
					 .display_name = "World View Physics Debug",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_physics_debug_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{
						.name		  = "world_view_perf_metrics_enabled",
						.display_name = "World View Performance Metrics",
						.offset		  = offsetof(editor_project_settings_data_t, world_view_perf_metrics_enabled),
						.size		  = sizeof(bool),
						.flags		  = reflected_field_flag_no_ui,
						.type		  = reflected_value_type_e::boolean,
					},
					{.name		   = "project_settings",
					 .display_name = "Project",
					 .sub_type_id  = type_id_t<project_settings_t>::value,
					 .offset	   = offsetof(editor_project_settings_data_t, project_settings),
					 .size		   = sizeof(project_settings_t),
					 .type		   = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<editor_project_settings_data_t>::value,
			.size	   = sizeof(editor_project_settings_data_t),
			.alignment = alignof(editor_project_settings_data_t),
		});
		registry.register_type({
			.name		  = "editor_project_t",
			.display_name = "Project Settings",
			.fields =
				{
					{.name		   = "settings",
					 .display_name = "Settings",
					 .sub_type_id  = type_id_t<editor_project_settings_data_t>::value,
					 .offset	   = offsetof(editor_project_t, settings),
					 .size		   = sizeof(editor_project_settings_data_t),
					 .type		   = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<editor_project_t>::value,
			.size	   = sizeof(editor_project_t),
			.alignment = alignof(editor_project_t),
		});
	}
}
