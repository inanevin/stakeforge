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

#include "editor_project_cook_options.hpp"
#include "assets/editor_asset_type.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	editor_project_cook_options_reflection_t::editor_project_cook_options_reflection_t()
	{
		reflection_registry_t::get().register_type({
			.name		  = "editor_project_cook_options_t",
			.display_name = "Project Cook Options",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops_with_default<resource_handle_t, NULL_RESOURCE_HANDLE>(reflected_value_type_e::u64, SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_WORLD),
					 .name			= "worlds",
					 .display_name	= "Worlds",
					 .offset		= offsetof(editor_project_cook_options_t, worlds),
					 .size			= sizeof(vector_t<resource_handle_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops_with_default<resource_handle_t, NULL_RESOURCE_HANDLE>(reflected_value_type_e::u64, SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_ANY_RESOURCE),
					 .name			= "extra_resources",
					 .display_name	= "Extra Resources",
					 .offset		= offsetof(editor_project_cook_options_t, extra_resources),
					 .size			= sizeof(vector_t<resource_handle_t>),
					 .type			= reflected_value_type_e::container},
					{.name = "is_borderless", .display_name = "Borderless", .offset = offsetof(editor_project_cook_options_t, is_borderless), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "is_fullscreen", .display_name = "Fullscreen", .offset = offsetof(editor_project_cook_options_t, is_fullscreen), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.ui_definition = {.dependency_field = "is_fullscreen"_hs, .dependency_value = 0, .dependency_type = reflected_field_dependency_type_e::show_if_equals},
					 .name			= "resolution",
					 .display_name	= "Resolution",
					 .sub_type_id	= type_id_t<vec2u16_t>::value,
					 .offset		= offsetof(editor_project_cook_options_t, resolution),
					 .size			= sizeof(vec2u16_t),
					 .type			= reflected_value_type_e::object},
				},
			.type_id   = type_id_t<editor_project_cook_options_t>::value,
			.size	   = sizeof(editor_project_cook_options_t),
			.alignment = alignof(editor_project_cook_options_t),
		});
	}
}
