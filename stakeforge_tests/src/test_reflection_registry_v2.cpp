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

#include "test_registry.hpp"

#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <cstddef>
#include <fstream>

namespace sfg
{
	namespace tests
	{
		enum class reflection_registry_enum8_e : u8
		{
			none,
			small,
			large,
		};

		enum class reflection_registry_enum32_e : u32
		{
			none,
			near,
			far,
		};

		struct reflection_registry_nested_t
		{
			vector_t<i32>					vector_value;
			inplace_vector_t<u8, 2>			inplace_vector2_value;
			string_t						string_value;
			reflection_registry_enum32_e enum32_value = reflection_registry_enum32_e::near;
			i32								i32_value	 = 0;
			f32								f32_value	 = 0.0f;
			u16								u16_value	 = 0;
			reflection_registry_enum8_e	enum8_value	 = reflection_registry_enum8_e::none;
			u8								u8_value	 = 0;
			bool							bool_value	 = false;
		};

		struct reflection_registry_data_t
		{
			reflection_registry_nested_t nested_value;
			vector_t<u32>					vector_value;
			inplace_vector_t<i16, 2>		inplace_vector2_value;
			string_t						string_value;
			reflection_registry_enum32_e enum32_value = reflection_registry_enum32_e::far;
			u64								u64_value	 = 0;
			i64								i64_value	 = 0;
			f32								f32_value	 = 0.0f;
			u32								u32_value	 = 0;
			i32								i32_value	 = 0;
			u16								u16_value	 = 0;
			i16								i16_value	 = 0;
			reflection_registry_enum8_e	enum8_value	 = reflection_registry_enum8_e::small;
			u8								u8_value	 = 0;
			i8								i8_value	 = 0;
			bool							bool_value	 = false;
		};
	}

	SFG_DEFINE_TYPE_ID(tests::reflection_registry_enum8_e);
	SFG_DEFINE_TYPE_ID(tests::reflection_registry_enum32_e);
	SFG_DEFINE_TYPE_ID(tests::reflection_registry_nested_t);
	SFG_DEFINE_TYPE_ID(tests::reflection_registry_data_t);

	namespace tests
	{
		class reflection_registry_test_t final
		{
		public:
			static bool round_trip_all_field_types()
			{
				test_context_t context;
				context.suite	 = "reflection_registry_t";
				context.name	 = "round_trip_all_field_types";
				context.failures = 0;

				register_reflection();

				reflection_registry_data_t source = make_source();

				ostream_t out_stream;
				reflection_registry_t::get().type_to_stream(type_id_t<reflection_registry_data_t>::value, &source, nullptr, out_stream);

				reflection_registry_data_t stream_result;
				istream_t					  in_stream(out_stream.get_raw(), out_stream.get_size());
				reflection_registry_t::get().type_from_stream(type_id_t<reflection_registry_data_t>::value, &stream_result, nullptr, in_stream);
				SFG_TEST_EXPECT(context, equals(source, stream_result));

				nlohmann::json out_json;
				reflection_registry_t::get().type_to_json(type_id_t<reflection_registry_data_t>::value, &source, nullptr, out_json);

				std::ofstream json_file("reflection_registry_test.json");
				SFG_TEST_EXPECT(context, json_file.is_open());
				json_file << out_json.dump(4);
				json_file.close();

				reflection_registry_data_t json_result;
				reflection_registry_t::get().type_from_json(type_id_t<reflection_registry_data_t>::value, &json_result, nullptr, out_json);
				SFG_TEST_EXPECT(context, equals(source, json_result));

				return context.failures == 0;
			}

