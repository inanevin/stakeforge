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

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
#define ENTITIES_INITIAL_ROW_CAPACITY 64
#define ENTITIES_INDENT_MULT		  2.0f

	enum entity_action_menu_command_e : u16
	{
		entity_action_menu_create_empty = 1,
		entity_action_menu_create_folder,
		entity_action_menu_create_cube,
		entity_action_menu_create_sphere,
		entity_action_menu_create_cylinder,
		entity_action_menu_create_capsule,
		entity_action_menu_create_plane,
		entity_action_menu_duplicate,
		entity_action_menu_hide,
		entity_action_menu_show_alone,
		entity_action_menu_delete,
		entity_action_menu_rename_folder,
		entity_action_menu_change_folder_color,
		entity_action_menu_delete_folder,
	};
}
