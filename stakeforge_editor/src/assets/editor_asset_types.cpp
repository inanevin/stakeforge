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

#include "assets/editor_asset_types.hpp"

#include "assets/editor_asset_manager.hpp"

#include <sfg/math/color.hpp>

namespace sfg
{
#define EDITOR_ASSET_COLOR(R, G, B) color_t::from255(R, G, B, 255.0f).srgb_to_linear().to_vector()

	void editor_asset_loader_audio_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.extensions = {"mp3"}, .display_name = "Audio", .color = EDITOR_ASSET_COLOR(64.0f, 177.0f, 255.0f), .asset_type = editor_asset_type_e::audio});
	}

	void editor_asset_loader_font_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.extensions = {"ttf"}, .display_name = "Font", .color = EDITOR_ASSET_COLOR(245.0f, 194.0f, 82.0f), .asset_type = editor_asset_type_e::font});
	}

	void editor_asset_loader_mesh_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.extensions = {"glb"}, .display_name = "Mesh", .color = EDITOR_ASSET_COLOR(158.0f, 120.0f, 255.0f), .asset_type = editor_asset_type_e::mesh});
	}

	void editor_asset_loader_skeleton_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "Skeleton", .color = EDITOR_ASSET_COLOR(184.0f, 155.0f, 255.0f), .asset_type = editor_asset_type_e::skeleton});
	}

	void editor_asset_loader_animation_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "Animation", .color = EDITOR_ASSET_COLOR(255.0f, 129.0f, 80.0f), .asset_type = editor_asset_type_e::animation});
	}

	void editor_asset_loader_material_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "Material", .color = EDITOR_ASSET_COLOR(255.0f, 102.0f, 0.0f), .asset_type = editor_asset_type_e::material});
	}

	void editor_asset_loader_shader_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.extensions = {"hlsl"}, .display_name = "Shader", .color = EDITOR_ASSET_COLOR(90.0f, 190.0f, 255.0f), .asset_type = editor_asset_type_e::shader});
	}

	void editor_asset_loader_texture_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.extensions = {"png", "jpg", "jpeg"}, .display_name = "Texture", .color = EDITOR_ASSET_COLOR(151.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture});
	}

	void editor_asset_loader_texture_sampler_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "Texture Sampler", .color = EDITOR_ASSET_COLOR(180.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture_sampler});
	}

	void editor_asset_loader_physical_material_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "Physical Material", .color = EDITOR_ASSET_COLOR(214.0f, 65.0f, 57.0f), .asset_type = editor_asset_type_e::physical_material});
	}

	void editor_asset_loader_prefab_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "Prefab", .color = EDITOR_ASSET_COLOR(107.0f, 210.0f, 132.0f), .asset_type = editor_asset_type_e::prefab});
	}

	void editor_asset_loader_animation_state_machine_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.display_name = "State Machine", .color = EDITOR_ASSET_COLOR(245.0f, 118.0f, 182.0f), .asset_type = editor_asset_type_e::animation_state_machine});
	}
}