		private:
			static void register_reflection()
			{
				reflection_registry_t& registry = reflection_registry_t::get();
				if (registry.find_type(type_id_t<reflection_registry_data_t>::value) != nullptr)
					return;

				registry.register_type({
					.name		  = "reflection_registry_enum8_e",
					.display_name = "Reflection Registry V2 Enum8",
					.fields =
						{
							{.name = "none", .display_name = "None"},
							{.name = "small", .display_name = "Small"},
							{.name = "large", .display_name = "Large"},
						},
					.type_id   = type_id_t<reflection_registry_enum8_e>::value,
					.size	   = sizeof(reflection_registry_enum8_e),
					.alignment = alignof(reflection_registry_enum8_e),
					.flags	   = reflected_type_flag_enum,
				});

				registry.register_type({
					.name		  = "reflection_registry_enum32_e",
					.display_name = "Reflection Registry V2 Enum32",
					.fields =
						{
							{.name = "none", .display_name = "None"},
							{.name = "near", .display_name = "Near"},
							{.name = "far", .display_name = "Far"},
						},
					.type_id   = type_id_t<reflection_registry_enum32_e>::value,
					.size	   = sizeof(reflection_registry_enum32_e),
					.alignment = alignof(reflection_registry_enum32_e),
					.flags	   = reflected_type_flag_enum,
				});

				registry.register_type({
					.name		  = "reflection_registry_nested_t",
					.display_name = "Reflection Registry V2 Nested",
					.fields =
						{
							{
								.container_ops = reflection_container_ops_t::vector_ops<i32>(reflected_value_type_e_v2::i32),
								.name		   = "vector_value",
								.display_name  = "Vector Value",
								.offset		   = offsetof(reflection_registry_nested_t, vector_value),
								.size		   = sizeof(reflection_registry_nested_t::vector_value),
								.type		   = reflected_value_type_e_v2::container,
							},
							{
								.container_ops = reflection_container_ops_t::inplace_vector_ops<u8, 2>(reflected_value_type_e_v2::u8),
								.name		   = "inplace_vector2_value",
								.display_name  = "Inplace Vector2 Value",
								.offset		   = offsetof(reflection_registry_nested_t, inplace_vector2_value),
								.size		   = sizeof(reflection_registry_nested_t::inplace_vector2_value),
								.type		   = reflected_value_type_e_v2::container,
							},
							{
								.name		  = "string_value",
								.display_name = "String Value",
								.offset		  = offsetof(reflection_registry_nested_t, string_value),
								.size		  = sizeof(reflection_registry_nested_t::string_value),
								.type		  = reflected_value_type_e_v2::string,
							},
							{
								.name		  = "enum32_value",
								.display_name = "Enum32 Value",
								.sub_type_id  = type_id_t<reflection_registry_enum32_e>::value,
								.offset		  = offsetof(reflection_registry_nested_t, enum32_value),
								.size		  = sizeof(reflection_registry_nested_t::enum32_value),
								.type		  = reflected_value_type_e_v2::object,
							},
							{
								.name		  = "i32_value",
								.display_name = "I32 Value",
								.offset		  = offsetof(reflection_registry_nested_t, i32_value),
								.size		  = sizeof(reflection_registry_nested_t::i32_value),
								.type		  = reflected_value_type_e_v2::i32,
							},
							{
								.name		  = "f32_value",
								.display_name = "F32 Value",
								.offset		  = offsetof(reflection_registry_nested_t, f32_value),
								.size		  = sizeof(reflection_registry_nested_t::f32_value),
								.type		  = reflected_value_type_e_v2::f32,
							},
							{
								.name		  = "u16_value",
								.display_name = "U16 Value",
								.offset		  = offsetof(reflection_registry_nested_t, u16_value),
								.size		  = sizeof(reflection_registry_nested_t::u16_value),
								.type		  = reflected_value_type_e_v2::u16,
							},
							{
								.name		  = "enum8_value",
								.display_name = "Enum8 Value",
								.sub_type_id  = type_id_t<reflection_registry_enum8_e>::value,
								.offset		  = offsetof(reflection_registry_nested_t, enum8_value),
								.size		  = sizeof(reflection_registry_nested_t::enum8_value),
								.type		  = reflected_value_type_e_v2::object,
							},
							{
								.name		  = "u8_value",
								.display_name = "U8 Value",
								.offset		  = offsetof(reflection_registry_nested_t, u8_value),
								.size		  = sizeof(reflection_registry_nested_t::u8_value),
								.type		  = reflected_value_type_e_v2::u8,
							},
							{
								.name		  = "bool_value",
								.display_name = "Bool Value",
								.offset		  = offsetof(reflection_registry_nested_t, bool_value),
								.size		  = sizeof(reflection_registry_nested_t::bool_value),
								.type		  = reflected_value_type_e_v2::boolean,
							},
						},
					.type_id   = type_id_t<reflection_registry_nested_t>::value,
					.size	   = sizeof(reflection_registry_nested_t),
					.alignment = alignof(reflection_registry_nested_t),
				});

				registry.register_type({
					.name		  = "reflection_registry_data_t",
					.display_name = "Reflection Registry V2 Data",
					.fields =
						{
							{
								.name		  = "nested_value",
								.display_name = "Nested Value",
								.sub_type_id  = type_id_t<reflection_registry_nested_t>::value,
								.offset		  = offsetof(reflection_registry_data_t, nested_value),
								.size		  = sizeof(reflection_registry_data_t::nested_value),
								.type		  = reflected_value_type_e_v2::object,
							},
							{
								.container_ops = reflection_container_ops_t::vector_ops<u32>(reflected_value_type_e_v2::u32),
								.name		   = "vector_value",
								.display_name  = "Vector Value",
								.offset		   = offsetof(reflection_registry_data_t, vector_value),
								.size		   = sizeof(reflection_registry_data_t::vector_value),
								.type		   = reflected_value_type_e_v2::container,
							},
							{
								.container_ops = reflection_container_ops_t::inplace_vector_ops<i16, 2>(reflected_value_type_e_v2::i16),
								.name		   = "inplace_vector2_value",
								.display_name  = "Inplace Vector2 Value",
								.offset		   = offsetof(reflection_registry_data_t, inplace_vector2_value),
								.size		   = sizeof(reflection_registry_data_t::inplace_vector2_value),
								.type		   = reflected_value_type_e_v2::container,
							},
							{
								.name		  = "string_value",
								.display_name = "String Value",
								.offset		  = offsetof(reflection_registry_data_t, string_value),
								.size		  = sizeof(reflection_registry_data_t::string_value),
								.type		  = reflected_value_type_e_v2::string,
							},
							{
								.name		  = "enum32_value",
								.display_name = "Enum32 Value",
								.sub_type_id  = type_id_t<reflection_registry_enum32_e>::value,
								.offset		  = offsetof(reflection_registry_data_t, enum32_value),
								.size		  = sizeof(reflection_registry_data_t::enum32_value),
								.type		  = reflected_value_type_e_v2::object,
							},
							{
								.name		  = "u64_value",
								.display_name = "U64 Value",
								.offset		  = offsetof(reflection_registry_data_t, u64_value),
								.size		  = sizeof(reflection_registry_data_t::u64_value),
								.type		  = reflected_value_type_e_v2::u64,
							},
							{
								.name		  = "i64_value",
								.display_name = "I64 Value",
								.offset		  = offsetof(reflection_registry_data_t, i64_value),
								.size		  = sizeof(reflection_registry_data_t::i64_value),
								.type		  = reflected_value_type_e_v2::i64,
							},
							{
								.name		  = "f32_value",
								.display_name = "F32 Value",
								.offset		  = offsetof(reflection_registry_data_t, f32_value),
								.size		  = sizeof(reflection_registry_data_t::f32_value),
								.type		  = reflected_value_type_e_v2::f32,
							},
							{
								.name		  = "u32_value",
								.display_name = "U32 Value",
								.offset		  = offsetof(reflection_registry_data_t, u32_value),
								.size		  = sizeof(reflection_registry_data_t::u32_value),
								.type		  = reflected_value_type_e_v2::u32,
							},
							{
								.name		  = "i32_value",
								.display_name = "I32 Value",
								.offset		  = offsetof(reflection_registry_data_t, i32_value),
								.size		  = sizeof(reflection_registry_data_t::i32_value),
								.type		  = reflected_value_type_e_v2::i32,
							},
							{
								.name		  = "u16_value",
								.display_name = "U16 Value",
								.offset		  = offsetof(reflection_registry_data_t, u16_value),
								.size		  = sizeof(reflection_registry_data_t::u16_value),
								.type		  = reflected_value_type_e_v2::u16,
							},
							{
								.name		  = "i16_value",
								.display_name = "I16 Value",
								.offset		  = offsetof(reflection_registry_data_t, i16_value),
								.size		  = sizeof(reflection_registry_data_t::i16_value),
								.type		  = reflected_value_type_e_v2::i16,
							},
							{
								.name		  = "enum8_value",
								.display_name = "Enum8 Value",
								.sub_type_id  = type_id_t<reflection_registry_enum8_e>::value,
								.offset		  = offsetof(reflection_registry_data_t, enum8_value),
								.size		  = sizeof(reflection_registry_data_t::enum8_value),
								.type		  = reflected_value_type_e_v2::object,
							},
							{
								.name		  = "u8_value",
								.display_name = "U8 Value",
								.offset		  = offsetof(reflection_registry_data_t, u8_value),
								.size		  = sizeof(reflection_registry_data_t::u8_value),
								.type		  = reflected_value_type_e_v2::u8,
							},
							{
								.name		  = "i8_value",
								.display_name = "I8 Value",
								.offset		  = offsetof(reflection_registry_data_t, i8_value),
								.size		  = sizeof(reflection_registry_data_t::i8_value),
								.type		  = reflected_value_type_e_v2::i8,
							},
							{
								.name		  = "bool_value",
								.display_name = "Bool Value",
								.offset		  = offsetof(reflection_registry_data_t, bool_value),
								.size		  = sizeof(reflection_registry_data_t::bool_value),
								.type		  = reflected_value_type_e_v2::boolean,
							},
						},
					.type_id   = type_id_t<reflection_registry_data_t>::value,
					.size	   = sizeof(reflection_registry_data_t),
					.alignment = alignof(reflection_registry_data_t),
				});
			}

