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

#include "physics_world.hpp"
#include "physics_runtime.hpp"
#include "physics_world_util.hpp"

#include <sfg/data/hash_map.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/physical_material.hpp>
#include <sfg/runtime/resources/ragdoll.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_animation_controller.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>
#include <tracy/Tracy.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>

namespace sfg
{
	namespace
	{
#define PHYSICS_BROAD_PHASE_STATIC 0
#define PHYSICS_BROAD_PHASE_MOVING 1

		struct physics_broad_phase_interface_t final : JPH::BroadPhaseLayerInterface
		{
			JPH::uint GetNumBroadPhaseLayers() const override
			{
				return 2;
			}

			JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
			{
				return JPH::BroadPhaseLayer(physics_world_util_t::is_moving_layer(layer) ? PHYSICS_BROAD_PHASE_MOVING : PHYSICS_BROAD_PHASE_STATIC);
			}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
			const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
			{
				return layer.GetValue() == PHYSICS_BROAD_PHASE_STATIC ? "static" : "moving";
			}
#endif
		};

		struct physics_object_broad_phase_filter_t final : JPH::ObjectVsBroadPhaseLayerFilter
		{
			bool ShouldCollide(JPH::ObjectLayer object_layer, JPH::BroadPhaseLayer broad_phase_layer) const override
			{
				if (!physics_world_util_t::is_moving_layer(object_layer))
					return broad_phase_layer.GetValue() == PHYSICS_BROAD_PHASE_MOVING;

				return true;
			}
		};

		struct physics_layer_pair_filter_t final : JPH::ObjectLayerPairFilter
		{
			const physics_runtime_config_t* config = nullptr;

			bool ShouldCollide(JPH::ObjectLayer layer_a, JPH::ObjectLayer layer_b) const override
			{
				if (!physics_world_util_t::is_moving_layer(layer_a) && !physics_world_util_t::is_moving_layer(layer_b))
					return false;

				const u8 project_a = physics_world_util_t::get_project_layer(layer_a);
				const u8 project_b = physics_world_util_t::get_project_layer(layer_b);
				return (config->collision_masks[project_a] & (1ull << project_b)) != 0 && (config->collision_masks[project_b] & (1ull << project_a)) != 0;
			}
		};

		struct query_object_layer_filter_t final : JPH::ObjectLayerFilter
		{
			u64 layers = UINT64_MAX;

			bool ShouldCollide(JPH::ObjectLayer layer) const override
			{
				return (layers & (1ull << physics_world_util_t::get_project_layer(layer))) != 0;
			}
		};

		struct query_broad_phase_filter_t final : JPH::BroadPhaseLayerFilter
		{
			u8 flags = physics_query_flag_all;

			bool ShouldCollide(JPH::BroadPhaseLayer layer) const override
			{
				if (layer.GetValue() == PHYSICS_BROAD_PHASE_STATIC)
					return (flags & physics_query_flag_static) != 0;

				return (flags & (physics_query_flag_kinematic | physics_query_flag_dynamic | physics_query_flag_character)) != 0;
			}
		};
	}

	class physics_world_t::impl_t final
	{
	public:
		struct body_lookup_t
		{
			entity_id_t entity		 = NULL_ENTITY_ID;
			u32			body_id		 = UINT32_MAX;
			u8			ragdoll_part = UINT8_MAX;
		};

		struct raw_contact_event_t
		{
			JPH::RVec3			   position	   = JPH::RVec3::sZero();
			JPH::Vec3			   normal	   = JPH::Vec3::sZero();
			JPH::SubShapeIDPair	   pair		   = {};
			f32					   penetration = 0.0f;
			physics_contact_type_e type		   = physics_contact_type_e::begin;
			bool				   is_sensor   = false;
		};

		class contact_listener_t final : public JPH::ContactListener
		{
		public:
			impl_t* impl = nullptr;

			void OnContactAdded(const JPH::Body& body_a, const JPH::Body& body_b, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override
			{
				push(body_a, body_b, manifold, settings, physics_contact_type_e::begin);
			}

			void OnContactPersisted(const JPH::Body& body_a, const JPH::Body& body_b, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override
			{
				push(body_a, body_b, manifold, settings, physics_contact_type_e::persist);
			}

			void OnContactRemoved(const JPH::SubShapeIDPair& pair) override
			{
				raw_contact_event_t event{
					.pair = pair,
					.type = physics_contact_type_e::end,
				};

				impl->_raw_contact_events.enqueue(event);
			}

		private:
			void push(const JPH::Body& body_a, const JPH::Body& body_b, const JPH::ContactManifold& manifold, const JPH::ContactSettings& settings, physics_contact_type_e type)
			{
				const JPH::RVec3	position = manifold.mRelativeContactPointsOn1.empty() ? manifold.mBaseOffset : manifold.GetWorldSpaceContactPointOn1(0);
				raw_contact_event_t event{
					.position	 = position,
					.normal		 = manifold.mWorldSpaceNormal,
					.pair		 = JPH::SubShapeIDPair(body_a.GetID(), manifold.mSubShapeID1, body_b.GetID(), manifold.mSubShapeID2),
					.penetration = manifold.mPenetrationDepth,
					.type		 = type,
					.is_sensor	 = settings.mIsSensor,
				};

				impl->_raw_contact_events.enqueue(event);
			}
		};

		struct query_body_filter_t final : JPH::BodyFilter
		{
			const impl_t*				  impl	 = nullptr;
			const physics_query_filter_t* filter = nullptr;

			bool ShouldCollide(const JPH::BodyID& body_id) const override
			{
				return impl->passes_filter(body_id, *filter);
			}
		};

		struct ray_all_collector_t final : JPH::CastRayCollector
		{
			impl_t*					 impl	  = nullptr;
			physics_hit_t*			 hits	  = nullptr;
			const physics_raycast_t* ray	  = nullptr;
			u32						 capacity = 0;
			u32						 count	  = 0;
			bool					 overflow = false;

			void AddHit(const JPH::RayCastResult& result) override
			{
				if (count >= capacity)
				{
					overflow = true;
					return;
				}

				impl->fill_hit(result, *ray, hits[count]);
				count++;
			}
		};

		struct sphere_all_collector_t final : JPH::CastShapeCollector
		{
			impl_t*						impl	 = nullptr;
			physics_hit_t*				hits	 = nullptr;
			const physics_spherecast_t* sphere	 = nullptr;
			u32							capacity = 0;
			u32							count	 = 0;
			bool						overflow = false;

			void AddHit(const JPH::ShapeCastResult& result) override
			{
				if (count >= capacity)
				{
					overflow = true;
					return;
				}

				impl->fill_hit(result, *sphere, hits[count]);
				count++;
			}
		};

		world_t*										 _world			 = nullptr;
		JPH::PhysicsSystem*								 _system		 = nullptr;
		JPH::TempAllocatorImpl*							 _temp_allocator = nullptr;
		physics_runtime_config_t						 _config		 = {};
		physics_broad_phase_interface_t					 _broad_phase_interface;
		physics_object_broad_phase_filter_t				 _object_broad_phase_filter;
		physics_layer_pair_filter_t						 _layer_pair_filter;
		contact_listener_t								 _contact_listener;
		moodycamel::ConcurrentQueue<raw_contact_event_t> _raw_contact_events;
		vector_t<body_lookup_t>							 _body_lookup;
		vector_t<physics_contact_event_t>				 _contact_events;
		hash_map_t<JPH::SubShapeIDPair, bool>			 _contact_sensors;
		vector_t<entity_id_t>							 _sync_entities;
		chunk_allocator32_t								 _ragdoll_pose_memory;
		resource_reload_listener_handle_t				 _resource_reload_listener = {};
		f32												 _accumulator			   = 0.0f;
		u32												 _next_ragdoll_group_id	   = 1;

		impl_t() : _raw_contact_events(4096)
		{
		}

		void init(world_t& world, const physics_runtime_config_t& config)
		{
			_world					  = &world;
			_config					  = config;
			_layer_pair_filter.config = &_config;

			_temp_allocator = new JPH::TempAllocatorImpl(_config.temp_allocator_bytes);
			_system			= new JPH::PhysicsSystem();
			_system->Init(_config.max_bodies, _config.body_mutex_count, _config.max_body_pairs, _config.max_contact_constraints, _broad_phase_interface, _object_broad_phase_filter, _layer_pair_filter);
			_system->SetGravity(physics_world_util_t::to_jolt(_config.gravity));

			_contact_listener.impl = this;
			_system->SetContactListener(&_contact_listener);

			_body_lookup.resize(_config.max_bodies);
			_contact_events.reserve(_config.contact_event_reserve);
			_contact_sensors.reserve(_config.contact_event_reserve);
			_sync_entities.reserve(_config.body_reserve + _config.character_reserve);
			_ragdoll_pose_memory.init(_config.ragdoll_pose_budget_bytes);
			_resource_reload_listener = resource_manager_t::get().add_reload_listener(on_resource_reload, this);
		}

		void uninit()
		{
			resource_manager_t::get().remove_reload_listener(_resource_reload_listener);
			_resource_reload_listener = {};

			clear();

			delete _system;
			delete _temp_allocator;

			_system			= nullptr;
			_temp_allocator = nullptr;
			_world			= nullptr;

			_body_lookup.resize(0);
			_contact_events.resize(0);
			_contact_sensors.clear();
			_sync_entities.resize(0);
			_ragdoll_pose_memory.uninit();

			_accumulator		   = 0.0f;
			_next_ragdoll_group_id = 1;
		}

		static void on_resource_reload(resource_manager_t& resource_manager, sid_t resource_id, resource_type_e resource_type, void* user_data)
		{
			if (resource_type != resource_type_e::ragdoll && resource_type != resource_type_e::skeleton && resource_type != resource_type_e::physical_material)
				return;

			impl_t&						 impl				  = *static_cast<impl_t*>(user_data);
			const ecs_component_table_t& system_ragdoll_table = impl._world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
			impl._sync_entities.resize(0);

			const ecs_component_table_ref_t refs[] = {system_ragdoll_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_ragdoll_t& system_ragdoll = ecs_helpers_t::row_get<component_system_ragdoll_t>(row, 0);
				bool							  recreate		 = resource_type == resource_type_e::ragdoll && system_ragdoll.ragdoll_resource == resource_id;
				recreate |= resource_type == resource_type_e::skeleton && system_ragdoll.skeleton == resource_id;

				if (resource_type == resource_type_e::physical_material)
				{
					const ragdoll_runtime_t* ragdoll_resource = resource_manager.find_runtime<ragdoll_runtime_t>(system_ragdoll.ragdoll_resource);
					recreate |= ragdoll_resource != nullptr && ragdoll_resource->physical_material == resource_id;
				}

				if (recreate)
					impl._sync_entities.push_back(row.id);
			}

			for (entity_id_t entity : impl._sync_entities)
				impl.destroy_ragdoll(entity);

			impl._sync_entities.resize(0);
		}

		void clear()
		{
			ecs_component_table_t& system_constraints_table = _world->get_component_table(type_id_t<component_system_constraints_t>::value);
			ecs_component_table_t& system_physics_table		= _world->get_component_table(type_id_t<component_system_physics_t>::value);
			ecs_component_table_t& system_ragdoll_table		= _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);

			_sync_entities.resize(0);
			const ecs_component_table_ref_t constraint_refs[] = {system_constraints_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = constraint_refs, .size = std::size(constraint_refs)}))
				_sync_entities.push_back(row.id);

