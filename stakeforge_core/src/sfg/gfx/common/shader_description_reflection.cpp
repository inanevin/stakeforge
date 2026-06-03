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

#include "shader_description_reflection.hpp"
#include "format_reflection.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t cull_mode_values[] = {
			{.name = "none", .display_name = "None", .value = static_cast<i64>(cull_mode::none)},
			{.name = "front", .display_name = "Front", .value = static_cast<i64>(cull_mode::front)},
			{.name = "back", .display_name = "Back", .value = static_cast<i64>(cull_mode::back)},
		};

		static const reflected_enum_value_desc_t fill_mode_values[] = {
			{.name = "solid", .display_name = "Solid", .value = static_cast<i64>(fill_mode::solid)},
			{.name = "wireframe", .display_name = "Wireframe", .value = static_cast<i64>(fill_mode::wireframe)},
		};

		static const reflected_enum_value_desc_t front_face_values[] = {
			{.name = "ccw", .display_name = "Counter Clockwise", .value = static_cast<i64>(front_face::ccw)},
			{.name = "cw", .display_name = "Clockwise", .value = static_cast<i64>(front_face::cw)},
		};

		static const reflected_enum_value_desc_t blend_factor_values[] = {
			{.name = "zero", .display_name = "Zero", .value = static_cast<i64>(blend_factor::zero)},
			{.name = "one", .display_name = "One", .value = static_cast<i64>(blend_factor::one)},
			{.name = "src_color", .display_name = "Src Color", .value = static_cast<i64>(blend_factor::src_color)},
			{.name = "one_minus_src_color", .display_name = "One Minus Src Color", .value = static_cast<i64>(blend_factor::one_minus_src_color)},
			{.name = "dst_color", .display_name = "Dst Color", .value = static_cast<i64>(blend_factor::dst_color)},
			{.name = "one_minus_dst_color", .display_name = "One Minus Dst Color", .value = static_cast<i64>(blend_factor::one_minus_dst_color)},
			{.name = "src_alpha", .display_name = "Src Alpha", .value = static_cast<i64>(blend_factor::src_alpha)},
			{.name = "one_minus_src_alpha", .display_name = "One Minus Src Alpha", .value = static_cast<i64>(blend_factor::one_minus_src_alpha)},
			{.name = "dst_alpha", .display_name = "Dst Alpha", .value = static_cast<i64>(blend_factor::dst_alpha)},
			{.name = "one_minus_dst_alpha", .display_name = "One Minus Dst Alpha", .value = static_cast<i64>(blend_factor::one_minus_dst_alpha)},
		};

		static const reflected_enum_value_desc_t blend_op_values[] = {
			{.name = "add", .display_name = "Add", .value = static_cast<i64>(blend_op::add)},
			{.name = "subtract", .display_name = "Subtract", .value = static_cast<i64>(blend_op::subtract)},
			{.name = "reverse_subtract", .display_name = "Reverse Subtract", .value = static_cast<i64>(blend_op::reverse_subtract)},
			{.name = "min", .display_name = "Min", .value = static_cast<i64>(blend_op::min)},
			{.name = "max", .display_name = "Max", .value = static_cast<i64>(blend_op::max)},
		};

		static const reflected_enum_value_desc_t stencil_op_values[] = {
			{.name = "keep", .display_name = "Keep", .value = static_cast<i64>(stencil_op::keep)},
			{.name = "zero", .display_name = "Zero", .value = static_cast<i64>(stencil_op::zero)},
			{.name = "replace", .display_name = "Replace", .value = static_cast<i64>(stencil_op::replace)},
			{.name = "increment_clamp", .display_name = "Increment Clamp", .value = static_cast<i64>(stencil_op::increment_clamp)},
			{.name = "decrement_clamp", .display_name = "Decrement Clamp", .value = static_cast<i64>(stencil_op::decrement_clamp)},
			{.name = "invert", .display_name = "Invert", .value = static_cast<i64>(stencil_op::invert)},
			{.name = "increment_wrap", .display_name = "Increment Wrap", .value = static_cast<i64>(stencil_op::increment_wrap)},
			{.name = "decrement_wrap", .display_name = "Decrement Wrap", .value = static_cast<i64>(stencil_op::decrement_wrap)},
		};

		static const reflected_enum_value_desc_t compare_op_values[] = {
			{.name = "never", .display_name = "Never", .value = static_cast<i64>(compare_op::never)},
			{.name = "less", .display_name = "Less", .value = static_cast<i64>(compare_op::less)},
			{.name = "equal", .display_name = "Equal", .value = static_cast<i64>(compare_op::equal)},
			{.name = "lequal", .display_name = "Less Equal", .value = static_cast<i64>(compare_op::lequal)},
			{.name = "greater", .display_name = "Greater", .value = static_cast<i64>(compare_op::greater)},
			{.name = "nequal", .display_name = "Not Equal", .value = static_cast<i64>(compare_op::nequal)},
			{.name = "gequal", .display_name = "Greater Equal", .value = static_cast<i64>(compare_op::gequal)},
			{.name = "always", .display_name = "Always", .value = static_cast<i64>(compare_op::always)},
		};

		static const reflected_enum_value_desc_t store_op_values[] = {
			{.name = "store", .display_name = "Store", .value = static_cast<i64>(store_op::store)},
			{.name = "dont_care", .display_name = "Dont Care", .value = static_cast<i64>(store_op::dont_care)},
			{.name = "none", .display_name = "None", .value = static_cast<i64>(store_op::none)},
		};

		static const reflected_enum_value_desc_t load_op_values[] = {
			{.name = "load", .display_name = "Load", .value = static_cast<i64>(load_op::load)},
			{.name = "clear", .display_name = "Clear", .value = static_cast<i64>(load_op::clear)},
			{.name = "dont_care", .display_name = "Dont Care", .value = static_cast<i64>(load_op::dont_care)},
			{.name = "none", .display_name = "None", .value = static_cast<i64>(load_op::none)},
		};

		bool get_vertex_input_offset(const void* object, const reflected_field_desc_t&, void* out_value, void*)
		{
			*static_cast<u32*>(out_value) = static_cast<u32>(static_cast<const vertex_input_t*>(object)->offset);
			return true;
		}

		bool set_vertex_input_offset(void* object, const reflected_field_desc_t&, const void* value, void*)
		{
			static_cast<vertex_input_t*>(object)->offset = static_cast<size_t>(*static_cast<const u32*>(value));
			return true;
		}

		bool get_vertex_input_size(const void* object, const reflected_field_desc_t&, void* out_value, void*)
		{
			*static_cast<u32*>(out_value) = static_cast<u32>(static_cast<const vertex_input_t*>(object)->size);
			return true;
		}

		bool set_vertex_input_size(void* object, const reflected_field_desc_t&, const void* value, void*)
		{
			static_cast<vertex_input_t*>(object)->size = static_cast<size_t>(*static_cast<const u32*>(value));
			return true;
		}
	}

	vertex_input_reflection_t::vertex_input_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "name", .display_name = "Name", .type = reflected_value_type_e::string, .offset = offsetof(vertex_input_t, name), .size = sizeof(vertex_input_t::name)},
			{.get = get_vertex_input_offset, .set = set_vertex_input_offset, .name = "offset", .display_name = "Offset", .type = reflected_value_type_e::u32, .offset = offsetof(vertex_input_t, offset), .size = sizeof(size_t)},
			{.get = get_vertex_input_size, .set = set_vertex_input_size, .name = "size", .display_name = "Size", .type = reflected_value_type_e::u32, .offset = offsetof(vertex_input_t, size), .size = sizeof(size_t)},
			{.name = "format", .display_name = "Format", .type = reflected_value_type_e::enum8, .value_type_id = format_reflection_t::TYPE_ID, .offset = offsetof(vertex_input_t, format), .size = sizeof(format_e)},
			{.name = "location", .display_name = "Location", .type = reflected_value_type_e::u8, .offset = offsetof(vertex_input_t, location), .size = sizeof(u8)},
			{.name = "index", .display_name = "Index", .type = reflected_value_type_e::u8, .offset = offsetof(vertex_input_t, index), .size = sizeof(u8)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "vertex_input_t",
			.type_id   = TYPE_ID,
			.size	   = sizeof(vertex_input_t),
			.alignment = alignof(vertex_input_t),
		});
	}

	cull_mode_reflection_t::cull_mode_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = cull_mode_values, .size = std::size(cull_mode_values)},
			.name		 = "cull_mode",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(cull_mode),
			.alignment	 = alignof(cull_mode),
		});
	}

	fill_mode_reflection_t::fill_mode_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = fill_mode_values, .size = std::size(fill_mode_values)},
			.name		 = "fill_mode",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(fill_mode),
			.alignment	 = alignof(fill_mode),
		});
	}

	front_face_reflection_t::front_face_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = front_face_values, .size = std::size(front_face_values)},
			.name		 = "front_face",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(front_face),
			.alignment	 = alignof(front_face),
		});
	}

	blend_factor_reflection_t::blend_factor_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = blend_factor_values, .size = std::size(blend_factor_values)},
			.name		 = "blend_factor",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(blend_factor),
			.alignment	 = alignof(blend_factor),
		});
	}

	blend_op_reflection_t::blend_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = blend_op_values, .size = std::size(blend_op_values)},
			.name		 = "blend_op",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(blend_op),
			.alignment	 = alignof(blend_op),
		});
	}

	stencil_op_reflection_t::stencil_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = stencil_op_values, .size = std::size(stencil_op_values)},
			.name		 = "stencil_op",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(stencil_op),
			.alignment	 = alignof(stencil_op),
		});
	}

	compare_op_reflection_t::compare_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = compare_op_values, .size = std::size(compare_op_values)},
			.name		 = "compare_op",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(compare_op),
			.alignment	 = alignof(compare_op),
		});
	}

	store_op_reflection_t::store_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = store_op_values, .size = std::size(store_op_values)},
			.name		 = "store_op",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(store_op),
			.alignment	 = alignof(store_op),
		});
	}

	load_op_reflection_t::load_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = load_op_values, .size = std::size(load_op_values)},
			.name		 = "load_op",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(load_op),
			.alignment	 = alignof(load_op),
		});
	}

	stencil_state_reflection_t::stencil_state_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "fail_op", .display_name = "Fail Op", .type = reflected_value_type_e::enum8, .value_type_id = stencil_op_reflection_t::TYPE_ID, .offset = offsetof(stencil_state_t, fail_op), .size = sizeof(stencil_op)},
			{.name = "pass_op", .display_name = "Pass Op", .type = reflected_value_type_e::enum8, .value_type_id = stencil_op_reflection_t::TYPE_ID, .offset = offsetof(stencil_state_t, pass_op), .size = sizeof(stencil_op)},
			{.name = "depth_fail_op", .display_name = "Depth Fail Op", .type = reflected_value_type_e::enum8, .value_type_id = stencil_op_reflection_t::TYPE_ID, .offset = offsetof(stencil_state_t, depth_fail_op), .size = sizeof(stencil_op)},
			{.name = "compare_op", .display_name = "Compare Op", .type = reflected_value_type_e::enum8, .value_type_id = compare_op_reflection_t::TYPE_ID, .offset = offsetof(stencil_state_t, compare_op), .size = sizeof(compare_op)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "stencil_state_t",
			.type_id   = TYPE_ID,
			.size	   = sizeof(stencil_state_t),
			.alignment = alignof(stencil_state_t),
		});
	}
}