			static reflection_registry_data_t make_source()
			{
				reflection_registry_data_t value;
				value.nested_value			= make_nested_source();
				value.vector_value			= {17, 29, 41};
				value.inplace_vector2_value = {static_cast<i16>(-7), static_cast<i16>(31)};
				value.string_value			= "reflection registry v2";
				value.enum32_value			= reflection_registry_enum32_e::far;
				value.u64_value				= 0x1122334455667788ull;
				value.i64_value				= -12345678901234ll;
				value.f32_value				= 12.5f;
				value.u32_value				= 123456789u;
				value.i32_value				= -1234567;
				value.u16_value				= 65000;
				value.i16_value				= -1234;
				value.enum8_value			= reflection_registry_enum8_e::large;
				value.u8_value				= 222;
				value.i8_value				= -88;
				value.bool_value			= true;
				return value;
			}

			static reflection_registry_nested_t make_nested_source()
			{
				reflection_registry_nested_t value;
				value.vector_value			= {-3, 5, 8};
				value.inplace_vector2_value = {static_cast<u8>(9), static_cast<u8>(12)};
				value.string_value			= "nested reflection object";
				value.enum32_value			= reflection_registry_enum32_e::near;
				value.i32_value				= -5150;
				value.f32_value				= -2.25f;
				value.u16_value				= 4096;
				value.enum8_value			= reflection_registry_enum8_e::small;
				value.u8_value				= 77;
				value.bool_value			= true;
				return value;
			}