			for (entity_id_t entity : _sync_entities)
				destroy_constraint(entity);

			_sync_entities.resize(0);

			const ecs_component_table_ref_t ragdoll_refs[] = {system_ragdoll_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = ragdoll_refs, .size = std::size(ragdoll_refs)}))
				_sync_entities.push_back(row.id);

			for (entity_id_t entity : _sync_entities)
				destroy_ragdoll(entity, false);

			const ecs_component_table_ref_t physics_refs[] = {system_physics_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = physics_refs, .size = std::size(physics_refs)}))
			{
				component_system_physics_t& system_physics = ecs_helpers_t::row_get_mutable<component_system_physics_t>(row, 0);
				const entity_id_t			entity		   = row.id;

				if (system_physics.character != 0)
					destroy_character(entity, system_physics);
				else if (system_physics.body_id != UINT32_MAX)
					destroy_body(entity, system_physics, false);
			}

			raw_contact_event_t raw_contact = {};

			while (_raw_contact_events.try_dequeue(raw_contact))
			{
			}

			ecs_t::table_clear(system_physics_table);
			ecs_t::table_clear(system_ragdoll_table);
			_contact_events.resize(0);
			_contact_sensors.clear();
			_sync_entities.resize(0);
			std::fill(_body_lookup.begin(), _body_lookup.end(), body_lookup_t{});
			_ragdoll_pose_memory.reset();
			_accumulator		   = 0.0f;
			_next_ragdoll_group_id = 1;
		}

		entity_id_t resolve_body_entity(const JPH::BodyID& body_id) const
		{
			const body_lookup_t& lookup = _body_lookup[body_id.GetIndex()];

			if (lookup.body_id != body_id.GetIndexAndSequenceNumber())
				return NULL_ENTITY_ID;

			return lookup.entity;
		}

		bool passes_filter(const JPH::BodyID& body_id, const physics_query_filter_t& filter) const
		{
			const entity_id_t entity = resolve_body_entity(body_id);

			if (entity == NULL_ENTITY_ID || entity == filter.ignored_entity)
				return false;

			const ecs_component_table_t&	  system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

			if (system_physics == nullptr)
			{
				if ((filter.flags & physics_query_flag_dynamic) == 0)
					return false;
			}
			else if (system_physics->character != 0)
			{
				if ((filter.flags & physics_query_flag_character) == 0)
					return false;
			}
			else
			{
				const physics_motion_type_e motion_type = static_cast<physics_motion_type_e>(system_physics->motion_type);

				if (motion_type == physics_motion_type_e::static_body && (filter.flags & physics_query_flag_static) == 0)
					return false;

				if (motion_type == physics_motion_type_e::kinematic_body && (filter.flags & physics_query_flag_kinematic) == 0)
					return false;

				if (motion_type == physics_motion_type_e::dynamic_body && (filter.flags & physics_query_flag_dynamic) == 0)
					return false;
			}

			if (_system->GetBodyInterface().IsSensor(body_id) && (filter.flags & physics_query_flag_sensor) == 0)
				return false;

			const ecs_component_table_t&   tags_table  = _world->get_component_table(type_id_t<component_entity_tags_t>::value);
			const component_entity_tags_t* entity_tags = ecs_helpers_t::table_find_as_const<component_entity_tags_t>(tags_table, entity);
			const u64					   tags		   = entity_tags != nullptr ? entity_tags->tags : 0;

			if (filter.required_any_tags.bits != 0 && (tags & filter.required_any_tags.bits) == 0)
				return false;

			if ((tags & filter.required_all_tags.bits) != filter.required_all_tags.bits)
				return false;

			if ((tags & filter.excluded_tags.bits) != 0)
				return false;

			return true;
		}

		void tick(f32 delta_time)
		{
			ZoneScoped;

			_contact_events.resize(0);
			sync_ragdoll_create_destroy();
			sync_body_create_destroy();
			sync_constraint_create_destroy();
			sync_constraint_properties();
			sync_static_and_kinematic_bodies_to_physics();

			const f32 fixed_delta = 1.0f / static_cast<f32>(_config.physics_rate);
			u32		  steps		  = 0;

			_accumulator += delta_time;

			while (_accumulator >= fixed_delta && steps < _config.max_sub_steps)
			{
				update_characters(fixed_delta);
				_system->Update(fixed_delta, 1, _temp_allocator, &physics_runtime_t::get_job_system());

				drain_contact_events();
				_accumulator -= fixed_delta;
				steps++;
			}

			if (steps != 0)
			{
				sync_dynamic_bodies_into_world();
				sync_ragdolls_into_world();
			}

			if (steps == _config.max_sub_steps && _accumulator >= fixed_delta)
				_accumulator = 0.0f;
		}

		void sync_static_and_kinematic_bodies_to_physics()
		{
			ZoneScoped;

			const ecs_component_table_t& system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const ecs_component_table_t& transform_table	  = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			const ecs_component_table_t& disabled_table		  = _world->get_component_table(type_id_t<component_disabled_t>::value);
			JPH::BodyInterface&			 body_interface		  = _system->GetBodyInterface();

			const ecs_component_table_ref_t refs[] = {system_physics_table.ref(), transform_table.ref(), !disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_physics_t& phy = ecs_helpers_t::row_get<component_system_physics_t>(row, 0);

				if (phy.motion_type == static_cast<u8>(physics_motion_type_e::dynamic_body))
					continue;

				const component_system_transform_t& transform	  = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				quat_t								body_rotation = transform.abs_rot * phy.local_rotation;
				vec3f_t								body_position = transform.abs_pos + transform.abs_rot * (phy.local_position * transform.abs_scale);

				body_interface.SetPositionAndRotation(JPH::BodyID(phy.body_id), physics_world_util_t::to_jolt_position(body_position), physics_world_util_t::to_jolt(body_rotation), JPH::EActivation::Activate);
			}
		}

		bool create_ragdoll(entity_id_t entity, const component_ragdoll_t& component)
		{
			resource_manager_t&		 resource_manager = resource_manager_t::get();
			const ragdoll_runtime_t* resource		  = resource_manager.find_runtime<ragdoll_runtime_t>(component.ragdoll);

			if (resource == nullptr)
				return false;

			const ecs_component_table_t&			 skinned_table = _world->get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
			const component_skinned_mesh_renderer_t& skinned	   = ecs_helpers_t::table_get_as_const<component_skinned_mesh_renderer_t>(skinned_table, entity);

			if (skinned.skeleton != resource->target_skeleton)
			{
				SFG_WARN("ragdoll skeleton does not match skinned renderer for entity {0}", entity);
				return false;
			}

			const skeleton_runtime_t* skeleton = resource_manager.find_runtime<skeleton_runtime_t>(resource->target_skeleton);

			if (skeleton == nullptr || resource->part_count == 0)
				return false;

			_world->calculate_transform_direct(entity);

			chunk_allocator32_t&							resource_memory		   = resource_manager.get_memory();
			const ragdoll_part_runtime_t*					resource_parts		   = resource_memory.get<ragdoll_part_runtime_t>(resource->parts);
			const skeleton_joint_runtime_t*					joints				   = resource_memory.get<skeleton_joint_runtime_t>(skeleton->joints);
			const ecs_component_table_t&					system_transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			const component_system_transform_t&				transform			   = ecs_helpers_t::table_get_as_const<component_system_transform_t>(system_transform_table, entity);
			const ecs_component_table_t&					system_skinned_table   = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
			const component_system_skinned_mesh_renderer_t& system_skinned		   = ecs_helpers_t::table_get_as_const<component_system_skinned_mesh_renderer_t>(system_skinned_table, entity);
			const span_t<const animation_bone_t>			current_bones		   = _world->get_animation_controller().get_bones(system_skinned.bones_handle);

			mat4x3_t*			   frozen_local_pose		= nullptr;
			const chunk_handle32_t frozen_local_pose_handle = _ragdoll_pose_memory.allocate<mat4x3_t>(skeleton->joint_count, frozen_local_pose);
			mat4x3_t*			   joint_global_pose		= nullptr;
			const chunk_handle32_t joint_global_pose_handle = _ragdoll_pose_memory.allocate<mat4x3_t>(skeleton->joint_count, joint_global_pose);

			for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
				joint_global_pose[joint_index] = system_skinned.final_bones_calculated ? current_bones[joint_index].bone_transform * joints[joint_index].bind_global : joints[joint_index].bind_global;

			for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
			{
				const u32 parent_index		   = joints[joint_index].parent_index;
				frozen_local_pose[joint_index] = parent_index == SKELETON_JOINT_NO_PARENT ? joint_global_pose[joint_index] : joint_global_pose[parent_index].inverse() * joint_global_pose[joint_index];
			}

			JPH::Ref<JPH::RagdollSettings> settings = new JPH::RagdollSettings();
			settings->mSkeleton						= new JPH::Skeleton();
			settings->mParts.resize(resource->part_count);

			JPH::Mat44 initial_pose[RAGDOLL_PART_MAX] = {};

			for (u32 part_index = 0; part_index < resource->part_count; ++part_index)
			{
				const ragdoll_part_runtime_t& source_part = resource_parts[part_index];
				SFG_ASSERT(source_part.joint_index < skeleton->joint_count);
				SFG_ASSERT(source_part.parent_part_index == RAGDOLL_PART_NO_PARENT || source_part.parent_part_index < part_index);

				const char* joint_name = resource_memory.get_text(joints[source_part.joint_index].name);
				settings->mSkeleton->AddJoint(joint_name, source_part.parent_part_index == RAGDOLL_PART_NO_PARENT ? -1 : static_cast<i32>(source_part.parent_part_index));

				vec3f_t		   joint_position = vec3f_t::zero;
				quat_t		   joint_rotation = quat_t::identity;
				vec3f_t		   joint_scale	  = vec3f_t::one;
				const mat4x3_t joint_world	  = transform.abs_mat * joint_global_pose[source_part.joint_index];
				joint_world.decompose(joint_position, joint_rotation, joint_scale);
				const f32 shape_scale = math::max(math::max(math::abs(joint_scale.x), math::abs(joint_scale.y)), math::abs(joint_scale.z));

				JPH::RagdollSettings::Part&					  part	  = settings->mParts[part_index];
				JPH::Ref<JPH::CapsuleShapeSettings>			  capsule = new JPH::CapsuleShapeSettings(math::max(source_part.half_height * shape_scale, 0.001f), math::max(source_part.radius * shape_scale, 0.001f));
				JPH::Ref<JPH::RotatedTranslatedShapeSettings> shape = new JPH::RotatedTranslatedShapeSettings(physics_world_util_t::to_jolt(source_part.local_position * joint_scale), physics_world_util_t::to_jolt(source_part.local_rotation), capsule);
				const JPH::ShapeSettings::ShapeResult		  shape_result = shape->Create();

				if (shape_result.HasError())
				{
					_ragdoll_pose_memory.free(joint_global_pose_handle);
					_ragdoll_pose_memory.free(frozen_local_pose_handle);
					SFG_WARN("failed to create ragdoll capsule for entity {0}: {1}", entity, shape_result.GetError().c_str());
					return false;
				}

				part.SetShape(shape_result.Get());
				part.mPosition					   = physics_world_util_t::to_jolt_position(joint_position);
				part.mRotation					   = physics_world_util_t::to_jolt(joint_rotation);
				part.mMotionType				   = JPH::EMotionType::Dynamic;
				part.mObjectLayer				   = physics_world_util_t::make_object_layer(component.collision_layer, physics_motion_type_e::dynamic_body);
				part.mUserData					   = entity;
				part.mLinearDamping				   = resource->linear_damping;
				part.mAngularDamping			   = resource->angular_damping;
				part.mGravityFactor				   = resource->gravity_factor;
				part.mAllowSleeping				   = resource->allow_sleep != 0;
				part.mOverrideMassProperties	   = JPH::EOverrideMassProperties::CalculateInertia;
				part.mMassPropertiesOverride.mMass = source_part.mass;

				if (resource->physical_material != NULL_RESOURCE_HANDLE)
				{
					const physical_material_runtime_t* material = resource_manager.find_runtime<physical_material_runtime_t>(resource->physical_material);

					if (material != nullptr)
					{
						part.mRestitution = material->restitution;
						part.mFriction	  = material->friction;
					}
				}

				initial_pose[part_index] = JPH::Mat44::sRotationTranslation(physics_world_util_t::to_jolt(joint_rotation), physics_world_util_t::to_jolt(joint_position));

				if (source_part.parent_part_index == RAGDOLL_PART_NO_PARENT)
					continue;

				JPH::SwingTwistConstraintSettings* constraint = new JPH::SwingTwistConstraintSettings();
				constraint->mSpace							  = JPH::EConstraintSpace::WorldSpace;
				constraint->mPosition1						  = physics_world_util_t::to_jolt_position(joint_position);
				constraint->mPosition2						  = physics_world_util_t::to_jolt_position(joint_position);
				constraint->mTwistAxis1						  = physics_world_util_t::to_jolt(joint_rotation * source_part.twist_axis.normalized());
				constraint->mTwistAxis2						  = constraint->mTwistAxis1;
				constraint->mPlaneAxis1						  = physics_world_util_t::to_jolt(joint_rotation * source_part.plane_axis.normalized());
				constraint->mPlaneAxis2						  = constraint->mPlaneAxis1;
				constraint->mNormalHalfConeAngle			  = math::degrees_to_radians(source_part.normal_half_cone_angle_degrees);
				constraint->mPlaneHalfConeAngle				  = math::degrees_to_radians(source_part.plane_half_cone_angle_degrees);
				constraint->mTwistMinAngle					  = math::degrees_to_radians(source_part.twist_min_angle_degrees);
				constraint->mTwistMaxAngle					  = math::degrees_to_radians(source_part.twist_max_angle_degrees);
				constraint->mMaxFrictionTorque				  = source_part.max_friction_torque;
				part.mToParent								  = constraint;
			}

			settings->Stabilize();
			settings->DisableParentChildCollisions(initial_pose);
			settings->CalculateBodyIndexToConstraintIndex();

			JPH::Ragdoll* ragdoll = settings->CreateRagdoll(_next_ragdoll_group_id++, entity, _system);

			if (ragdoll == nullptr)
			{
				_ragdoll_pose_memory.free(joint_global_pose_handle);
				_ragdoll_pose_memory.free(frozen_local_pose_handle);
				SFG_WARN("failed to allocate ragdoll bodies for entity {0}", entity);
				return false;
			}

			ragdoll->AddRef();
			ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

			for (u32 part_index = 0; part_index < resource->part_count; ++part_index)
			{
				const JPH::BodyID body_id		 = ragdoll->GetBodyID(static_cast<i32>(part_index));
				_body_lookup[body_id.GetIndex()] = {
					.entity		  = entity,
					.body_id	  = body_id.GetIndexAndSequenceNumber(),
					.ragdoll_part = static_cast<u8>(part_index),
				};
			}

			vec3f_t root_position = vec3f_t::zero;
			quat_t	root_rotation = quat_t::identity;
			vec3f_t root_scale	  = vec3f_t::one;
			joint_global_pose[resource_parts[0].joint_index].decompose(root_position, root_rotation, root_scale);

			ecs_component_table_t&		system_ragdoll_table = _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
			component_system_ragdoll_t& system_ragdoll		 = ecs_helpers_t::table_add_or_get_as<component_system_ragdoll_t>(system_ragdoll_table, entity);
			const JPH::AABox			initial_bounds		 = ragdoll->GetWorldSpaceBounds();
			system_ragdoll									 = {
				.ragdoll		  = ragdoll,
				.entity_from_root = mat4x3_t::transform(root_position, root_rotation, vec3f_t::one).inverse(),
				.world_bounds =
					{
						{initial_bounds.mMin.GetX(), initial_bounds.mMin.GetY(), initial_bounds.mMin.GetZ()},
						{initial_bounds.mMax.GetX(), initial_bounds.mMax.GetY(), initial_bounds.mMax.GetZ()},
					},
				.frozen_local_pose = frozen_local_pose_handle,
				.joint_global_pose = joint_global_pose_handle,
				.ragdoll_resource  = component.ragdoll,
				.skeleton		   = resource->target_skeleton,
				.joint_count	   = skeleton->joint_count,
				.collision_layer   = component.collision_layer,
			};

			return true;
		}

		void destroy_ragdoll(entity_id_t entity, bool reset_animation = true)
		{
			ecs_component_table_t&		system_ragdoll_table = _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
			component_system_ragdoll_t& system_ragdoll		 = ecs_helpers_t::table_get_as<component_system_ragdoll_t>(system_ragdoll_table, entity);

			const u32 body_count = static_cast<u32>(system_ragdoll.ragdoll->GetBodyCount());

			for (u32 part_index = 0; part_index < body_count; ++part_index)
			{
				const JPH::BodyID body_id		 = system_ragdoll.ragdoll->GetBodyID(static_cast<i32>(part_index));
				_body_lookup[body_id.GetIndex()] = {};
			}

			system_ragdoll.ragdoll->RemoveFromPhysicsSystem();
			system_ragdoll.ragdoll->Release();
			_ragdoll_pose_memory.free(system_ragdoll.joint_global_pose);
			_ragdoll_pose_memory.free(system_ragdoll.frozen_local_pose);
			ecs_t::table_remove(system_ragdoll_table, entity);

			if (reset_animation)
				_world->get_animation_controller().reset_pose_after_ragdoll(entity);
		}

		void sync_ragdoll_create_destroy()
		{
			const ecs_component_table_t& ragdoll_table		  = _world->get_component_table(type_id_t<component_ragdoll_t>::value);
			const ecs_component_table_t& skinned_table		  = _world->get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
			const ecs_component_table_t& system_skinned_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
			const ecs_component_table_t& disabled_table		  = _world->get_component_table(type_id_t<component_disabled_t>::value);
			const ecs_component_table_t& system_ragdoll_table = _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
			const ecs_component_table_t& system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			frame_vector_t<entity_id_t>	 entities			  = {};

			{
				const ecs_component_table_ref_t refs[] = {system_ragdoll_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					const component_system_ragdoll_t&		 system_ragdoll = ecs_helpers_t::row_get<component_system_ragdoll_t>(row, 0);
					const component_ragdoll_t*				 component		= ecs_helpers_t::table_find_as_const<component_ragdoll_t>(ragdoll_table, row.id);
					const component_skinned_mesh_renderer_t* skinned		= ecs_helpers_t::table_find_as_const<component_skinned_mesh_renderer_t>(skinned_table, row.id);

					if (component == nullptr || skinned == nullptr || ecs_t::table_has(disabled_table, row.id) || component->ragdoll != system_ragdoll.ragdoll_resource || component->collision_layer != system_ragdoll.collision_layer ||
						skinned->skeleton != system_ragdoll.skeleton)
						entities.push_back(row.id);
				}

				for (entity_id_t entity : entities)
					destroy_ragdoll(entity);
			}

			entities.resize(0);

			{
				const ecs_component_table_ref_t refs[] = {ragdoll_table.ref(), skinned_table.ref(), system_skinned_table.ref(), !disabled_table.ref(), !system_ragdoll_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					entities.push_back(row.id);

				for (entity_id_t entity : entities)
				{
					if (ecs_t::table_has(system_physics_table, entity))
						destroy_entity_physics(entity);

					const component_ragdoll_t& component = ecs_helpers_t::table_get_as_const<component_ragdoll_t>(ragdoll_table, entity);
					create_ragdoll(entity, component);
				}
			}
		}

		bool create_body(entity_id_t entity, component_system_physics_t& system_physics, const component_physical_t& physical)
		{
			const physics_motion_type_e motion_type = physical.motion_type;
			system_physics.motion_type				= static_cast<u8>(motion_type);
			system_physics.local_position			= physical.local_position;
			system_physics.local_rotation			= physical.local_rotation;
			system_physics.physical_material		= physical.physical_material;

			_world->calculate_transform_direct(entity);

			const ecs_component_table_t&		transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			const component_system_transform_t& transform		= ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);

			JPH::RefConst<JPH::Shape> shape = physics_world_util_t::create_shape(*_world, entity, physical, transform.abs_scale);
			if (shape == nullptr)
				return false;

			const quat_t			  body_rotation = transform.abs_rot * physical.local_rotation;
			const vec3f_t			  body_position = transform.abs_pos + transform.abs_rot * (physical.local_position * transform.abs_scale);
			JPH::BodyCreationSettings settings(
				shape, physics_world_util_t::to_jolt_position(body_position), physics_world_util_t::to_jolt(body_rotation), physics_world_util_t::to_jolt(motion_type), physics_world_util_t::make_object_layer(physical.collision_layer, motion_type));

			settings.mIsSensor = physical.is_sensor != 0;
			settings.mUserData = entity;
			settings.mCollideKinematicVsNonDynamic =
				_config.kinematic_sensors_collide_with_non_dynamic &&
				motion_type == physics_motion_type_e::kinematic_body &&
				physical.is_sensor != 0;

			if (motion_type != physics_motion_type_e::static_body)
			{
				settings.mLinearDamping	 = physical.linear_damping;
				settings.mAngularDamping = physical.angular_damping;
				settings.mGravityFactor	 = physical.gravity_factor;
				settings.mAllowSleeping	 = physical.allow_sleep != 0;
				settings.mMotionQuality	 = physical.motion_quality_continuous != 0 ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;

				if (motion_type == physics_motion_type_e::dynamic_body)
				{
					settings.mOverrideMassProperties	   = JPH::EOverrideMassProperties::CalculateInertia;
					settings.mMassPropertiesOverride.mMass = physical.mass;
				}
			}

			if (physical.physical_material != NULL_RESOURCE_HANDLE)
			{
				const physical_material_runtime_t* material = resource_manager_t::get().find_runtime<physical_material_runtime_t>(physical.physical_material);

				if (material != nullptr)
				{
					settings.mRestitution = material->restitution;
					settings.mFriction	  = material->friction;
				}
			}

			JPH::BodyInterface& body_interface = _system->GetBodyInterface();
			JPH::Body*			body		   = body_interface.CreateBody(settings);

			if (body == nullptr)
			{
				SFG_WARN("failed to allocate physics body for entity {0}", entity);
				return false;
			}

			const JPH::BodyID body_id		 = body->GetID();
			system_physics.body_id			 = body_id.GetIndexAndSequenceNumber();
			system_physics.single_sub_entity = entity;

			_body_lookup[body_id.GetIndex()] = {.entity = entity, .body_id = system_physics.body_id};

			body_interface.AddBody(body_id, motion_type == physics_motion_type_e::static_body ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
			return true;
		}

		void destroy_body(entity_id_t entity, component_system_physics_t& system_physics, bool destroy_constraints = true)
		{
			SFG_ASSERT(system_physics.body_id != UINT32_MAX && system_physics.character == 0);

			if (destroy_constraints)
				destroy_body_constraints(entity);

			const JPH::BodyID	body_id(system_physics.body_id);
			JPH::BodyInterface& body_interface = _system->GetBodyInterface();
			body_interface.RemoveBody(body_id);
			body_interface.DestroyBody(body_id);
			_body_lookup[body_id.GetIndex()] = {};

			system_physics.body_id			 = UINT32_MAX;
			system_physics.single_sub_entity = NULL_ENTITY_ID;
		}

		bool create_character(entity_id_t entity, component_system_physics_t& system_physics, const component_character_mover_t& mover)
		{
			_world->calculate_transform_direct(entity);
			system_physics.motion_type = static_cast<u8>(physics_motion_type_e::kinematic_body);

			const ecs_component_table_t&		transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			const component_system_transform_t& transform		= ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);

			JPH::CapsuleShapeSettings		shape_settings(math::max(mover.half_height, 0.001f), math::max(mover.radius, 0.001f));
			JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();

			if (shape_result.HasError())
			{
				SFG_WARN("failed to create character shape for entity {0}: {1}", entity, shape_result.GetError().c_str());
				return false;
			}

			JPH::CharacterVirtualSettings settings = {};
			settings.mMaxSlopeAngle				   = math::degrees_to_radians(mover.max_slope_degrees);
			settings.mMaxStrength				   = mover.max_strength;
			settings.mMass						   = mover.mass;
			settings.mShape						   = shape_result.Get();
			settings.mShapeOffset				   = physics_world_util_t::to_jolt(mover.shape_offset);
			settings.mCharacterPadding			   = mover.padding;
			settings.mPredictiveContactDistance	   = mover.predictive_contact_distance;
			settings.mPenetrationRecoverySpeed	   = mover.penetration_recovery_speed;
			settings.mEnhancedInternalEdgeRemoval  = mover.enhanced_internal_edge_removal != 0;
			settings.mSupportingVolume			   = JPH::Plane(JPH::Vec3::sAxisY(), -mover.radius);
			settings.mInnerBodyShape			   = shape_result.Get();
			settings.mInnerBodyLayer			   = physics_world_util_t::make_object_layer(mover.collision_layer, physics_motion_type_e::kinematic_body);

			JPH::CharacterVirtual* character = new JPH::CharacterVirtual(&settings, physics_world_util_t::to_jolt_position(transform.abs_pos), physics_world_util_t::to_jolt(transform.abs_rot), entity, _system);
			character->AddRef();

			const JPH::BodyID body_id		 = character->GetInnerBodyID();
			system_physics.character		 = character;
			system_physics.body_id			 = body_id.GetIndexAndSequenceNumber();
			system_physics.single_sub_entity = entity;
			_body_lookup[body_id.GetIndex()] = {.entity = entity, .body_id = system_physics.body_id};
			return true;
		}

		void destroy_character(entity_id_t entity, component_system_physics_t& system_physics)
		{
			SFG_ASSERT(system_physics.character != 0 && system_physics.body_id != UINT32_MAX);

			const JPH::BodyID body_id(system_physics.body_id);
			_body_lookup[body_id.GetIndex()] = {};
			system_physics.character->Release();
			system_physics.character			= nullptr;
			system_physics.body_id				= UINT32_MAX;
			system_physics.single_sub_entity	= NULL_ENTITY_ID;
			system_physics.last_ground_velocity = vec3f_t::zero;
		}

		void create_entity_physics(entity_id_t entity, const component_physical_t* physical, const component_character_mover_t* mover)
		{
			ecs_component_table_t&		system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			component_system_physics_t& phy					 = ecs_helpers_t::table_add_or_get_as<component_system_physics_t>(system_physics_table, entity);

			if (physical != nullptr)
			{
				create_body(entity, phy, *physical);
				return;
			}

			create_character(entity, phy, *mover);
		}

		void destroy_entity_physics(entity_id_t entity)
		{
			ecs_component_table_t&		system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			component_system_physics_t* system_physics		 = ecs_helpers_t::table_find_as<component_system_physics_t>(system_physics_table, entity);
			SFG_ASSERT(system_physics != nullptr);

			if (system_physics->body_id != UINT32_MAX && system_physics->character == 0)
				destroy_body(entity, *system_physics);
			else if (system_physics->body_id == UINT32_MAX && system_physics->character != nullptr)
			{
				destroy_body_constraints(entity);
				destroy_character(entity, *system_physics);
			}

			ecs_t::table_remove(system_physics_table, entity);
		}

		void create_constraint(entity_id_t entity)
		{
			component_system_constraints_t system_constraints = {};

			if (!physics_world_util_t::create_constraints(*_world, *_system, entity, system_constraints))
				return;

			SFG_ASSERT(system_constraints.active_mask != 0);

			ecs_component_table_t&			system_constraints_table = _world->get_component_table(type_id_t<component_system_constraints_t>::value);
			component_system_constraints_t& stored_constraints		 = ecs_helpers_t::table_add_or_get_as<component_system_constraints_t>(system_constraints_table, entity);

			stored_constraints = system_constraints;
		}

		void destroy_constraint(entity_id_t entity)
		{
			ecs_component_table_t&			system_constraints_table = _world->get_component_table(type_id_t<component_system_constraints_t>::value);
			component_system_constraints_t* system_constraints		 = ecs_helpers_t::table_find_as<component_system_constraints_t>(system_constraints_table, entity);
			SFG_ASSERT(system_constraints != nullptr);

			physics_world_util_t::destroy_constraints(*_system, *system_constraints);
			ecs_t::table_remove(system_constraints_table, entity);
		}

		void destroy_body_constraints(entity_id_t entity)
		{
			const ecs_component_table_t&	system_constraints_table = _world->get_component_table(type_id_t<component_system_constraints_t>::value);
			const ecs_component_table_ref_t refs[]					 = {system_constraints_table.ref()};
			frame_vector_t<entity_id_t>		constraint_entities		 = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_constraints_t& system_constraints = ecs_helpers_t::row_get<component_system_constraints_t>(row, 0);

				if (row.id == entity)
				{
					constraint_entities.push_back(row.id);
					continue;
				}

				for (u32 i = 0; i < static_cast<u32>(system_constraint_type_e::count); ++i)
				{
					const u16 slot_bit = static_cast<u16>(1u << i);

					if ((system_constraints.active_mask & slot_bit) != 0 && system_constraints.slots[i].target_entity == entity)
					{
						constraint_entities.push_back(row.id);
						break;
					}
				}
			}

			for (entity_id_t constraint_entity : constraint_entities)
				destroy_constraint(constraint_entity);
		}

		void sync_constraint_create_destroy()
		{
			ZoneScoped;

			const ecs_component_table_t& system_constraints_table = _world->get_component_table(type_id_t<component_system_constraints_t>::value);
			const ecs_component_table_t& system_physics_table	  = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const ecs_component_table_t& disabled_table			  = _world->get_component_table(type_id_t<component_disabled_t>::value);
			const ecs_component_table_t* constraint_tables[]	  = {
				&_world->get_component_table(type_id_t<component_fixed_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_distance_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_point_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_hinge_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_cone_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_slider_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_swing_twist_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_six_dof_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_pulley_constraint_t>::value),
				&_world->get_component_table(type_id_t<component_vehicle_constraint_t>::value),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			{
				const ecs_component_table_ref_t refs[] = {system_constraints_table.ref(), disabled_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					destroy_entities.push_back(row.id);

				for (entity_id_t entity : destroy_entities)
					destroy_constraint(entity);
			}

			for (const ecs_component_table_t* constraint_table : constraint_tables)
			{
				const ecs_component_table_ref_t refs[] = {constraint_table->ref(), system_physics_table.ref(), !disabled_table.ref(), !system_constraints_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					create_constraint(row.id);
			}
		}

		void sync_constraint_properties()
		{
			ZoneScoped;

			const ecs_component_table_t&	system_constraints_table = _world->get_component_table(type_id_t<component_system_constraints_t>::value);
			const ecs_component_table_ref_t refs[]					 = {system_constraints_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_constraints_t& system_constraints = ecs_helpers_t::row_get<component_system_constraints_t>(row, 0);

				physics_world_util_t::sync_constraint_properties(*_world, *_system, row.id, system_constraints);
			}
		}

		void sync_body_create_destroy()
		{
			ZoneScoped;

			const ecs_component_table_t& system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const ecs_component_table_t& mover_table		  = _world->get_component_table(type_id_t<component_character_mover_t>::value);
			const ecs_component_table_t& physical_table		  = _world->get_component_table(type_id_t<component_physical_t>::value);
			const ecs_component_table_t& disabled_table		  = _world->get_component_table(type_id_t<component_disabled_t>::value);
			const ecs_component_table_t& ragdoll_table		  = _world->get_component_table(type_id_t<component_ragdoll_t>::value);

			frame_vector_t<entity_id_t> destroy_entities = {};

			// destroy flow, disabled
			{
				destroy_entities.resize(0);
				const ecs_component_table_ref_t refs[] = {system_physics_table.ref(), disabled_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					destroy_entities.push_back(row.id);
				}

				for (entity_id_t id : destroy_entities)
					destroy_entity_physics(id);
			}

			// destroy flow, no mover, no physical
			{
				destroy_entities.resize(0);
				const ecs_component_table_ref_t refs[] = {system_physics_table.ref(), !mover_table.ref(), !physical_table.ref(), !disabled_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					destroy_entities.push_back(row.id);
				}

				for (entity_id_t id : destroy_entities)
					destroy_entity_physics(id);
			}

			// destroy flow, ragdoll owns physics
			{
				destroy_entities.resize(0);
				const ecs_component_table_ref_t refs[] = {system_physics_table.ref(), ragdoll_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					destroy_entities.push_back(row.id);

				for (entity_id_t id : destroy_entities)
					destroy_entity_physics(id);
			}

			// create flow - no mover, physical
			{
				const ecs_component_table_ref_t refs[] = {physical_table.ref(), !mover_table.ref(), !system_physics_table.ref(), !disabled_table.ref(), !ragdoll_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					const component_physical_t& rb = ecs_helpers_t::row_get<component_physical_t>(row, 0);
					create_entity_physics(row.id, &rb, nullptr);
				}
			}

			// create flow - no physical, mover
			{
				const ecs_component_table_ref_t refs[] = {mover_table.ref(), !physical_table.ref(), !system_physics_table.ref(), !disabled_table.ref(), !ragdoll_table.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					const component_character_mover_t& mover = ecs_helpers_t::row_get<component_character_mover_t>(row, 0);
					create_entity_physics(row.id, nullptr, &mover);
				}
			}
		}

		void update_characters(f32 delta_time)
		{
			ZoneScoped;

			ecs_component_table_t&			system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const ecs_component_table_t&	disabled_table		 = _world->get_component_table(type_id_t<component_disabled_t>::value);
			const ecs_component_table_t&	mover_table			 = _world->get_component_table(type_id_t<component_character_mover_t>::value);
			const ecs_component_table_ref_t refs[]				 = {system_physics_table.ref(), mover_table.ref(), !disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				component_system_physics_t& system_physics = ecs_helpers_t::row_get_mutable<component_system_physics_t>(row, 0);

				if (system_physics.character == nullptr)
					continue;

				const component_character_mover_t& mover	 = ecs_helpers_t::row_get<component_character_mover_t>(row, 1);
				JPH::CharacterVirtual&			   character = *system_physics.character;

				character.UpdateGroundVelocity();

				const JPH::Vec3 ground_velocity = character.GetGroundVelocity();
				JPH::Vec3		velocity		= character.GetLinearVelocity();

				if (character.GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround)
				{
					const f32 vertical_velocity		   = velocity.Dot(JPH::Vec3::sAxisY());
					const f32 ground_vertical_velocity = ground_velocity.Dot(JPH::Vec3::sAxisY());

					if (vertical_velocity - ground_vertical_velocity < 0.1f)
					{
						const JPH::Vec3 previous_ground = physics_world_util_t::to_jolt(system_physics.last_ground_velocity);
						velocity						= velocity - previous_ground + ground_velocity;
						velocity -= velocity.Dot(JPH::Vec3::sAxisY()) * JPH::Vec3::sAxisY();
						velocity += ground_vertical_velocity * JPH::Vec3::sAxisY();
					}
				}

				velocity += physics_world_util_t::to_jolt(_config.gravity) * delta_time;
				character.SetLinearVelocity(velocity);
				system_physics.last_ground_velocity = physics_world_util_t::from_jolt(ground_velocity);

				query_broad_phase_filter_t broad_filter = {};
				broad_filter.flags						= physics_query_flag_all;

				query_object_layer_filter_t layer_filter = {};
				layer_filter.layers						 = _config.collision_masks[mover.collision_layer];

				const physics_query_filter_t query_filter{
					.collision_layers = {.bits = layer_filter.layers},
					.ignored_entity	  = row.id,
				};

				query_body_filter_t body_filter = {};
				body_filter.impl				= this;
				body_filter.filter				= &query_filter;

				JPH::CharacterVirtual::ExtendedUpdateSettings update_settings = {};
				update_settings.mStickToFloorStepDown						  = JPH::Vec3(0.0f, -mover.step_down, 0.0f);
				update_settings.mWalkStairsStepUp							  = JPH::Vec3(0.0f, mover.step_up, 0.0f);
				update_settings.mWalkStairsMinStepForward					  = mover.min_step_forward;
				update_settings.mWalkStairsStepForwardTest					  = mover.step_forward_test;

				character.ExtendedUpdate(delta_time, physics_world_util_t::to_jolt(_config.gravity), update_settings, broad_filter, layer_filter, body_filter, {}, *_temp_allocator);
			}
		}

		void sync_dynamic_bodies_into_world()
		{
			ZoneScoped;

			const ecs_component_table_t& system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const ecs_component_table_t& disabled_table		  = _world->get_component_table(type_id_t<component_disabled_t>::value);

			ecs_component_table_t& system_transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			ecs_component_table_t& transform_table		  = _world->get_component_table(type_id_t<component_transform_t>::value);

			JPH::BodyInterface& body_interface = _system->GetBodyInterface();

			const ecs_component_table_ref_t absolute_refs[] = {system_physics_table.ref(), system_transform_table.ref(), !disabled_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = absolute_refs, .size = std::size(absolute_refs)}))
			{
				const component_system_physics_t& system_physics = ecs_helpers_t::row_get<component_system_physics_t>(row, 0);

				if (system_physics.motion_type == static_cast<u8>(physics_motion_type_e::static_body))
					continue;

				JPH::RVec3 body_position = JPH::RVec3::sZero();
				JPH::Quat  body_rotation = JPH::Quat::sIdentity();
				body_interface.GetPositionAndRotation(JPH::BodyID(system_physics.body_id), body_position, body_rotation);

				component_system_transform_t& system_transform = ecs_helpers_t::row_get_mutable<component_system_transform_t>(row, 1);
				const quat_t					 entity_rotation  = physics_world_util_t::from_jolt(body_rotation) * system_physics.local_rotation.inverse();
				const vec3f_t				 entity_position  = physics_world_util_t::from_jolt(body_position) - entity_rotation * (system_physics.local_position * system_transform.abs_scale);

				system_transform.abs_pos = entity_position;
				system_transform.abs_rot = entity_rotation;
				system_transform.abs_mat = mat4x3_t::transform(entity_position, entity_rotation, system_transform.abs_scale);

				_world->set_entity_pos_local(row.id, _world->abs_pos_to_local(row.id, entity_position));
				_world->set_entity_rot_local(row.id, _world->abs_rot_to_local(row.id, entity_rotation));
			}
		}

		void sync_ragdolls_into_world()
		{
			ZoneScoped;

			const ecs_component_table_t&	system_ragdoll_table   = _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
			ecs_component_table_t&			system_transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			resource_manager_t&				resource_manager	   = resource_manager_t::get();
			const chunk_allocator32_t&		resource_memory		   = resource_manager.get_memory();
			const ecs_component_table_ref_t refs[]				   = {system_ragdoll_table.ref(), system_transform_table.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				component_system_ragdoll_t&	  system_ragdoll   = ecs_helpers_t::row_get_mutable<component_system_ragdoll_t>(row, 0);
				component_system_transform_t& system_transform = ecs_helpers_t::row_get_mutable<component_system_transform_t>(row, 1);
				const ragdoll_runtime_t*	  resource		   = resource_manager.find_runtime<ragdoll_runtime_t>(system_ragdoll.ragdoll_resource);
				const skeleton_runtime_t*	  skeleton		   = resource_manager.find_runtime<skeleton_runtime_t>(system_ragdoll.skeleton);

				SFG_ASSERT(resource != nullptr);
				SFG_ASSERT(skeleton != nullptr);

				const ragdoll_part_runtime_t*	parts			 = resource_memory.get<ragdoll_part_runtime_t>(resource->parts);
				const skeleton_joint_runtime_t* joints			 = resource_memory.get<skeleton_joint_runtime_t>(skeleton->joints);
				const u32*						evaluation_order = resource_memory.get<u32>(skeleton->evaluation_order);
				mat4x3_t*						joint_globals	 = _ragdoll_pose_memory.get<mat4x3_t>(system_ragdoll.joint_global_pose);
				const mat4x3_t*					frozen_locals	 = _ragdoll_pose_memory.get<mat4x3_t>(system_ragdoll.frozen_local_pose);

				JPH::RVec3 root_offset				   = JPH::RVec3::sZero();
				JPH::Mat44 part_pose[RAGDOLL_PART_MAX] = {};
				system_ragdoll.ragdoll->GetPose(root_offset, part_pose);

				u8 joint_to_part[MAX_SKELETON_BONES] = {};
				SFG_MEMSET(joint_to_part, UINT8_MAX, sizeof(joint_to_part));

				mat4x3_t part_world[RAGDOLL_PART_MAX] = {};

				for (u32 part_index = 0; part_index < resource->part_count; ++part_index)
				{
					const JPH::Mat44& pose		  = part_pose[part_index];
					const JPH::Vec3	  translation = pose.GetTranslation() + JPH::Vec3(root_offset);

					part_world[part_index] = mat4x3_t(pose.GetAxisX().GetX(),
													  pose.GetAxisX().GetY(),
													  pose.GetAxisX().GetZ(),
													  pose.GetAxisY().GetX(),
													  pose.GetAxisY().GetY(),
													  pose.GetAxisY().GetZ(),
													  pose.GetAxisZ().GetX(),
													  pose.GetAxisZ().GetY(),
													  pose.GetAxisZ().GetZ(),
													  translation.GetX(),
													  translation.GetY(),
													  translation.GetZ());

					joint_to_part[parts[part_index].joint_index] = static_cast<u8>(part_index);
				}

				const mat4x3_t entity_world_rigid = part_world[0] * system_ragdoll.entity_from_root;
				vec3f_t		   entity_position	  = vec3f_t::zero;
				quat_t		   entity_rotation	  = quat_t::identity;
				vec3f_t		   entity_scale		  = vec3f_t::one;
				entity_world_rigid.decompose(entity_position, entity_rotation, entity_scale);

				const mat4x3_t inverse_entity_world_rigid = mat4x3_t::transform(entity_position, entity_rotation, vec3f_t::one).inverse();

				for (u32 part_index = 0; part_index < resource->part_count; ++part_index)
				{
					const u32 joint_index = parts[part_index].joint_index;

					vec3f_t preserved_position = vec3f_t::zero;
					quat_t  preserved_rotation = quat_t::identity;
					vec3f_t preserved_scale    = vec3f_t::one;
					joint_globals[joint_index].decompose(preserved_position, preserved_rotation, preserved_scale);

					vec3f_t joint_position = vec3f_t::zero;
					quat_t  joint_rotation = quat_t::identity;
					vec3f_t rigid_scale    = vec3f_t::one;
					(inverse_entity_world_rigid * part_world[part_index]).decompose(joint_position, joint_rotation, rigid_scale);

					joint_globals[joint_index] = mat4x3_t::transform(joint_position, joint_rotation, preserved_scale);
				}

				for (u32 order_index = 0; order_index < skeleton->joint_count; ++order_index)
				{
					const u32 joint_index = evaluation_order[order_index];

					if (joint_to_part[joint_index] != UINT8_MAX)
						continue;

					const u32 parent_index	   = joints[joint_index].parent_index;
					joint_globals[joint_index] = parent_index == SKELETON_JOINT_NO_PARENT ? frozen_locals[joint_index] : joint_globals[parent_index] * frozen_locals[joint_index];
				}

				system_transform.abs_pos = entity_position;
				system_transform.abs_rot = entity_rotation;
				system_transform.abs_mat = mat4x3_t::transform(entity_position, entity_rotation, system_transform.abs_scale);

				_world->set_entity_pos_local(row.id, _world->abs_pos_to_local(row.id, entity_position));
				_world->set_entity_rot_local(row.id, _world->abs_rot_to_local(row.id, entity_rotation));

				const JPH::AABox bounds		= system_ragdoll.ragdoll->GetWorldSpaceBounds();
				system_ragdoll.world_bounds = {
					{bounds.mMin.GetX(), bounds.mMin.GetY(), bounds.mMin.GetZ()},
					{bounds.mMax.GetX(), bounds.mMax.GetY(), bounds.mMax.GetZ()},
				};
			}
		}

		void drain_contact_events()
		{
			ZoneScoped;

			raw_contact_event_t raw = {};

			while (_raw_contact_events.try_dequeue(raw))
			{
				const JPH::BodyID&		   body_id_a = raw.pair.GetBody1ID();
				const JPH::BodyID&		   body_id_b = raw.pair.GetBody2ID();
				const JPH::SubShapeIDPair& pair		 = raw.pair;

				if (raw.type == physics_contact_type_e::end)
				{
					const auto sensor = _contact_sensors.find(pair);

					if (sensor != _contact_sensors.end())
					{
						raw.is_sensor = sensor->second;
						_contact_sensors.erase(sensor);
					}
				}
				else
					_contact_sensors.insert_or_assign(pair, raw.is_sensor);

				const entity_id_t entity_a = resolve_body_entity(body_id_a);
				const entity_id_t entity_b = resolve_body_entity(body_id_b);

				if (entity_a == NULL_ENTITY_ID || entity_b == NULL_ENTITY_ID)
					continue;

				_contact_events.push_back({
					.position		= physics_world_util_t::from_jolt(raw.position),
					.normal			= physics_world_util_t::from_jolt(raw.normal),
					.entity_a		= entity_a,
					.entity_b		= entity_b,
					.sub_shape_a	= raw.pair.GetSubShapeID1().GetValue(),
					.sub_shape_b	= raw.pair.GetSubShapeID2().GetValue(),
					.penetration	= raw.penetration,
					.sub_shape_id_a = raw.pair.GetSubShapeID1().GetValue(),
					.sub_shape_id_b = raw.pair.GetSubShapeID2().GetValue(),
					.type			= raw.type,
					.is_sensor		= raw.is_sensor,
				});
			}
		}

		bool validate_ray(const physics_raycast_t& ray, JPH::RRayCast& out_ray) const
		{
			if (ray.distance <= 0.0f || ray.direction.is_zero())
				return false;

			const vec3f_t delta = ray.direction.normalized() * ray.distance;
			out_ray				= JPH::RRayCast(physics_world_util_t::to_jolt_position(ray.origin), physics_world_util_t::to_jolt(delta));
			return true;
		}

		void fill_hit(const JPH::RayCastResult& result, const physics_raycast_t& ray, physics_hit_t& hit)
		{
			const entity_id_t entity = resolve_body_entity(result.mBodyID);
			SFG_ASSERT(entity != NULL_ENTITY_ID);

			const ecs_component_table_t&	  system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
			const body_lookup_t&			  lookup			   = _body_lookup[result.mBodyID.GetIndex()];

			const vec3f_t delta = ray.direction.normalized() * ray.distance;
			hit.position		= ray.origin + delta * result.mFraction;
			hit.distance		= ray.distance * result.mFraction;
			hit.fraction		= result.mFraction;
			hit.entity			= entity;
			hit.sub_shape_id	= result.mSubShapeID2.GetValue();
			hit.is_sensor		= _system->GetBodyInterface().IsSensor(result.mBodyID);
			hit.is_character	= system_physics != nullptr && system_physics->character != 0;

			if (system_physics != nullptr)
				hit.physical_material = system_physics->physical_material;
			else
			{
				const ecs_component_table_t&	  system_ragdoll_table = _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
				const component_system_ragdoll_t& system_ragdoll	   = ecs_helpers_t::table_get_as_const<component_system_ragdoll_t>(system_ragdoll_table, entity);
				const ragdoll_runtime_t*		  ragdoll_resource	   = resource_manager_t::get().find_runtime<ragdoll_runtime_t>(system_ragdoll.ragdoll_resource);
				SFG_ASSERT(ragdoll_resource != nullptr);

				hit.physical_material = ragdoll_resource->physical_material;
				hit.sub_shape_id	  = lookup.ragdoll_part;
			}

			JPH::BodyLockRead lock(_system->GetBodyLockInterface(), result.mBodyID);

			if (lock.Succeeded())
				hit.normal = physics_world_util_t::from_jolt(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, physics_world_util_t::to_jolt_position(hit.position)));
		}

		bool raycast_closest(const physics_raycast_t& ray, physics_hit_t& out_hit, const physics_query_filter_t& filter)
		{
			JPH::RRayCast jolt_ray = {};

			if (!validate_ray(ray, jolt_ray))
				return false;

			query_broad_phase_filter_t broad_filter = {};
			broad_filter.flags						= filter.flags;

			query_object_layer_filter_t layer_filter = {};
			layer_filter.layers						 = filter.collision_layers.bits;

			query_body_filter_t body_filter = {};
			body_filter.impl				= this;
			body_filter.filter				= &filter;

			JPH::RayCastResult result = {};

			if (!_system->GetNarrowPhaseQuery().CastRay(jolt_ray, result, broad_filter, layer_filter, body_filter))
				return false;

			fill_hit(result, ray, out_hit);
			return true;
		}

		bool raycast_any(const physics_raycast_t& ray, const physics_query_filter_t& filter)
		{
			JPH::RRayCast jolt_ray = {};

			if (!validate_ray(ray, jolt_ray))
				return false;

			query_broad_phase_filter_t broad_filter = {};
			broad_filter.flags						= filter.flags;

			query_object_layer_filter_t layer_filter = {};
			layer_filter.layers						 = filter.collision_layers.bits;

			query_body_filter_t body_filter = {};
			body_filter.impl				= this;
			body_filter.filter				= &filter;

			JPH::AnyHitCollisionCollector<JPH::CastRayCollector> collector = {};
			JPH::RayCastSettings								 settings  = {};

			_system->GetNarrowPhaseQuery().CastRay(jolt_ray, settings, collector, broad_filter, layer_filter, body_filter);
			return collector.HadHit();
		}

		physics_query_result_t raycast_all(const physics_raycast_t& ray, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter)
		{
			JPH::RRayCast jolt_ray = {};

			if (!validate_ray(ray, jolt_ray))
				return {};

			query_broad_phase_filter_t broad_filter = {};
			broad_filter.flags						= filter.flags;

			query_object_layer_filter_t layer_filter = {};
			layer_filter.layers						 = filter.collision_layers.bits;

			query_body_filter_t body_filter = {};
			body_filter.impl				= this;
			body_filter.filter				= &filter;

			ray_all_collector_t collector = {};
			collector.impl				  = this;
			collector.hits				  = out_hits.data;
			collector.ray				  = &ray;
			collector.capacity			  = static_cast<u32>(out_hits.size);

			JPH::RayCastSettings settings = {};

			_system->GetNarrowPhaseQuery().CastRay(jolt_ray, settings, collector, broad_filter, layer_filter, body_filter);

			if (collector.count > 1)
				std::sort(out_hits.data, out_hits.data + collector.count, [](const physics_hit_t& lhs, const physics_hit_t& rhs) { return lhs.fraction < rhs.fraction; });

			return {.hit_count = collector.count, .overflow = collector.overflow};
		}

		bool validate_spherecast(const physics_spherecast_t& sphere) const
		{
			return sphere.radius > 0.0f && sphere.distance > 0.0f && !sphere.direction.is_zero();
		}

		void fill_hit(const JPH::ShapeCastResult& result, const physics_spherecast_t& sphere, physics_hit_t& hit)
		{
			const entity_id_t entity = resolve_body_entity(result.mBodyID2);
			SFG_ASSERT(entity != NULL_ENTITY_ID);

			const ecs_component_table_t&	  system_physics_table = _world->get_component_table(type_id_t<component_system_physics_t>::value);
			const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
			const body_lookup_t&			  lookup			   = _body_lookup[result.mBodyID2.GetIndex()];

			hit.position	 = sphere.origin + physics_world_util_t::from_jolt(result.mContactPointOn2);
			hit.normal		 = physics_world_util_t::from_jolt(-result.mPenetrationAxis.NormalizedOr(JPH::Vec3::sZero()));
			hit.entity		 = entity;
			hit.distance	 = sphere.distance * result.mFraction;
			hit.fraction	 = result.mFraction;
			hit.sub_shape_id = result.mSubShapeID2.GetValue();
			hit.is_sensor	 = _system->GetBodyInterface().IsSensor(result.mBodyID2);
			hit.is_character = system_physics != nullptr && system_physics->character != 0;

			if (system_physics != nullptr)
				hit.physical_material = system_physics->physical_material;
			else
			{
				const ecs_component_table_t&	  system_ragdoll_table = _world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
				const component_system_ragdoll_t& system_ragdoll	   = ecs_helpers_t::table_get_as_const<component_system_ragdoll_t>(system_ragdoll_table, entity);
				const ragdoll_runtime_t*		  ragdoll_resource	   = resource_manager_t::get().find_runtime<ragdoll_runtime_t>(system_ragdoll.ragdoll_resource);
				SFG_ASSERT(ragdoll_resource != nullptr);

				hit.physical_material = ragdoll_resource->physical_material;
				hit.sub_shape_id	  = lookup.ragdoll_part;
			}
		}

		bool spherecast_any(const physics_spherecast_t& sphere, const physics_query_filter_t& filter)
		{
			if (!validate_spherecast(sphere))
				return false;

			const JPH::SphereShape shape(sphere.radius);
			const vec3f_t		   delta = sphere.direction.normalized() * sphere.distance;
			const JPH::RShapeCast  shape_cast(&shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(physics_world_util_t::to_jolt_position(sphere.origin)), physics_world_util_t::to_jolt(delta));

			query_broad_phase_filter_t broad_filter = {};
			broad_filter.flags						= filter.flags;

			query_object_layer_filter_t layer_filter = {};
			layer_filter.layers						 = filter.collision_layers.bits;

			query_body_filter_t body_filter = {};
			body_filter.impl				= this;
			body_filter.filter				= &filter;

			JPH::AnyHitCollisionCollector<JPH::CastShapeCollector> collector = {};
			JPH::ShapeCastSettings								   settings	 = {};
			_system->GetNarrowPhaseQuery().CastShape(shape_cast, settings, physics_world_util_t::to_jolt_position(sphere.origin), collector, broad_filter, layer_filter, body_filter);
			return collector.HadHit();
		}

		bool spherecast_closest(const physics_spherecast_t& sphere, physics_hit_t& out_hit, const physics_query_filter_t& filter)
		{
			if (!validate_spherecast(sphere))
				return false;

			const JPH::SphereShape shape(sphere.radius);
			const vec3f_t		   delta = sphere.direction.normalized() * sphere.distance;
			const JPH::RShapeCast  shape_cast(&shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(physics_world_util_t::to_jolt_position(sphere.origin)), physics_world_util_t::to_jolt(delta));

			query_broad_phase_filter_t broad_filter = {};
			broad_filter.flags						= filter.flags;

			query_object_layer_filter_t layer_filter = {};
			layer_filter.layers						 = filter.collision_layers.bits;

			query_body_filter_t body_filter = {};
			body_filter.impl				= this;
			body_filter.filter				= &filter;

			JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector = {};
			JPH::ShapeCastSettings									   settings	 = {};
			_system->GetNarrowPhaseQuery().CastShape(shape_cast, settings, physics_world_util_t::to_jolt_position(sphere.origin), collector, broad_filter, layer_filter, body_filter);

			if (!collector.HadHit())
				return false;

			fill_hit(collector.mHit, sphere, out_hit);
			return true;
		}

		physics_query_result_t spherecast_all(const physics_spherecast_t& sphere, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter)
		{
			if (!validate_spherecast(sphere))
				return {};

			const JPH::SphereShape shape(sphere.radius);
			const vec3f_t		   delta = sphere.direction.normalized() * sphere.distance;
			const JPH::RShapeCast  shape_cast(&shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(physics_world_util_t::to_jolt_position(sphere.origin)), physics_world_util_t::to_jolt(delta));

			query_broad_phase_filter_t broad_filter = {};
			broad_filter.flags						= filter.flags;

			query_object_layer_filter_t layer_filter = {};
			layer_filter.layers						 = filter.collision_layers.bits;

			query_body_filter_t body_filter = {};
			body_filter.impl				= this;
			body_filter.filter				= &filter;

			sphere_all_collector_t collector = {};
			collector.impl					 = this;
			collector.hits					 = out_hits.data;
			collector.sphere				 = &sphere;
			collector.capacity				 = static_cast<u32>(out_hits.size);

			JPH::ShapeCastSettings settings = {};
			_system->GetNarrowPhaseQuery().CastShape(shape_cast, settings, physics_world_util_t::to_jolt_position(sphere.origin), collector, broad_filter, layer_filter, body_filter);

			if (collector.count > 1)
				std::sort(out_hits.data, out_hits.data + collector.count, [](const physics_hit_t& lhs, const physics_hit_t& rhs) { return lhs.fraction < rhs.fraction; });

			return {.hit_count = collector.count, .overflow = collector.overflow};
		}
	};

	physics_world_t::physics_world_t()	= default;
	physics_world_t::~physics_world_t() = default;

	void physics_world_t::init(world_t& world, const physics_runtime_config_t& config)
	{
		SFG_ASSERT(_impl == nullptr);

		_impl = new impl_t();
		_impl->init(world, config);
	}

	void physics_world_t::uninit()
	{
		SFG_ASSERT(_impl != nullptr);

		_impl->uninit();
		delete _impl;

		_impl = nullptr;
	}

	void physics_world_t::clear()
	{
		_impl->clear();
	}

	void physics_world_t::tick(f32 delta_time)
	{
		_impl->tick(delta_time);
	}

	void physics_world_t::draw_debug(world_debug_draw_t& debug_draw)
	{
		physics_runtime_t::draw_debug(*_impl->_system, debug_draw);
	}

	void physics_world_t::sync_body_create_destroy()
	{
		_impl->sync_body_create_destroy();
	}

	void physics_world_t::destroy_entity(entity_id_t entity)
	{
		const ecs_component_table_t& system_ragdoll_table = _impl->_world->get_component_table(type_id_t<component_system_ragdoll_t>::value);

		if (ecs_t::table_has(system_ragdoll_table, entity))
			_impl->destroy_ragdoll(entity, false);

		const ecs_component_table_t& system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);

		if (ecs_t::table_has(system_physics_table, entity))
			_impl->destroy_entity_physics(entity);
	}

	void physics_world_t::destroy_body(entity_id_t entity)
	{
		const ecs_component_table_t& system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		component_system_physics_t*	 sys				  = ecs_helpers_t::table_find_as<component_system_physics_t>(system_physics_table, entity);

		if (!sys)
		{
			return;
		}

		_impl->destroy_body(entity, *sys);
	}

	span_t<const mat4x3_t> physics_world_t::get_ragdoll_joint_globals(entity_id_t entity) const
	{
		const ecs_component_table_t&	  system_ragdoll_table = _impl->_world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
		const component_system_ragdoll_t& system_ragdoll	   = ecs_helpers_t::table_get_as_const<component_system_ragdoll_t>(system_ragdoll_table, entity);

		return {
			.data = _impl->_ragdoll_pose_memory.get<mat4x3_t>(system_ragdoll.joint_global_pose),
			.size = system_ragdoll.joint_count,
		};
	}

	bool physics_world_t::set_body_linear_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		const ecs_component_table_t&	  system_ragdoll_table = _impl->_world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
		const component_system_ragdoll_t* system_ragdoll		 = ecs_helpers_t::table_find_as_const<component_system_ragdoll_t>(system_ragdoll_table, entity);

		if (system_ragdoll != nullptr && system_ragdoll->ragdoll != nullptr)
		{
			system_ragdoll->ragdoll->SetLinearVelocity(physics_world_util_t::to_jolt(velocity));
			return true;
		}

		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

		if (system_physics == nullptr || system_physics->character != nullptr || system_physics->body_id == UINT32_MAX)
			return false;

		if (system_physics->motion_type == static_cast<u8>(physics_motion_type_e::static_body))
		{
			SFG_WARN("can't set linear velocity of a static body!");
			return false;
		}

		_impl->_system->GetBodyInterface().SetLinearVelocity(JPH::BodyID(system_physics->body_id), physics_world_util_t::to_jolt(velocity));
		return true;
	}

	void physics_world_t::set_body_angular_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr);
		SFG_ASSERT(system_physics->character != nullptr || system_physics->body_id != UINT32_MAX);

		if (system_physics->motion_type == static_cast<u8>(physics_motion_type_e::static_body))
		{
			SFG_WARN("can't set angular velocity of a static body!");
			return;
		}

		_impl->_system->GetBodyInterface().SetAngularVelocity(JPH::BodyID(system_physics->body_id), physics_world_util_t::to_jolt(velocity));
	}

	void physics_world_t::add_body_force(entity_id_t entity, const vec3f_t& force)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr);
		SFG_ASSERT(system_physics->character != nullptr || system_physics->body_id != UINT32_MAX);

		_impl->_system->GetBodyInterface().AddForce(JPH::BodyID(system_physics->body_id), physics_world_util_t::to_jolt(force));
	}

	bool physics_world_t::add_body_impulse(entity_id_t entity, const vec3f_t& impulse)
	{
		const ecs_component_table_t&	  system_ragdoll_table = _impl->_world->get_component_table(type_id_t<component_system_ragdoll_t>::value);
		const component_system_ragdoll_t* system_ragdoll		 = ecs_helpers_t::table_find_as_const<component_system_ragdoll_t>(system_ragdoll_table, entity);

		if (system_ragdoll != nullptr && system_ragdoll->ragdoll != nullptr)
		{
			system_ragdoll->ragdoll->AddImpulse(physics_world_util_t::to_jolt(impulse));
			return true;
		}

		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

		if (system_physics == nullptr || system_physics->character != nullptr || system_physics->body_id == UINT32_MAX)
			return false;

		_impl->_system->GetBodyInterface().AddImpulse(JPH::BodyID(system_physics->body_id), physics_world_util_t::to_jolt(impulse));
		return true;
	}

	void physics_world_t::wake_body(entity_id_t entity)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr);
		SFG_ASSERT(system_physics->character != nullptr || system_physics->body_id != UINT32_MAX);

		_impl->_system->GetBodyInterface().ActivateBody(JPH::BodyID(system_physics->body_id));
	}

	bool physics_world_t::get_body_state(entity_id_t entity, physics_body_state_t& out_state) const
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

		if (system_physics == nullptr || system_physics->character != 0 || system_physics->body_id == UINT32_MAX)
			return false;

		const JPH::BodyID body_id(system_physics->body_id);

		JPH::RVec3			position	   = JPH::RVec3::sZero();
		JPH::Quat			rotation	   = JPH::Quat::sIdentity();
		JPH::BodyInterface& body_interface = _impl->_system->GetBodyInterface();
		body_interface.GetPositionAndRotation(body_id, position, rotation);

		out_state.position		   = physics_world_util_t::from_jolt(position);
		out_state.rotation		   = physics_world_util_t::from_jolt(rotation);
		out_state.linear_velocity  = physics_world_util_t::from_jolt(body_interface.GetLinearVelocity(body_id));
		out_state.angular_velocity = physics_world_util_t::from_jolt(body_interface.GetAngularVelocity(body_id));
		out_state.is_active		   = body_interface.IsActive(body_id);
		return true;
	}

	bool physics_world_t::is_body(entity_id_t entity) const
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

		return system_physics != nullptr && system_physics->character == nullptr && system_physics->body_id != UINT32_MAX;
	}

	bool physics_world_t::raycast_any(const physics_raycast_t& ray, const physics_query_filter_t& filter) const
	{
		return _impl->raycast_any(ray, filter);
	}

	bool physics_world_t::raycast_closest(const physics_raycast_t& ray, physics_hit_t& out_hit, const physics_query_filter_t& filter) const
	{
		out_hit = {};
		return _impl->raycast_closest(ray, out_hit, filter);
	}

	physics_query_result_t physics_world_t::raycast_all(const physics_raycast_t& ray, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter) const
	{
		return _impl->raycast_all(ray, out_hits, filter);
	}

	bool physics_world_t::linecast_closest(const physics_linecast_t& line, physics_hit_t& out_hit, const physics_query_filter_t& filter) const
	{
		const vec3f_t delta = line.end - line.start;
		return raycast_closest({.origin = line.start, .direction = delta, .distance = delta.magnitude()}, out_hit, filter);
	}

	void physics_world_t::linecast_closest_batch(span_t<const physics_linecast_t> lines, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter) const
	{
		SFG_ASSERT(out_hits.size >= lines.size);

		for (size_t i = 0; i < lines.size; ++i)
		{
			out_hits.data[i] = {};
			linecast_closest(lines.data[i], out_hits.data[i], filter);
		}
	}

	bool physics_world_t::spherecast_any(const physics_spherecast_t& sphere, const physics_query_filter_t& filter) const
	{
		return _impl->spherecast_any(sphere, filter);
	}

	bool physics_world_t::spherecast_closest(const physics_spherecast_t& sphere, physics_hit_t& out_hit, const physics_query_filter_t& filter) const
	{
		out_hit = {};
		return _impl->spherecast_closest(sphere, out_hit, filter);
	}

	physics_query_result_t physics_world_t::spherecast_all(const physics_spherecast_t& sphere, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter) const
	{
		return _impl->spherecast_all(sphere, out_hits, filter);
	}

	void physics_world_t::set_character_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr && system_physics->character != 0);

		system_physics->character->SetLinearVelocity(physics_world_util_t::to_jolt(velocity));
	}

	void physics_world_t::add_character_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr && system_physics->character != 0);

		JPH::CharacterVirtual* character = system_physics->character;
		character->SetLinearVelocity(character->GetLinearVelocity() + physics_world_util_t::to_jolt(velocity));
	}

	void physics_world_t::jump_character(entity_id_t entity, f32 speed)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr && system_physics->character != 0);
		JPH::CharacterVirtual* character = system_physics->character;

		if (character->GetGroundState() != JPH::CharacterVirtual::EGroundState::OnGround)
			return;

		JPH::Vec3 velocity = character->GetLinearVelocity();
		velocity -= velocity.Dot(JPH::Vec3::sAxisY()) * JPH::Vec3::sAxisY();
		velocity += speed * JPH::Vec3::sAxisY();
		character->SetLinearVelocity(velocity);
	}

	void physics_world_t::teleport_character(entity_id_t entity, const vec3f_t& position)
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);
		SFG_ASSERT(system_physics != nullptr && system_physics->character != 0);

		system_physics->character->SetPosition(physics_world_util_t::to_jolt_position(position));
		_impl->_world->set_entity_pos_local(entity, _impl->_world->abs_pos_to_local(entity, position));
	}

	bool physics_world_t::get_character_state(entity_id_t entity, character_mover_state_t& out_state) const
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

		if (system_physics == nullptr || system_physics->character == 0)
			return false;

		const JPH::CharacterVirtual& jolt_character = *system_physics->character;
		out_state.velocity							= physics_world_util_t::from_jolt(jolt_character.GetLinearVelocity());
		out_state.ground_normal						= physics_world_util_t::from_jolt(jolt_character.GetGroundNormal());
		out_state.ground_velocity					= physics_world_util_t::from_jolt(jolt_character.GetGroundVelocity());
		out_state.is_grounded						= jolt_character.GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

		const JPH::BodyID ground_body_id = jolt_character.GetGroundBodyID();

		if (ground_body_id.IsInvalid())
		{
			out_state.ground_entity		  = NULL_ENTITY_ID;
			out_state.ground_sub_shape_id = 0;
			return true;
		}

		out_state.ground_entity		  = _impl->resolve_body_entity(ground_body_id);
		out_state.ground_sub_shape_id = jolt_character.GetGroundSubShapeID().GetValue();
		return true;
	}

	bool physics_world_t::is_character(entity_id_t entity) const
	{
		const ecs_component_table_t&	  system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const component_system_physics_t* system_physics	   = ecs_helpers_t::table_find_as_const<component_system_physics_t>(system_physics_table, entity);

		return system_physics != nullptr && system_physics->character != nullptr;
	}

	void physics_world_t::update_collision_masks(const u64 masks[PHYSICS_COLLISION_LAYER_MAX], u64 active_layers)
	{
		for (u32 i = 0; i < PHYSICS_COLLISION_LAYER_MAX; ++i)
			_impl->_config.collision_masks[i] = masks[i];

		_impl->_config.active_collision_layers = active_layers;
	}

	void physics_world_t::update_step_settings(u32 physics_rate, u32 max_sub_steps)
	{
		SFG_ASSERT(physics_rate != 0 && max_sub_steps != 0);

		_impl->_config.physics_rate	 = physics_rate;
		_impl->_config.max_sub_steps = max_sub_steps;
		_impl->_accumulator			 = 0.0f;
	}

	void physics_world_t::update_kinematic_sensors_collide_with_non_dynamic(bool enabled)
	{
		_impl->_config.kinematic_sensors_collide_with_non_dynamic = enabled;

		const ecs_component_table_t& system_physics_table = _impl->_world->get_component_table(type_id_t<component_system_physics_t>::value);
		const ecs_component_table_ref_t refs[] = {system_physics_table.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_system_physics_t& system_physics = ecs_helpers_t::row_get<component_system_physics_t>(row, 0);
			if (system_physics.character != nullptr ||
				system_physics.body_id == UINT32_MAX ||
				system_physics.motion_type != static_cast<u8>(physics_motion_type_e::kinematic_body))
				continue;

			JPH::BodyLockWrite lock(_impl->_system->GetBodyLockInterface(), JPH::BodyID(system_physics.body_id));
			if (!lock.Succeeded())
				continue;

			JPH::Body& body = lock.GetBody();
			body.SetCollideKinematicVsNonDynamic(enabled && body.IsSensor());
		}
	}

	span_t<const physics_contact_event_t> physics_world_t::get_contact_events() const
	{
		return {.data = _impl->_contact_events.data(), .size = _impl->_contact_events.size()};
	}

}
