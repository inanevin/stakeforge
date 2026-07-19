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

#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/physical_material.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

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
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace sfg
{
	namespace
	{
#define PHYSICS_OBJECT_LAYER_MOVING_BIT (1 << 6)
#define PHYSICS_BROAD_PHASE_STATIC		0
#define PHYSICS_BROAD_PHASE_MOVING		1

		JPH::Vec3 to_jolt(const vec3f_t& value)
		{
			return {value.x, value.y, value.z};
		}

		JPH::RVec3 to_jolt_position(const vec3f_t& value)
		{
			return {value.x, value.y, value.z};
		}

		JPH::Quat to_jolt(const quat_t& value)
		{
			return {value.x, value.y, value.z, value.w};
		}

		vec3f_t from_jolt(JPH::Vec3Arg value)
		{
			return {value.GetX(), value.GetY(), value.GetZ()};
		}

		quat_t from_jolt(JPH::QuatArg value)
		{
			return {value.GetX(), value.GetY(), value.GetZ(), value.GetW()};
		}

		JPH::ObjectLayer make_object_layer(u8 project_layer, physics_motion_type_e motion_type)
		{
			SFG_ASSERT(project_layer < PHYSICS_COLLISION_LAYER_MAX);
			return static_cast<JPH::ObjectLayer>(project_layer | (motion_type == physics_motion_type_e::static_body ? 0 : PHYSICS_OBJECT_LAYER_MOVING_BIT));
		}

		u8 get_project_layer(JPH::ObjectLayer layer)
		{
			return static_cast<u8>(layer & (PHYSICS_OBJECT_LAYER_MOVING_BIT - 1));
		}

		bool is_moving_layer(JPH::ObjectLayer layer)
		{
			return (layer & PHYSICS_OBJECT_LAYER_MOVING_BIT) != 0;
		}

		JPH::EMotionType to_jolt(physics_motion_type_e motion_type)
		{
			switch (motion_type)
			{
			case physics_motion_type_e::static_body:
				return JPH::EMotionType::Static;
			case physics_motion_type_e::kinematic_body:
				return JPH::EMotionType::Kinematic;
			case physics_motion_type_e::dynamic_body:
				return JPH::EMotionType::Dynamic;
			}

			SFG_ASSERT(false);
			return JPH::EMotionType::Static;
		}

		struct physics_broad_phase_interface_t final : JPH::BroadPhaseLayerInterface
		{
			JPH::uint GetNumBroadPhaseLayers() const override
			{
				return 2;
			}

			JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
			{
				return JPH::BroadPhaseLayer(is_moving_layer(layer) ? PHYSICS_BROAD_PHASE_MOVING : PHYSICS_BROAD_PHASE_STATIC);
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
				if (!is_moving_layer(object_layer))
					return broad_phase_layer.GetValue() == PHYSICS_BROAD_PHASE_MOVING;

				return true;
			}
		};

		struct physics_layer_pair_filter_t final : JPH::ObjectLayerPairFilter
		{
			const physics_runtime_config_t* config = nullptr;

			bool ShouldCollide(JPH::ObjectLayer layer_a, JPH::ObjectLayer layer_b) const override
			{
				if (!is_moving_layer(layer_a) && !is_moving_layer(layer_b))
					return false;

				const u8 project_a = get_project_layer(layer_a);
				const u8 project_b = get_project_layer(layer_b);
				return (config->collision_masks[project_a] & (1ull << project_b)) != 0 && (config->collision_masks[project_b] & (1ull << project_a)) != 0;
			}
		};

		struct query_object_layer_filter_t final : JPH::ObjectLayerFilter
		{
			u64 layers = UINT64_MAX;

			bool ShouldCollide(JPH::ObjectLayer layer) const override
			{
				return (layers & (1ull << get_project_layer(layer))) != 0;
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
		struct body_proxy_t
		{
			component_collider_t   collider			 = {};
			component_rigid_body_t rigid_body		 = {};
			JPH::BodyID			   body_id			 = {};
			vec3f_t				   last_entity_pos	 = vec3f_t::zero;
			quat_t				   last_entity_rot	 = quat_t::identity;
			vec3f_t				   scale			 = vec3f_t::one;
			resource_handle_t	   physical_material = NULL_RESOURCE_HANDLE;
			entity_id_t			   entity			 = NULL_ENTITY_ID;
			u64					   tags				 = 0;
			u32					   generation		 = 1;
			bool				   has_rigid_body	 = false;
			bool				   active			 = false;
			bool				   is_character		 = false;
		};

		struct raw_contact_event_t
		{
			JPH::RVec3			   position	   = JPH::RVec3::sZero();
			JPH::Vec3			   normal	   = JPH::Vec3::sZero();
			u64					   handle_a	   = 0;
			u64					   handle_b	   = 0;
			f32					   penetration = 0.0f;
			physics_contact_type_e type		   = physics_contact_type_e::begin;
			bool				   is_sensor   = false;
		};

		struct character_proxy_t
		{
			JPH::Ref<JPH::CharacterVirtual> character;
			component_character_mover_t		mover				 = {};
			vec3f_t							last_entity_pos		 = vec3f_t::zero;
			quat_t							last_entity_rot		 = quat_t::identity;
			vec3f_t							last_ground_velocity = vec3f_t::zero;
			entity_id_t						entity				 = NULL_ENTITY_ID;
			u32								body_proxy_index	 = UINT32_MAX;
			bool							active				 = false;
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
					.handle_a = impl->get_handle(pair.GetBody1ID()),
					.handle_b = impl->get_handle(pair.GetBody2ID()),
					.type	  = physics_contact_type_e::end,
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
					.handle_a	 = body_a.GetUserData(),
					.handle_b	 = body_b.GetUserData(),
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

		world_t*										 _world			 = nullptr;
		JPH::PhysicsSystem*								 _system		 = nullptr;
		JPH::TempAllocatorImpl*							 _temp_allocator = nullptr;
		physics_runtime_config_t						 _config		 = {};
		physics_broad_phase_interface_t					 _broad_phase_interface;
		physics_object_broad_phase_filter_t				 _object_broad_phase_filter;
		physics_layer_pair_filter_t						 _layer_pair_filter;
		contact_listener_t								 _contact_listener;
		moodycamel::ConcurrentQueue<raw_contact_event_t> _raw_contact_events;
		vector_t<body_proxy_t>							 _bodies;
		vector_t<u32>									 _body_free_list;
		vector_t<u64>									 _body_index_to_handle;
		vector_t<u32>									 _entity_to_proxy;
		vector_t<character_proxy_t>						 _characters;
		vector_t<u32>									 _character_free_list;
		vector_t<u32>									 _entity_to_character;
		vector_t<physics_contact_event_t>				 _contact_events;
		f32												 _accumulator		 = 0.0f;
		bool											 _simulation_enabled = false;

		impl_t() : _raw_contact_events(4096)
		{
		}

		void init(world_t& world, const physics_runtime_config_t& config)
		{
			_world					  = &world;
			_config					  = config;
			_layer_pair_filter.config = &_config;
			_temp_allocator			  = new JPH::TempAllocatorImpl(_config.temp_allocator_bytes);
			_system					  = new JPH::PhysicsSystem();
			_system->Init(_config.max_bodies, _config.body_mutex_count, _config.max_body_pairs, _config.max_contact_constraints, _broad_phase_interface, _object_broad_phase_filter, _layer_pair_filter);
			_system->SetGravity(to_jolt(_config.gravity));
			_contact_listener.impl = this;
			_system->SetContactListener(&_contact_listener);
			_bodies.reserve(_config.body_reserve);
			_body_free_list.reserve(_config.body_reserve);
			_characters.reserve(_config.character_reserve);
			_character_free_list.reserve(_config.character_reserve);
			_body_index_to_handle.resize(_config.max_bodies, 0);
			_entity_to_proxy.resize(ECS_MAX_ENTITIES, UINT32_MAX);
			_entity_to_character.resize(ECS_MAX_ENTITIES, UINT32_MAX);
			_contact_events.reserve(_config.contact_event_reserve);
		}

		void uninit()
		{
			for (u32 i = 0; i < _characters.size(); ++i)
			{
				if (_characters[i].active)
					destroy_character(i);
			}

			for (u32 i = 0; i < _bodies.size(); ++i)
			{
				if (_bodies[i].active)
					destroy_body(i);
			}

			delete _system;
			delete _temp_allocator;
			_system			= nullptr;
			_temp_allocator = nullptr;
			_world			= nullptr;
			_bodies.resize(0);
			_body_free_list.resize(0);
			_body_index_to_handle.resize(0);
			_entity_to_proxy.resize(0);
			_characters.resize(0);
			_character_free_list.resize(0);
			_entity_to_character.resize(0);
			_contact_events.resize(0);
			_accumulator = 0.0f;
		}

		u64 get_handle(const JPH::BodyID& body_id) const
		{
			const u32 index = body_id.GetIndex();
			if (index >= _body_index_to_handle.size())
				return 0;

			return _body_index_to_handle[index];
		}

		body_proxy_t* resolve_handle(u64 handle)
		{
			const u32 index		 = static_cast<u32>(handle);
			const u32 generation = static_cast<u32>(handle >> 32);
			if (index >= _bodies.size())
				return nullptr;

			body_proxy_t& proxy = _bodies[index];
			return proxy.active && proxy.generation == generation ? &proxy : nullptr;
		}

		const body_proxy_t* resolve_handle(u64 handle) const
		{
			return const_cast<impl_t*>(this)->resolve_handle(handle);
		}

		body_proxy_t* resolve_body(const JPH::BodyID& body_id)
		{
			body_proxy_t* proxy = resolve_handle(get_handle(body_id));
			return proxy != nullptr && proxy->body_id == body_id ? proxy : nullptr;
		}

		const body_proxy_t* resolve_body(const JPH::BodyID& body_id) const
		{
			return const_cast<impl_t*>(this)->resolve_body(body_id);
		}

		body_proxy_t* get_body(entity_id_t entity)
		{
			const u32 index = _entity_to_proxy[entity];
			return index != UINT32_MAX ? &_bodies[index] : nullptr;
		}

		const body_proxy_t* get_body(entity_id_t entity) const
		{
			return const_cast<impl_t*>(this)->get_body(entity);
		}

		bool passes_filter(const JPH::BodyID& body_id, const physics_query_filter_t& filter) const
		{
			const body_proxy_t* proxy = resolve_body(body_id);
			if (proxy == nullptr || proxy->entity == filter.ignored_entity)
				return false;
			if (proxy->is_character)
			{
				if ((filter.flags & physics_query_flag_character) == 0)
					return false;
			}
			else
			{
				const physics_motion_type_e motion_type = proxy->has_rigid_body ? proxy->rigid_body.motion_type : physics_motion_type_e::static_body;
				if (motion_type == physics_motion_type_e::static_body && (filter.flags & physics_query_flag_static) == 0)
					return false;
				if (motion_type == physics_motion_type_e::kinematic_body && (filter.flags & physics_query_flag_kinematic) == 0)
					return false;
				if (motion_type == physics_motion_type_e::dynamic_body && (filter.flags & physics_query_flag_dynamic) == 0)
					return false;
			}

			if (proxy->collider.is_sensor != 0 && (filter.flags & physics_query_flag_sensor) == 0)
				return false;
			if (filter.required_any_tags.bits != 0 && (proxy->tags & filter.required_any_tags.bits) == 0)
				return false;
			if ((proxy->tags & filter.required_all_tags.bits) != filter.required_all_tags.bits)
				return false;
			if ((proxy->tags & filter.excluded_tags.bits) != 0)
				return false;

			return true;
		}

		void tick(f32 delta_time)
		{
			_contact_events.resize(0);
			reconcile_bodies();
			reconcile_characters();

			if (!_simulation_enabled)
			{
				sync_bodies_to_physics(delta_time);
				sync_characters_to_physics();
				return;
			}

			const f32 fixed_delta = 1.0f / static_cast<f32>(_config.physics_rate);
			_accumulator += delta_time;
			u32 steps = 0;
			while (_accumulator >= fixed_delta && steps < _config.max_sub_steps)
			{
				sync_bodies_to_physics(fixed_delta);
				sync_characters_to_physics();
				update_characters(fixed_delta);
				_system->Update(fixed_delta, 1, _temp_allocator, &physics_runtime_t::get_job_system());
				sync_dynamic_bodies_to_world();
				drain_contact_events();
				_accumulator -= fixed_delta;
				steps++;
			}

			if (steps == _config.max_sub_steps && _accumulator >= fixed_delta)
				_accumulator = 0.0f;
		}

		void reconcile_bodies()
		{
			ecs_component_table_t&		 collider_table	  = _world->get_component_table(type_id_t<component_collider_t>::value);
			ecs_component_table_t&		 rigid_body_table = _world->get_component_table(type_id_t<component_rigid_body_t>::value);
			ecs_component_table_t&		 tags_table		  = _world->get_component_table(type_id_t<component_entity_tags_t>::value);
			const ecs_component_table_t& disabled_table	  = _world->get_component_table(type_id_t<component_disabled_t>::value);
			const ecs_component_table_t& transform_table  = _world->get_component_table(type_id_t<component_system_transform_t>::value);

			for (u32 i = 0; i < _bodies.size(); ++i)
			{
				body_proxy_t& proxy = _bodies[i];
				if (!proxy.active || proxy.is_character)
					continue;

				if (!_world->is_alive(proxy.entity) || !ecs_t::table_has(collider_table, proxy.entity) || ecs_t::table_has(disabled_table, proxy.entity))
				{
					destroy_body(i);
					continue;
				}

				const component_collider_t&	  collider	 = ecs_helpers_t::table_get_as_const<component_collider_t>(collider_table, proxy.entity);
				const component_rigid_body_t* rigid_body = ecs_helpers_t::table_find_as_const<component_rigid_body_t>(rigid_body_table, proxy.entity);
				_world->calculate_transform_direct(proxy.entity);
				const component_system_transform_t& transform	   = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, proxy.entity);
				const bool							has_rigid_body = rigid_body != nullptr;
				if (proxy.collider != collider || proxy.has_rigid_body != has_rigid_body || (has_rigid_body && proxy.rigid_body != *rigid_body) || proxy.scale != transform.abs_scale)
				{
					destroy_body(i);
					continue;
				}

				const component_entity_tags_t* tags = ecs_helpers_t::table_find_as_const<component_entity_tags_t>(tags_table, proxy.entity);
				proxy.tags							= tags != nullptr ? tags->tags : 0;
			}

			const ecs_component_table_ref_t refs[] = {collider_table.ref()};
			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = 1}))
			{
				if (_entity_to_proxy[row.id] != UINT32_MAX || ecs_t::table_has(disabled_table, row.id))
					continue;

				const component_collider_t&	   collider	  = ecs_helpers_t::row_get<component_collider_t>(row);
				const component_rigid_body_t*  rigid_body = ecs_helpers_t::table_find_as_const<component_rigid_body_t>(rigid_body_table, row.id);
				const component_entity_tags_t* tags		  = ecs_helpers_t::table_find_as_const<component_entity_tags_t>(tags_table, row.id);
				create_body(row.id, collider, rigid_body, tags != nullptr ? tags->tags : 0);
			}
		}

		void reconcile_characters()
		{
			ecs_component_table_t&		 mover_table	= _world->get_component_table(type_id_t<component_character_mover_t>::value);
			ecs_component_table_t&		 tags_table		= _world->get_component_table(type_id_t<component_entity_tags_t>::value);
			const ecs_component_table_t& disabled_table = _world->get_component_table(type_id_t<component_disabled_t>::value);

			for (u32 i = 0; i < _characters.size(); ++i)
			{
				character_proxy_t& proxy = _characters[i];
				if (!proxy.active)
					continue;

				if (!_world->is_alive(proxy.entity) || !ecs_t::table_has(mover_table, proxy.entity) || ecs_t::table_has(disabled_table, proxy.entity))
				{
					destroy_character(i);
					continue;
				}

				const component_character_mover_t& mover = ecs_helpers_t::table_get_as_const<component_character_mover_t>(mover_table, proxy.entity);
				if (proxy.mover != mover)
				{
					destroy_character(i);
					continue;
				}

				const component_entity_tags_t* tags	 = ecs_helpers_t::table_find_as_const<component_entity_tags_t>(tags_table, proxy.entity);
				_bodies[proxy.body_proxy_index].tags = tags != nullptr ? tags->tags : 0;
			}

			const ecs_component_table_ref_t refs[] = {mover_table.ref()};
			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = 1}))
			{
				if (_entity_to_character[row.id] != UINT32_MAX || ecs_t::table_has(disabled_table, row.id))
					continue;

				const component_character_mover_t& mover = ecs_helpers_t::row_get<component_character_mover_t>(row);
				const component_entity_tags_t*	   tags	 = ecs_helpers_t::table_find_as_const<component_entity_tags_t>(tags_table, row.id);
				create_character(row.id, mover, tags != nullptr ? tags->tags : 0);
			}
		}

		u32 allocate_proxy()
		{
			if (!_body_free_list.empty())
			{
				const u32 index = _body_free_list.back();
				_body_free_list.pop_back();
				return index;
			}

			_bodies.emplace_back();
			return static_cast<u32>(_bodies.size() - 1);
		}

		u32 allocate_character_proxy()
		{
			if (!_character_free_list.empty())
			{
				const u32 index = _character_free_list.back();
				_character_free_list.pop_back();
				return index;
			}

			_characters.emplace_back();
			return static_cast<u32>(_characters.size() - 1);
		}

		void create_character(entity_id_t entity, const component_character_mover_t& mover, u64 tags)
		{
			_world->calculate_transform_direct(entity);
			const ecs_component_table_t&		transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			const component_system_transform_t& transform		= ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);

			JPH::CapsuleShapeSettings		shape_settings(math::max(mover.half_height, 0.001f), math::max(mover.radius, 0.001f));
			JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
			if (shape_result.HasError())
			{
				SFG_ERR("failed to create character shape for entity {0}: {1}", entity, shape_result.GetError().c_str());
				return;
			}

			const u32	  body_proxy_index		= allocate_proxy();
			body_proxy_t& body_proxy			= _bodies[body_proxy_index];
			body_proxy.active					= true;
			body_proxy.entity					= entity;
			body_proxy.tags						= tags;
			body_proxy.is_character				= true;
			body_proxy.collider.collision_layer = mover.collision_layer;
			body_proxy.rigid_body.motion_type	= physics_motion_type_e::kinematic_body;
			body_proxy.has_rigid_body			= true;
			const u64 handle					= (static_cast<u64>(body_proxy.generation) << 32) | body_proxy_index;

			JPH::CharacterVirtualSettings settings;
			settings.mMaxSlopeAngle				  = math::degrees_to_radians(mover.max_slope_degrees);
			settings.mMaxStrength				  = mover.max_strength;
			settings.mMass						  = mover.mass;
			settings.mShape						  = shape_result.Get();
			settings.mShapeOffset				  = to_jolt(mover.shape_offset);
			settings.mCharacterPadding			  = mover.padding;
			settings.mPredictiveContactDistance	  = mover.predictive_contact_distance;
			settings.mPenetrationRecoverySpeed	  = mover.penetration_recovery_speed;
			settings.mEnhancedInternalEdgeRemoval = mover.enhanced_internal_edge_removal != 0;
			settings.mSupportingVolume			  = JPH::Plane(JPH::Vec3::sAxisY(), -mover.radius);
			settings.mInnerBodyShape			  = shape_result.Get();
			settings.mInnerBodyLayer			  = make_object_layer(mover.collision_layer, physics_motion_type_e::kinematic_body);

			const u32		   character_index = allocate_character_proxy();
			character_proxy_t& proxy		   = _characters[character_index];
			proxy.character					   = new JPH::CharacterVirtual(&settings, to_jolt_position(transform.abs_pos), to_jolt(transform.abs_rot), handle, _system);
			proxy.mover						   = mover;
			proxy.last_entity_pos			   = transform.abs_pos;
			proxy.last_entity_rot			   = transform.abs_rot;
			proxy.entity					   = entity;
			proxy.body_proxy_index			   = body_proxy_index;
			proxy.active					   = true;

			body_proxy.body_id									 = proxy.character->GetInnerBodyID();
			_body_index_to_handle[body_proxy.body_id.GetIndex()] = handle;
			_entity_to_character[entity]						 = character_index;
		}

		void destroy_character(u32 character_index)
		{
			character_proxy_t& proxy = _characters[character_index];
			SFG_ASSERT(proxy.active);
			body_proxy_t& body_proxy							 = _bodies[proxy.body_proxy_index];
			_body_index_to_handle[body_proxy.body_id.GetIndex()] = 0;
			proxy.character										 = nullptr;
			body_proxy.active									 = false;
			body_proxy.entity									 = NULL_ENTITY_ID;
			body_proxy.generation++;
			_body_free_list.push_back(proxy.body_proxy_index);
			_entity_to_character[proxy.entity] = UINT32_MAX;
			proxy.active					   = false;
			proxy.entity					   = NULL_ENTITY_ID;
			proxy.body_proxy_index			   = UINT32_MAX;
			_character_free_list.push_back(character_index);
		}

		void create_body(entity_id_t entity, const component_collider_t& collider, const component_rigid_body_t* rigid_body, u64 tags)
		{
			const u32	  proxy_index = allocate_proxy();
			body_proxy_t& proxy		  = _bodies[proxy_index];
			proxy.active			  = true;
			proxy.entity			  = entity;
			proxy.collider			  = collider;
			proxy.has_rigid_body	  = rigid_body != nullptr;
			proxy.rigid_body		  = rigid_body != nullptr ? *rigid_body : component_rigid_body_t{.motion_type = physics_motion_type_e::static_body};
			proxy.tags				  = tags;
			proxy.physical_material	  = collider.physical_material;

			_world->calculate_transform_direct(entity);
			const ecs_component_table_t&		transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			const component_system_transform_t& transform		= ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);
			proxy.scale											= transform.abs_scale;

			const vec3f_t			  abs_scale = vec3f_t::abs(transform.abs_scale);
			JPH::RefConst<JPH::Shape> shape;
			switch (collider.shape)
			{
			case physics_shape_type_e::box: {
				JPH::BoxShapeSettings settings(to_jolt(vec3f_t::max(collider.half_extent * abs_scale, {0.001f, 0.001f, 0.001f})));
				shape = settings.Create().Get();
				break;
			}
			case physics_shape_type_e::sphere: {
				const f32				 radius = collider.radius * math::max(abs_scale.x, math::max(abs_scale.y, abs_scale.z));
				JPH::SphereShapeSettings settings(math::max(radius, 0.001f));
				shape = settings.Create().Get();
				break;
			}
			case physics_shape_type_e::capsule: {
				const f32				  radius = collider.radius * math::max(abs_scale.x, abs_scale.z);
				JPH::CapsuleShapeSettings settings(math::max(collider.half_height * abs_scale.y, 0.001f), math::max(radius, 0.001f));
				shape = settings.Create().Get();
				break;
			}
			case physics_shape_type_e::cylinder: {
				const f32				   radius = collider.radius * math::max(abs_scale.x, abs_scale.z);
				JPH::CylinderShapeSettings settings(math::max(collider.half_height * abs_scale.y, 0.001f), math::max(radius, 0.001f));
				shape = settings.Create().Get();
				break;
			}
			}

			if (shape == nullptr)
			{
				SFG_ERR("failed to create physics shape for entity {0}", entity);
				proxy.active = false;
				proxy.generation++;
				_body_free_list.push_back(proxy_index);
				return;
			}

			const physics_motion_type_e motion_type	  = rigid_body != nullptr ? rigid_body->motion_type : physics_motion_type_e::static_body;
			const quat_t				body_rotation = transform.abs_rot * collider.local_rotation;
			const vec3f_t				body_position = transform.abs_pos + transform.abs_rot * (collider.local_position * transform.abs_scale);
			JPH::BodyCreationSettings	settings(shape, to_jolt_position(body_position), to_jolt(body_rotation), to_jolt(motion_type), make_object_layer(collider.collision_layer, motion_type));
			settings.mIsSensor = collider.is_sensor != 0;
			settings.mUserData = (static_cast<u64>(proxy.generation) << 32) | proxy_index;

			if (rigid_body != nullptr)
			{
				settings.mLinearDamping	 = rigid_body->linear_damping;
				settings.mAngularDamping = rigid_body->angular_damping;
				settings.mGravityFactor	 = rigid_body->gravity_factor;
				settings.mAllowSleeping	 = rigid_body->allow_sleep != 0;
				settings.mMotionQuality	 = rigid_body->motion_quality_continuous != 0 ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
				if (motion_type == physics_motion_type_e::dynamic_body)
				{
					settings.mOverrideMassProperties	   = JPH::EOverrideMassProperties::CalculateInertia;
					settings.mMassPropertiesOverride.mMass = rigid_body->mass;
				}
			}

			if (collider.physical_material != NULL_RESOURCE_HANDLE)
			{
				const physical_material_runtime_t* material = resource_manager_t::get().find_runtime<physical_material_runtime_t>(collider.physical_material);
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
				SFG_ERR("failed to allocate physics body for entity {0}", entity);
				proxy.active = false;
				proxy.generation++;
				_body_free_list.push_back(proxy_index);
				return;
			}

			proxy.body_id									= body->GetID();
			proxy.last_entity_pos							= transform.abs_pos;
			proxy.last_entity_rot							= transform.abs_rot;
			_body_index_to_handle[proxy.body_id.GetIndex()] = settings.mUserData;
			_entity_to_proxy[entity]						= proxy_index;
			body_interface.AddBody(proxy.body_id, motion_type == physics_motion_type_e::static_body ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
		}

		void destroy_body(u32 proxy_index)
		{
			body_proxy_t& proxy = _bodies[proxy_index];
			SFG_ASSERT(proxy.active);
			JPH::BodyInterface& body_interface = _system->GetBodyInterface();
			body_interface.RemoveBody(proxy.body_id);
			body_interface.DestroyBody(proxy.body_id);
			_body_index_to_handle[proxy.body_id.GetIndex()] = 0;
			if (proxy.entity < _entity_to_proxy.size() && _entity_to_proxy[proxy.entity] == proxy_index)
				_entity_to_proxy[proxy.entity] = UINT32_MAX;
			proxy.active = false;
			proxy.entity = NULL_ENTITY_ID;
			proxy.generation++;
			_body_free_list.push_back(proxy_index);
		}

		void sync_bodies_to_physics(f32 delta_time)
		{
			const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			JPH::BodyInterface&			 body_interface	 = _system->GetBodyInterface();
			for (body_proxy_t& proxy : _bodies)
			{
				if (!proxy.active || proxy.is_character)
					continue;

				_world->calculate_transform_direct(proxy.entity);
				const component_system_transform_t& transform	  = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, proxy.entity);
				const quat_t						body_rotation = transform.abs_rot * proxy.collider.local_rotation;
				const vec3f_t						body_position = transform.abs_pos + transform.abs_rot * (proxy.collider.local_position * transform.abs_scale);
				const physics_motion_type_e			motion_type	  = proxy.has_rigid_body ? proxy.rigid_body.motion_type : physics_motion_type_e::static_body;

				if (motion_type == physics_motion_type_e::kinematic_body)
				{
					body_interface.MoveKinematic(proxy.body_id, to_jolt_position(body_position), to_jolt(body_rotation), delta_time);
				}
				else if (transform.abs_pos != proxy.last_entity_pos || transform.abs_rot != proxy.last_entity_rot)
				{
					body_interface.SetPositionAndRotation(proxy.body_id, to_jolt_position(body_position), to_jolt(body_rotation), motion_type == physics_motion_type_e::dynamic_body ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
				}

				proxy.last_entity_pos = transform.abs_pos;
				proxy.last_entity_rot = transform.abs_rot;
			}
		}

		void sync_characters_to_physics()
		{
			const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
			for (character_proxy_t& proxy : _characters)
			{
				if (!proxy.active)
					continue;

				_world->calculate_transform_direct(proxy.entity);
				const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, proxy.entity);
				if (transform.abs_pos != proxy.last_entity_pos)
					proxy.character->SetPosition(to_jolt_position(transform.abs_pos));
				if (transform.abs_rot != proxy.last_entity_rot)
					proxy.character->SetRotation(to_jolt(transform.abs_rot));

				proxy.last_entity_pos = transform.abs_pos;
				proxy.last_entity_rot = transform.abs_rot;
			}
		}

		void update_characters(f32 delta_time)
		{
			for (character_proxy_t& proxy : _characters)
			{
				if (!proxy.active)
					continue;

				JPH::CharacterVirtual& character = *proxy.character;
				character.UpdateGroundVelocity();
				const JPH::Vec3 ground_velocity = character.GetGroundVelocity();
				JPH::Vec3		velocity		= character.GetLinearVelocity();
				if (character.GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround)
				{
					const f32 vertical_velocity		   = velocity.Dot(JPH::Vec3::sAxisY());
					const f32 ground_vertical_velocity = ground_velocity.Dot(JPH::Vec3::sAxisY());
					if (vertical_velocity - ground_vertical_velocity < 0.1f)
					{
						const JPH::Vec3 previous_ground = to_jolt(proxy.last_ground_velocity);
						velocity						= velocity - previous_ground + ground_velocity;
						velocity -= velocity.Dot(JPH::Vec3::sAxisY()) * JPH::Vec3::sAxisY();
						velocity += ground_vertical_velocity * JPH::Vec3::sAxisY();
					}
				}

				velocity += to_jolt(_config.gravity) * delta_time;
				character.SetLinearVelocity(velocity);
				proxy.last_ground_velocity = from_jolt(ground_velocity);

				query_broad_phase_filter_t broad_filter;
				broad_filter.flags = physics_query_flag_all;
				query_object_layer_filter_t layer_filter;
				layer_filter.layers = _config.collision_masks[proxy.mover.collision_layer];
				const physics_query_filter_t query_filter{
					.collision_layers = {.bits = layer_filter.layers},
					.ignored_entity	  = proxy.entity,
				};
				query_body_filter_t body_filter;
				body_filter.impl   = this;
				body_filter.filter = &query_filter;

				JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
				update_settings.mStickToFloorStepDown	   = JPH::Vec3(0.0f, -proxy.mover.step_down, 0.0f);
				update_settings.mWalkStairsStepUp		   = JPH::Vec3(0.0f, proxy.mover.step_up, 0.0f);
				update_settings.mWalkStairsMinStepForward  = proxy.mover.min_step_forward;
				update_settings.mWalkStairsStepForwardTest = proxy.mover.step_forward_test;
				character.ExtendedUpdate(delta_time, to_jolt(_config.gravity), update_settings, broad_filter, layer_filter, body_filter, {}, *_temp_allocator);

				const vec3f_t position = from_jolt(character.GetPosition());
				const quat_t  rotation = from_jolt(character.GetRotation());
				_world->set_entity_pos_local(proxy.entity, _world->abs_pos_to_local(proxy.entity, position));
				_world->set_entity_rot_local(proxy.entity, _world->abs_rot_to_local(proxy.entity, rotation));
				proxy.last_entity_pos = position;
				proxy.last_entity_rot = rotation;
			}
		}

		character_proxy_t* get_character(entity_id_t entity)
		{
			const u32 index = _entity_to_character[entity];
			return index != UINT32_MAX ? &_characters[index] : nullptr;
		}

		const character_proxy_t* get_character(entity_id_t entity) const
		{
			return const_cast<impl_t*>(this)->get_character(entity);
		}

		void sync_dynamic_bodies_to_world()
		{
			JPH::BodyInterface& body_interface = _system->GetBodyInterface();
			for (body_proxy_t& proxy : _bodies)
			{
				if (!proxy.active || !proxy.has_rigid_body || proxy.rigid_body.motion_type != physics_motion_type_e::dynamic_body || proxy.is_character)
					continue;

				JPH::RVec3 body_position;
				JPH::Quat  body_rotation;
				body_interface.GetPositionAndRotation(proxy.body_id, body_position, body_rotation);
				const quat_t  entity_rotation = from_jolt(body_rotation) * proxy.collider.local_rotation.inverse();
				const vec3f_t entity_position = from_jolt(body_position) - entity_rotation * (proxy.collider.local_position * proxy.scale);
				_world->set_entity_pos_local(proxy.entity, _world->abs_pos_to_local(proxy.entity, entity_position));
				_world->set_entity_rot_local(proxy.entity, _world->abs_rot_to_local(proxy.entity, entity_rotation));
				proxy.last_entity_pos = entity_position;
				proxy.last_entity_rot = entity_rotation;
			}
		}

		void drain_contact_events()
		{
			raw_contact_event_t raw;
			while (_raw_contact_events.try_dequeue(raw))
			{
				const body_proxy_t* proxy_a = resolve_handle(raw.handle_a);
				const body_proxy_t* proxy_b = resolve_handle(raw.handle_b);
				if (proxy_a == nullptr || proxy_b == nullptr)
					continue;

				_contact_events.push_back({
					.position	 = from_jolt(raw.position),
					.normal		 = from_jolt(raw.normal),
					.entity_a	 = proxy_a->entity,
					.entity_b	 = proxy_b->entity,
					.penetration = raw.penetration,
					.type		 = raw.type,
					.is_sensor	 = raw.is_sensor,
				});
			}
		}

		bool validate_ray(const physics_raycast_t& ray, JPH::RRayCast& out_ray) const
		{
			if (ray.distance <= 0.0f || ray.direction.is_zero())
				return false;

			const vec3f_t delta = ray.direction.normalized() * ray.distance;
			out_ray				= JPH::RRayCast(to_jolt_position(ray.origin), to_jolt(delta));
			return true;
		}

		void fill_hit(const JPH::RayCastResult& result, const physics_raycast_t& ray, physics_hit_t& hit)
		{
			const body_proxy_t* proxy = resolve_body(result.mBodyID);
			SFG_ASSERT(proxy != nullptr);
			const vec3f_t delta	  = ray.direction.normalized() * ray.distance;
			hit.position		  = ray.origin + delta * result.mFraction;
			hit.distance		  = ray.distance * result.mFraction;
			hit.fraction		  = result.mFraction;
			hit.entity			  = proxy->entity;
			hit.physical_material = proxy->physical_material;
			hit.sub_shape_id	  = result.mSubShapeID2.GetValue();
			hit.is_sensor		  = proxy->collider.is_sensor != 0;
			hit.is_character	  = proxy->is_character;

			JPH::BodyLockRead lock(_system->GetBodyLockInterface(), result.mBodyID);
			if (lock.Succeeded())
				hit.normal = from_jolt(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, to_jolt_position(hit.position)));
		}

		bool raycast_closest(const physics_raycast_t& ray, physics_hit_t& out_hit, const physics_query_filter_t& filter)
		{
			JPH::RRayCast jolt_ray;
			if (!validate_ray(ray, jolt_ray))
				return false;

			query_broad_phase_filter_t broad_filter;
			broad_filter.flags = filter.flags;
			query_object_layer_filter_t layer_filter;
			layer_filter.layers = filter.collision_layers.bits;
			query_body_filter_t body_filter;
			body_filter.impl   = this;
			body_filter.filter = &filter;
			JPH::RayCastResult result;
			if (!_system->GetNarrowPhaseQuery().CastRay(jolt_ray, result, broad_filter, layer_filter, body_filter))
				return false;

			fill_hit(result, ray, out_hit);
			return true;
		}

		bool raycast_any(const physics_raycast_t& ray, const physics_query_filter_t& filter)
		{
			JPH::RRayCast jolt_ray;
			if (!validate_ray(ray, jolt_ray))
				return false;

			query_broad_phase_filter_t broad_filter;
			broad_filter.flags = filter.flags;
			query_object_layer_filter_t layer_filter;
			layer_filter.layers = filter.collision_layers.bits;
			query_body_filter_t body_filter;
			body_filter.impl   = this;
			body_filter.filter = &filter;
			JPH::AnyHitCollisionCollector<JPH::CastRayCollector> collector;
			JPH::RayCastSettings								 settings;
			_system->GetNarrowPhaseQuery().CastRay(jolt_ray, settings, collector, broad_filter, layer_filter, body_filter);
			return collector.HadHit();
		}

		physics_query_result_t raycast_all(const physics_raycast_t& ray, span_t<physics_hit_t> out_hits, const physics_query_filter_t& filter)
		{
			JPH::RRayCast jolt_ray;
			if (!validate_ray(ray, jolt_ray))
				return {};

			query_broad_phase_filter_t broad_filter;
			broad_filter.flags = filter.flags;
			query_object_layer_filter_t layer_filter;
			layer_filter.layers = filter.collision_layers.bits;
			query_body_filter_t body_filter;
			body_filter.impl   = this;
			body_filter.filter = &filter;
			ray_all_collector_t collector;
			collector.impl	   = this;
			collector.hits	   = out_hits.data;
			collector.ray	   = &ray;
			collector.capacity = static_cast<u32>(out_hits.size);
			JPH::RayCastSettings settings;
			_system->GetNarrowPhaseQuery().CastRay(jolt_ray, settings, collector, broad_filter, layer_filter, body_filter);

			if (collector.count > 1)
				std::sort(out_hits.data, out_hits.data + collector.count, [](const physics_hit_t& lhs, const physics_hit_t& rhs) { return lhs.fraction < rhs.fraction; });

			return {.hit_count = collector.count, .overflow = collector.overflow};
		}
	}

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

	void physics_world_t::tick(f32 delta_time)
	{
		_impl->tick(delta_time);
	}

	void physics_world_t::set_body_linear_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		impl_t::body_proxy_t* body = _impl->get_body(entity);
		SFG_ASSERT(body != nullptr && !body->is_character && body->has_rigid_body && body->rigid_body.motion_type != physics_motion_type_e::static_body);
		_impl->_system->GetBodyInterface().SetLinearVelocity(body->body_id, to_jolt(velocity));
	}

	void physics_world_t::set_body_angular_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		impl_t::body_proxy_t* body = _impl->get_body(entity);
		SFG_ASSERT(body != nullptr && !body->is_character && body->has_rigid_body && body->rigid_body.motion_type != physics_motion_type_e::static_body);
		_impl->_system->GetBodyInterface().SetAngularVelocity(body->body_id, to_jolt(velocity));
	}

	void physics_world_t::add_body_force(entity_id_t entity, const vec3f_t& force)
	{
		impl_t::body_proxy_t* body = _impl->get_body(entity);
		SFG_ASSERT(body != nullptr && !body->is_character && body->has_rigid_body && body->rigid_body.motion_type == physics_motion_type_e::dynamic_body);
		_impl->_system->GetBodyInterface().AddForce(body->body_id, to_jolt(force));
	}

	void physics_world_t::add_body_impulse(entity_id_t entity, const vec3f_t& impulse)
	{
		impl_t::body_proxy_t* body = _impl->get_body(entity);
		SFG_ASSERT(body != nullptr && !body->is_character && body->has_rigid_body && body->rigid_body.motion_type == physics_motion_type_e::dynamic_body);
		_impl->_system->GetBodyInterface().AddImpulse(body->body_id, to_jolt(impulse));
	}

	void physics_world_t::wake_body(entity_id_t entity)
	{
		impl_t::body_proxy_t* body = _impl->get_body(entity);
		SFG_ASSERT(body != nullptr && !body->is_character && body->has_rigid_body && body->rigid_body.motion_type != physics_motion_type_e::static_body);
		_impl->_system->GetBodyInterface().ActivateBody(body->body_id);
	}

	bool physics_world_t::get_body_state(entity_id_t entity, physics_body_state_t& out_state) const
	{
		const impl_t::body_proxy_t* body = _impl->get_body(entity);
		if (body == nullptr || body->is_character)
			return false;

		JPH::RVec3			position;
		JPH::Quat			rotation;
		JPH::BodyInterface& body_interface = _impl->_system->GetBodyInterface();
		body_interface.GetPositionAndRotation(body->body_id, position, rotation);
		out_state.position		   = from_jolt(position);
		out_state.rotation		   = from_jolt(rotation);
		out_state.linear_velocity  = from_jolt(body_interface.GetLinearVelocity(body->body_id));
		out_state.angular_velocity = from_jolt(body_interface.GetAngularVelocity(body->body_id));
		out_state.is_active		   = body_interface.IsActive(body->body_id);
		return true;
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

	void physics_world_t::set_character_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		impl_t::character_proxy_t* character = _impl->get_character(entity);
		SFG_ASSERT(character != nullptr);
		character->character->SetLinearVelocity(to_jolt(velocity));
	}

	void physics_world_t::add_character_velocity(entity_id_t entity, const vec3f_t& velocity)
	{
		impl_t::character_proxy_t* character = _impl->get_character(entity);
		SFG_ASSERT(character != nullptr);
		character->character->SetLinearVelocity(character->character->GetLinearVelocity() + to_jolt(velocity));
	}

	void physics_world_t::jump_character(entity_id_t entity, f32 speed)
	{
		impl_t::character_proxy_t* character = _impl->get_character(entity);
		SFG_ASSERT(character != nullptr);
		if (character->character->GetGroundState() != JPH::CharacterVirtual::EGroundState::OnGround)
			return;

		JPH::Vec3 velocity = character->character->GetLinearVelocity();
		velocity -= velocity.Dot(JPH::Vec3::sAxisY()) * JPH::Vec3::sAxisY();
		velocity += speed * JPH::Vec3::sAxisY();
		character->character->SetLinearVelocity(velocity);
	}

	void physics_world_t::teleport_character(entity_id_t entity, const vec3f_t& position)
	{
		impl_t::character_proxy_t* character = _impl->get_character(entity);
		SFG_ASSERT(character != nullptr);
		character->character->SetPosition(to_jolt_position(position));
		character->last_entity_pos = position;
		_impl->_world->set_entity_pos_local(entity, _impl->_world->abs_pos_to_local(entity, position));
	}

	bool physics_world_t::get_character_state(entity_id_t entity, character_mover_state_t& out_state) const
	{
		const impl_t::character_proxy_t* character = _impl->get_character(entity);
		if (character == nullptr)
			return false;

		const JPH::CharacterVirtual& jolt_character = *character->character;
		out_state.velocity							= from_jolt(jolt_character.GetLinearVelocity());
		out_state.ground_normal						= from_jolt(jolt_character.GetGroundNormal());
		out_state.ground_velocity					= from_jolt(jolt_character.GetGroundVelocity());
		out_state.is_grounded						= jolt_character.GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
		const impl_t::body_proxy_t* ground			= _impl->resolve_body(jolt_character.GetGroundBodyID());
		out_state.ground_entity						= ground != nullptr ? ground->entity : NULL_ENTITY_ID;
		return true;
	}

	void physics_world_t::set_simulation_enabled(bool enabled)
	{
		_impl->_simulation_enabled = enabled;
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

	span_t<const physics_contact_event_t> physics_world_t::get_contact_events() const
	{
		return {.data = _impl->_contact_events.data(), .size = _impl->_contact_events.size()};
	}

	bool physics_world_t::is_simulation_enabled() const
	{
		return _impl->_simulation_enabled;
	}
}
