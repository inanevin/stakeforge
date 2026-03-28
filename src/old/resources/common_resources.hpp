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

#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"
#include "common/string_id.hpp"

#define NOMINMAX
#include <limits>

namespace SFG
{

#define DUMMY_COLOR_TEXTURE_SID	 UINT64_MAX - 1000
#define DUMMY_NORMAL_TEXTURE_SID UINT64_MAX - 999
#define DUMMY_ORM_TEXTURE_SID	 UINT64_MAX - 998
#define DUMMY_SAMPLER_SID		 UINT64_MAX - 997

#define DEFAULT_OPAQUE_SHADER_PATH "assets/engine/shaders/world/gbuffer_lit.stkshader"
#define DEFAULT_OPAQUE_SHADER_SID  "assets/engine/shaders/world/gbuffer_lit.stkshader"_hs

#define DEFAULT_FORWARD_SHADER_PATH "assets/engine/shaders/world/forward.stkshader"
#define DEFAULT_FORWARD_SHADER_SID	"assets/engine/shaders/world/forward.stkshader"_hs

#define DEFAULT_GUI_SHADER_PATH "assets/engine/shaders/world/gui_default.stkshader"
#define DEFAULT_GUI_SHADER_SID	"assets/engine/shaders/world/gui_default.stkshader"_hs

#define DEFAULT_GUI_TEXT_SHADER_PATH "assets/engine/shaders/world/gui_text.stkshader"
#define DEFAULT_GUI_TEXT_SHADER_SID	 "assets/engine/shaders/world/gui_text.stkshader"_hs

#define DEFAULT_GUI_SDF_SHADER_PATH "assets/engine/shaders/world/gui_sdf.stkshader"
#define DEFAULT_GUI_SDF_SHADER_SID	"assets/engine/shaders/world/gui_sdf.stkshader"_hs

#define DEFAULT_GUI_MAT_PATH "assets/engine/materials/world/gui_default.stkmat"
#define DEFAULT_GUI_MAT_SID	 "assets/engine/materials/world/gui_default.stkmat"_hs

#define DEFAULT_GUI_TEXT_MAT_PATH "assets/engine/materials/world/gui_text.stkmat"
#define DEFAULT_GUI_TEXT_MAT_SID  "assets/engine/materials/world/gui_text.stkmat"_hs

#define DEFAULT_GUI_SDF_MAT_PATH "assets/engine/materials/world/gui_sdf.stkmat"
#define DEFAULT_GUI_SDF_MAT_SID	 "assets/engine/materials/world/gui_sdf.stkmat"_hs

	typedef pool_handle16 resource_handle;
	typedef u16		  resource_id;

	struct resource_handle_and_type
	{
		resource_handle handle	= {};
		string_id		type_id = 0;
	};

#define NULL_RESOURCE_ID std::numeric_limits<resource_id>::max()

	enum material_flags : u32
	{
		material_flags_is_gbuffer	   = 1 << 0,
		material_flags_is_alpha_cutoff = 1 << 1,
		material_flags_is_forward	   = 1 << 2,
		material_flags_is_double_sided = 1 << 3,
		material_flags_is_gui		   = 1 << 4,
		material_flags_is_particle	   = 1 << 5,
		material_flags_is_sprite	   = 1 << 6,
		material_flags_created		   = 1 << 7,
	};

	enum shader_variant_flags : u32
	{
		variant_flag_skinned			= 1 << 0,
		variant_flag_alpha_cutoff		= 1 << 1,
		variant_flag_z_prepass			= 1 << 2,
		variant_flag_double_sided		= 1 << 3,
		variant_flag_shadow_rendering	= 1 << 4,
		variant_flag_selection_outline	= 1 << 5,
		variant_flag_id_write			= 1 << 6,
		variant_flag_gui_3d				= 1 << 7,
		variant_flag_console			= 1 << 8,
		variant_flag_console_and_editor = 1 << 9,
	};

}
