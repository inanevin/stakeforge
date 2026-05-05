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
#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	enum class reflected_field_type : u8
	{
		rf_float = 0,
		rf_int,
		rf_uint,
		rf_vector2,
		rf_vector3,
		rf_vector4,
		rf_vector2ui16,
		rf_color,
		rf_resource,
		rf_entity,
		rf_bool,
		rf_uint8,
		rf_string,
		rf_enum,
	};

#define SFG_PROP_TYPE_float			sfg::reflected_field_type::rf_float
#define SFG_PROP_TYPE_int			sfg::reflected_field_type::rf_int
#define SFG_PROP_TYPE_float_limited sfg::reflected_field_type::rf_float_clamped
#define SFG_PROP_TYPE_int_limited	sfg::reflected_field_type::rf_int_clamped
#define SFG_PROP_TYPE_vector2		sfg::reflected_field_type::rf_vector2
#define SFG_PROP_TYPE_vector3		sfg::reflected_field_type::rf_vector3
#define SFG_PROP_TYPE_vector4		sfg::reflected_field_type::rf_vector4
#define SFG_PROP_TYPE_color			sfg::reflected_field_type::rf_color
#define SFG_PROP_TYPE_resource		sfg::reflected_field_type::rf_resource
#define SFG_PROP_TYPE_entity		sfg::reflected_field_type::rf_entity

	class field_base_t;
	class world_t;

	struct reflected_field_changed_params_t
	{
		world_t& w;
		void*	 object_ptr	 = nullptr;
		void*	 data_ptr	 = nullptr;
		sid_t	 field_title = 0;
		u32		 list_index	 = 0;
	};

	struct reflected_button_params_t
	{
		world_t& w;
		void*	 object_ptr = nullptr;
		sid_t	 button_id	= 0;
	};
};
