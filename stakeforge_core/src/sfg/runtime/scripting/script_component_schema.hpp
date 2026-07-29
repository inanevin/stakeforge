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
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	struct script_component_field_desc_t
	{
		string_t			   name		   = {};
		sid_t				   field_id	   = 0;
		sid_t				   sub_type_id = 0;
		u32					   offset	   = 0;
		u32					   size		   = 0;
		bitmask32			   flags	   = 0;
		reflected_value_type_e value_type  = reflected_value_type_e::invalid;
	};

	struct script_component_desc_t
	{
		vector_t<script_component_field_desc_t> fields	  = {};
		string_t								name	  = {};
		string_t								full_name = {};
		sid_t									type_id	  = 0;
		u32										size	  = 0;
		u32										alignment = 1;

		const script_component_field_desc_t* find_field(sid_t field_id) const;
		bool								 is_layout_equal(const script_component_desc_t& other) const;
		bool								 is_reflection_equal(const script_component_desc_t& other) const;
	};

	struct script_component_schema_delta_t
	{
		vector_t<sid_t> added			   = {};
		vector_t<sid_t> removed			   = {};
		vector_t<sid_t> layout_changed	   = {};
		vector_t<sid_t> reflection_changed = {};

		bool has_changes() const;
	};

	class script_component_schema_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool							parse(const char* schema_json);
		void							register_reflection_types() const;
		script_component_schema_delta_t compare(const script_component_schema_t& candidate) const;

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		const script_component_desc_t* find_component(sid_t type_id) const;

		inline const vector_t<script_component_desc_t>& get_components() const
		{
			return _components;
		}

		size_t get_field_count() const;

	private:
		vector_t<script_component_desc_t> _components = {};
	};
}
