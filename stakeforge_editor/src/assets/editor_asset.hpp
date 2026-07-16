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

#pragma once

#include "assets/editor_asset_node.hpp"
#include "assets/editor_asset_type.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
#define DEFAULT_GBUFFER_SHADER_ASSET_GUID			  1000
#define DEFAULT_FORWARD_SHADER_ASSET_GUID			  1001
#define DEFAULT_ALBEDO_TEXTURE_ASSET_GUID			  1002
#define DEFAULT_ORM_TEXTURE_ASSET_GUID				  1003
#define DEFAULT_NORMAL_TEXTURE_ASSET_GUID			  1004
#define DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID			  1005
#define DEFAULT_GBUFFER_MATERIAL_ASSET_GUID			  1006
#define DEFAULT_FORWARD_MATERIAL_ASSET_GUID			  1007
#define DEFAULT_PHYSICAL_MATERIAL_ASSET_GUID		  1008
#define DEFAULT_LINEAR_SAMPLER_ASSET_GUID			  1009
#define DEFAULT_NEAREST_SAMPLER_ASSET_GUID			  1010
#define DEFAULT_ANISOTROPIC_SAMPLER_ASSET_GUID		  1011
#define DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID		  1012
#define DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID	  1013
#define DEFAULT_NEAREST_SAMPLER_REPEAT_ASSET_GUID	  1014
#define DEFAULT_MESH_CUBE_GUID						  1015
#define DEFAULT_MESH_SPHERE_GUID					  1016
#define DEFAULT_MESH_CYLINDER_GUID					  1017
#define DEFAULT_MESH_CAPSULE_GUID					  1018
#define DEFAULT_ANISOTROPIC_SAMPLER_REPEAT_ASSET_GUID 1019
#define GIZMO_MESH_TRANSLATION						  1020
#define GIZMO_MESH_ROTATION							  1021
#define GIZMO_MESH_SCALE							  1022
#define DEFAULT_UNLIT_SHADER_ASSET_GUID				  1023
#define DEFAULT_UNLIT_MATERIAL_ASSET_GUID			  1024

	enum class editor_asset_source_type_e : u8
	{
		none,
		file,
		file_blob,
		embedded,
	};

	enum class editor_asset_status_e : u8
	{
		ok,
		missing_dependency,
		missing_embedded_data,
		missing_file_source,
	};

	struct editor_asset_t
	{
		static constexpr u32 VERSION = 0;

		string_t				   embedded_source = {};
		string_t				   cook_options	   = "{}";
		string_t				   source_relative = {};
		u32						   version		   = 0;
		sid_t					   guid			   = NULL_SID;
		sid_t					   thumbnail_guid  = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		editor_asset_source_type_e source_type	   = editor_asset_source_type_e::file;
		editor_asset_status_e	   status		   = editor_asset_status_e::ok;
		u8						   sub_type		   = 0;
	};

	struct editor_asset_descriptor_t
	{
		vector_t<string_t>	extensions	 = {};
		string_t			display_name = {};
		vec4f_t				color		 = {};
		editor_asset_type_e asset_type	 = editor_asset_type_e::invalid;
	};

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t);
	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t);
	void to_json(nlohmann::json& j, const editor_asset_t& asset);
	void from_json(const nlohmann::json& j, editor_asset_t& asset);
}
