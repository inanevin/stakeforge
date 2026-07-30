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
#include <sfg/runtime/scripting/script_component_schema.hpp>
#include <sfg/runtime/world/ecs.hpp>

namespace sfg
{
	namespace tests
	{
		namespace
		{
			bool schema_diff_and_reflection_lifetime()
			{
				test_context_t context = {
					.suite	  = "script_component_schema",
					.name	  = "schema_diff_and_reflection_lifetime",
					.failures = 0,
				};

				const char* current_json			   = R"({
					"components": [
						{"name":"Velocity","full_name":"Game.Velocity","id":90001,"size":4,"alignment":4,"fields":[{"name":"Value","id":91001,"value_type":"f32","sub_type_id":0,"offset":0,"size":4,"no_ui":true}]},
						{"name":"Removed","full_name":"Game.Removed","id":90002,"size":4,"alignment":4,"fields":[{"name":"Value","id":91002,"value_type":"u32","sub_type_id":0,"offset":0,"size":4}]}
					],
					"world_scripts": [
						{"name":"MainWorld","full_name":"Game.MainWorld","id":92001}
					]
				})";
				const char* candidate_json			   = R"({
					"components": [
						{"name":"Velocity","full_name":"Game.Velocity","id":90001,"size":8,"alignment":4,"fields":[{"name":"Value","id":91001,"value_type":"f32","sub_type_id":0,"offset":0,"size":4,"no_ui":true},{"name":"Scale","id":91003,"value_type":"f32","sub_type_id":0,"offset":4,"size":4}]},
						{"name":"Added","full_name":"Game.Added","id":90003,"size":4,"alignment":4,"fields":[{"name":"Value","id":91004,"value_type":"i32","sub_type_id":0,"offset":0,"size":4}]}
					],
					"world_scripts": [
						{"name":"MainWorld","full_name":"Game.MainWorld","id":92001}
					]
				})";
				const char* visibility_json			   = R"({
					"components": [
						{"name":"Velocity","full_name":"Game.Velocity","id":90001,"size":4,"alignment":4,"fields":[{"name":"Value","id":91001,"value_type":"f32","sub_type_id":0,"offset":0,"size":4}]},
						{"name":"Removed","full_name":"Game.Removed","id":90002,"size":4,"alignment":4,"fields":[{"name":"Value","id":91002,"value_type":"u32","sub_type_id":0,"offset":0,"size":4}]}
					],
					"world_scripts": [
						{"name":"MainWorld","full_name":"Game.MainWorld","id":92001}
					]
				})";
				const char* world_script_mismatch_json = R"({
					"components": [
						{"name":"Velocity","full_name":"Game.Velocity","id":90001,"size":4,"alignment":4,"fields":[{"name":"Value","id":91001,"value_type":"f32","sub_type_id":0,"offset":0,"size":4,"no_ui":true}]},
						{"name":"Removed","full_name":"Game.Removed","id":90002,"size":4,"alignment":4,"fields":[{"name":"Value","id":91002,"value_type":"u32","sub_type_id":0,"offset":0,"size":4}]}
					],
					"world_scripts": [
						{"name":"ReleaseWorld","full_name":"Game.ReleaseWorld","id":92001}
					]
				})";

				script_component_schema_t current				= {};
				script_component_schema_t equivalent			= {};
				script_component_schema_t candidate				= {};
				script_component_schema_t visibility_candidate	= {};
				script_component_schema_t world_script_mismatch = {};

				SFG_TEST_EXPECT(context, current.parse(current_json));
				SFG_TEST_EXPECT(context, equivalent.parse(current_json));
				SFG_TEST_EXPECT(context, candidate.parse(candidate_json));
				SFG_TEST_EXPECT(context, visibility_candidate.parse(visibility_json));
				SFG_TEST_EXPECT(context, world_script_mismatch.parse(world_script_mismatch_json));

				const script_component_schema_delta_t delta			   = current.compare(candidate);
				const script_component_schema_delta_t visibility_delta = current.compare(visibility_candidate);

				SFG_TEST_EXPECT(context, delta.added.size() == 1 && delta.added[0] == 90003);
				SFG_TEST_EXPECT(context, delta.removed.size() == 1 && delta.removed[0] == 90002);
				SFG_TEST_EXPECT(context, delta.layout_changed.size() == 1 && delta.layout_changed[0] == 90001);
				SFG_TEST_EXPECT(context, delta.reflection_changed.empty());
				SFG_TEST_EXPECT(context, visibility_delta.added.empty());
				SFG_TEST_EXPECT(context, visibility_delta.removed.empty());
				SFG_TEST_EXPECT(context, visibility_delta.layout_changed.empty());
				SFG_TEST_EXPECT(context, visibility_delta.reflection_changed.size() == 1 && visibility_delta.reflection_changed[0] == 90001);
				SFG_TEST_EXPECT(context, current.is_equivalent(equivalent));
				SFG_TEST_EXPECT(context, !current.is_equivalent(candidate));
				SFG_TEST_EXPECT(context, !current.is_equivalent(visibility_candidate));
				SFG_TEST_EXPECT(context, !current.is_equivalent(world_script_mismatch));

				const script_world_script_desc_t* world_script = current.find_world_script(92001);

				SFG_TEST_EXPECT(context, world_script != nullptr);
				SFG_TEST_EXPECT(context, world_script != nullptr && world_script->full_name == "Game.MainWorld");

				reflection_registry_t& registry = reflection_registry_t::get();

				registry.reserve_script_capacity();
				registry.remove_script_types();
				current.register_reflection_types();

				const reflected_type_t*	 velocity_type	= registry.find_type(90001);
				const reflected_field_t* velocity_field = velocity_type == nullptr ? nullptr : registry.get_field(velocity_type->fields.start);
				u32						 value			= 42;

				SFG_TEST_EXPECT(context, velocity_type != nullptr);
				SFG_TEST_EXPECT(context, velocity_type != nullptr && velocity_type->owner == reflection_owner_e::game_scripts);
				SFG_TEST_EXPECT(context, velocity_field != nullptr && velocity_field->field_identifier == 91001);
				SFG_TEST_EXPECT(context, velocity_field != nullptr && velocity_field->flags.is_set(reflected_field_flag_no_ui));

				registry.initialize_type(90001, &value);
				SFG_TEST_EXPECT(context, value == 0);

				registry.register_type({
					.name = "Game.Reference",
					.fields =
						{
							{
								.name		 = "Target",
								.sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID,
								.offset		 = 0,
								.size		 = sizeof(u64),
								.type		 = reflected_value_type_e::u64,
							},
						},
					.type_id   = 90004,
					.size	   = sizeof(u64),
					.alignment = alignof(u64),
					.flags	   = reflected_type_flag_component | reflected_type_flag_script,
					.owner	   = reflection_owner_e::game_scripts,
				});

				u64 reference = 0;
				registry.initialize_type(90004, &reference);
				SFG_TEST_EXPECT(context, reference == NULL_SID);

				registry.remove_script_types();
				SFG_TEST_EXPECT(context, registry.find_type(90001) == nullptr);

				return context.failures == 0;
			}

			bool query_cursor_supports_optional_and_excluded_tables()
			{
				test_context_t context = {
					.suite	  = "script_component_schema",
					.name	  = "query_cursor_supports_optional_and_excluded_tables",
					.failures = 0,
				};

				ecs_component_table_t required_a = {};
				ecs_component_table_t required_b = {};
				ecs_component_table_t optional_c = {};
				ecs_component_table_t excluded_d = {};

				ecs_t::table_init(required_a,
								  {
									  .type_id	 = 92001,
									  .size		 = sizeof(u32),
									  .alignment = alignof(u32),
									  .flags	 = ecs_component_type_flags_none,
								  });
				ecs_t::table_init(required_b,
								  {
									  .type_id	 = 92002,
									  .size		 = sizeof(u32),
									  .alignment = alignof(u32),
									  .flags	 = ecs_component_type_flags_none,
								  });
				ecs_t::table_init(optional_c,
								  {
									  .type_id	 = 92003,
									  .size		 = sizeof(u32),
									  .alignment = alignof(u32),
									  .flags	 = ecs_component_type_flags_none,
								  });
				ecs_t::table_init(excluded_d,
								  {
									  .type_id	 = 92004,
									  .size		 = sizeof(u32),
									  .alignment = alignof(u32),
									  .flags	 = ecs_component_type_flags_none,
								  });

				*static_cast<u32*>(ecs_t::table_add(required_a, 1)) = 11;
				*static_cast<u32*>(ecs_t::table_add(required_b, 1)) = 12;
				*static_cast<u32*>(ecs_t::table_add(optional_c, 1)) = 13;
				*static_cast<u32*>(ecs_t::table_add(required_a, 2)) = 21;
				*static_cast<u32*>(ecs_t::table_add(required_b, 2)) = 22;
				*static_cast<u32*>(ecs_t::table_add(required_a, 3)) = 31;
				*static_cast<u32*>(ecs_t::table_add(required_a, 4)) = 41;
				*static_cast<u32*>(ecs_t::table_add(required_b, 4)) = 42;
				*static_cast<u32*>(ecs_t::table_add(excluded_d, 4)) = 44;

				const ecs_component_table_ref_t table_refs[] = {
					required_a.ref(),
					required_b.ref(),
					optional_c.ref().optional(),
					excluded_d.ref().excluded(),
				};
				ecs_query_cursor_t cursor = {};

				ecs_t::inner_join_init(cursor, {.data = table_refs, .size = std::size(table_refs)});
				SFG_TEST_EXPECT(context, ecs_t::inner_join_next(cursor));
				SFG_TEST_EXPECT(context, cursor.current.id == 1);
				SFG_TEST_EXPECT(context, *static_cast<u32*>(cursor.current.components[0]) == 11);
				SFG_TEST_EXPECT(context, *static_cast<u32*>(cursor.current.components[1]) == 12);
				SFG_TEST_EXPECT(context, *static_cast<u32*>(cursor.current.components[2]) == 13);
				SFG_TEST_EXPECT(context, cursor.current.components[3] == nullptr);
				SFG_TEST_EXPECT(context, cursor.current.component_presence_mask == 0b0111);

				ecs_query_cursor_t moved_cursor = cursor;

				SFG_TEST_EXPECT(context, ecs_t::inner_join_next(moved_cursor));
				SFG_TEST_EXPECT(context, moved_cursor.current.id == 2);
				SFG_TEST_EXPECT(context, *static_cast<u32*>(moved_cursor.current.components[0]) == 21);
				SFG_TEST_EXPECT(context, *static_cast<u32*>(moved_cursor.current.components[1]) == 22);
				SFG_TEST_EXPECT(context, moved_cursor.current.components[2] == nullptr);
				SFG_TEST_EXPECT(context, moved_cursor.current.components[3] == nullptr);
				SFG_TEST_EXPECT(context, moved_cursor.current.component_presence_mask == 0b0011);
				SFG_TEST_EXPECT(context, !ecs_t::inner_join_next(moved_cursor));

				ecs_t::table_uninit(excluded_d);
				ecs_t::table_uninit(optional_c);
				ecs_t::table_uninit(required_b);
				ecs_t::table_uninit(required_a);

				return context.failures == 0;
			}
		}

		void register_script_component_schema_tests()
		{
			register_test("script_component_schema", "schema_diff_and_reflection_lifetime", &schema_diff_and_reflection_lifetime);
			register_test("script_component_schema", "query_cursor_supports_optional_and_excluded_tables", &query_cursor_supports_optional_and_excluded_tables);
		}
	}
}