			static bool equals(const reflection_registry_data_t& a, const reflection_registry_data_t& b)
			{
				return equals(a.nested_value, b.nested_value) && a.vector_value == b.vector_value && equals(a.inplace_vector2_value, b.inplace_vector2_value) && a.string_value == b.string_value && a.enum32_value == b.enum32_value &&
					   a.u64_value == b.u64_value && a.i64_value == b.i64_value && a.f32_value == b.f32_value && a.u32_value == b.u32_value && a.i32_value == b.i32_value && a.u16_value == b.u16_value && a.i16_value == b.i16_value &&
					   a.enum8_value == b.enum8_value && a.u8_value == b.u8_value && a.i8_value == b.i8_value && a.bool_value == b.bool_value;
			}

			static bool equals(const reflection_registry_nested_t& a, const reflection_registry_nested_t& b)
			{
				return a.vector_value == b.vector_value && equals(a.inplace_vector2_value, b.inplace_vector2_value) && a.string_value == b.string_value && a.enum32_value == b.enum32_value && a.i32_value == b.i32_value && a.f32_value == b.f32_value &&
					   a.u16_value == b.u16_value && a.enum8_value == b.enum8_value && a.u8_value == b.u8_value && a.bool_value == b.bool_value;
			}

			template <typename T, int N> static bool equals(const inplace_vector_t<T, N>& a, const inplace_vector_t<T, N>& b)
			{
				if (a.size() != b.size())
					return false;

				for (size_t i = 0; i < a.size(); ++i)
				{
					if (a[i] != b[i])
						return false;
				}

				return true;
			}
		};

		void register_reflection_registry_tests()
		{
			register_test("reflection_registry_t", "round_trip_all_field_types", &reflection_registry_test_t::round_trip_all_field_types);
		}
	}
}
