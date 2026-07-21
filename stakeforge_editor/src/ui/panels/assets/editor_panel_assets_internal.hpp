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
#define ASSETS_PANE_SPLIT_MIN				 0.15f
#define ASSETS_PANE_SPLIT_MAX				 0.35f
#define ASSETS_SPLIT_BORDER_THICKNESS_MULT	 2.0f
#define ASSETS_FOLDER_INDENT_MULT			 2.0f
#define ASSETS_INITIAL_ROW_CAPACITY			 64
#define ASSETS_INITIAL_GRID_ITEM_CAPACITY	 128
#define ASSETS_FILTER_ID_ALL				 0
#define ASSETS_FILTER_ID_FAVOURITES			 1
#define ASSETS_ITEM_STYLE_ID_GRID			 0
#define ASSETS_ITEM_STYLE_ID_LIST			 1
#define ASSETS_IMPORT_FILE_MAX				 255
#define ASSETS_IMPORT_FILE_EXTENSIONS		 "glb;png;jpg;jpeg;hdr;mp3;ttf"
#define ASSETS_FIX_INTEGRITY_FILE_EXTENSIONS "glb;png;jpg;jpeg;hdr;mp3;ttf;hlsl"

	enum assets_action_menu_command_e : u16
	{
		assets_action_menu_create_folder				  = 1,
		assets_action_menu_delete						  = 2,
		assets_action_menu_duplicate					  = 3,
		assets_action_menu_toggle_favourite				  = 4,
		assets_action_menu_open_directory				  = 5,
		assets_action_menu_rename						  = 6,
		assets_action_menu_create_animation_state_machine = 7,
		assets_action_menu_create_opaque_shader			  = 8,
		assets_action_menu_create_transparent_shader	  = 9,
		assets_action_menu_create_post_process_shader	  = 10,
		assets_action_menu_create_ui_shader				  = 11,
		assets_action_menu_create_ui_text_shader		  = 12,
		assets_action_menu_create_texture_sampler		  = 13,
		assets_action_menu_create_gbuffer_material		  = 14,
		assets_action_menu_create_forward_material		  = 15,
		assets_action_menu_create_physical_material		  = 16,
		assets_item_action_menu_rename					  = 17,
		assets_item_action_menu_duplicate				  = 18,
		assets_item_action_menu_delete					  = 19,
		assets_item_action_menu_open_directory			  = 20,
		assets_item_action_menu_toggle_favourite		  = 21,
		assets_action_menu_import						  = 22,
		assets_item_action_menu_fix_integrity			  = 23,
		assets_action_menu_create_world					  = 24,
		assets_action_menu_create_unlit_shader			  = 25,
		assets_action_menu_create_unlit_material		  = 26,
		assets_action_menu_import_orm_texture			  = 27,
	};
}
