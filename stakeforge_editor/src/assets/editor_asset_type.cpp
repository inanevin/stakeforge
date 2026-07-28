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

#include "editor_asset_type.hpp"

#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_asset_type_e& t)
	{
		switch (t)
		{
		case editor_asset_type_e::audio:
			j = "audio";
			break;
		case editor_asset_type_e::font:
			j = "font";
			break;
		case editor_asset_type_e::mesh:
			j = "mesh";
			break;
		case editor_asset_type_e::skeleton:
			j = "skeleton";
			break;
		case editor_asset_type_e::animation:
			j = "animation";
			break;
		case editor_asset_type_e::material:
			j = "material";
			break;
		case editor_asset_type_e::shader:
			j = "shader";
			break;
		case editor_asset_type_e::texture:
			j = "texture";
			break;
		case editor_asset_type_e::texture_sampler:
			j = "texture_sampler";
			break;
		case editor_asset_type_e::physical_material:
			j = "physical_material";
			break;
		case editor_asset_type_e::prefab:
			j = "prefab";
			break;
		case editor_asset_type_e::animation_graph:
			j = "animation_graph";
			break;
		case editor_asset_type_e::cubemap:
			j = "cubemap";
			break;
		case editor_asset_type_e::physics_collision_mesh:
			j = "physics_collision_mesh";
			break;
		case editor_asset_type_e::sprite:
			j = "sprite";
			break;
		case editor_asset_type_e::curve:
			j = "curve";
			break;
		case editor_asset_type_e::world:
			j = "world";
			break;
		default:
			j = "invalid";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_asset_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "audio")
			t = editor_asset_type_e::audio;
		else if (s == "font")
			t = editor_asset_type_e::font;
		else if (s == "mesh")
			t = editor_asset_type_e::mesh;
		else if (s == "skeleton")
			t = editor_asset_type_e::skeleton;
		else if (s == "animation")
			t = editor_asset_type_e::animation;
		else if (s == "material")
			t = editor_asset_type_e::material;
		else if (s == "shader")
			t = editor_asset_type_e::shader;
		else if (s == "texture")
			t = editor_asset_type_e::texture;
		else if (s == "texture_sampler")
			t = editor_asset_type_e::texture_sampler;
		else if (s == "physical_material")
			t = editor_asset_type_e::physical_material;
		else if (s == "prefab")
			t = editor_asset_type_e::prefab;
		else if (s == "animation_graph")
			t = editor_asset_type_e::animation_graph;
		else if (s == "cubemap" || s == "hdr_skybox")
			t = editor_asset_type_e::cubemap;
		else if (s == "physics_collision_mesh")
			t = editor_asset_type_e::physics_collision_mesh;
		else if (s == "sprite")
			t = editor_asset_type_e::sprite;
		else if (s == "curve")
			t = editor_asset_type_e::curve;
		else if (s == "world")
			t = editor_asset_type_e::world;
		else
			t = editor_asset_type_e::invalid;
	}

	editor_asset_type_e editor_asset_type_from_resource_type(resource_type_e type)
	{
		switch (type)
		{
		case resource_type_e::audio:
			return editor_asset_type_e::audio;
		case resource_type_e::font:
			return editor_asset_type_e::font;
		case resource_type_e::mesh:
			return editor_asset_type_e::mesh;
		case resource_type_e::skeleton:
			return editor_asset_type_e::skeleton;
		case resource_type_e::animation:
			return editor_asset_type_e::animation;
		case resource_type_e::material:
			return editor_asset_type_e::material;
		case resource_type_e::shader:
			return editor_asset_type_e::shader;
		case resource_type_e::texture:
			return editor_asset_type_e::texture;
		case resource_type_e::texture_sampler:
			return editor_asset_type_e::texture_sampler;
		case resource_type_e::physical_material:
			return editor_asset_type_e::physical_material;
		case resource_type_e::prefab:
			return editor_asset_type_e::prefab;
		case resource_type_e::animation_graph:
			return editor_asset_type_e::animation_graph;
		case resource_type_e::cubemap:
			return editor_asset_type_e::cubemap;
		case resource_type_e::physics_collision_mesh:
			return editor_asset_type_e::physics_collision_mesh;
		case resource_type_e::sprite:
			return editor_asset_type_e::sprite;
		case resource_type_e::curve:
			return editor_asset_type_e::curve;
		default:
			return editor_asset_type_e::invalid;
		}
	}

	editor_asset_type_e editor_asset_type_from_reflection_sub_type_id(sid_t sub_type_id)
	{
		if (sub_type_id == SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_WORLD)
			return editor_asset_type_e::world;

		if (sub_type_id == SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_ANY_RESOURCE)
			return editor_asset_type_e::invalid;

		return editor_asset_type_from_resource_type(resource_type_from_reflection_sub_type_id(sub_type_id));
	}
}
