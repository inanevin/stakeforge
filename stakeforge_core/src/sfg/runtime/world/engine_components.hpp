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

#include <sfg/common/type_id.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/physics/physics_types.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct component_hierarchy_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_hierarchy";

		entity_id_t first_child	 = NULL_ENTITY_ID;
		entity_id_t parent		 = NULL_ENTITY_ID;
		entity_id_t next_sibling = NULL_ENTITY_ID;
		entity_id_t prev_sibling = NULL_ENTITY_ID;
	};

	SFG_DEFINE_TYPE_ID(component_hierarchy_t);

	struct component_guid_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_guid";

		entity_guid_t guid = NULL_ENTITY_GUID;
	};

	SFG_DEFINE_TYPE_ID(component_guid_t);

	struct component_transform_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_transform";

		vec3f_t pos	  = vec3f_t::zero;
		quat_t	rot	  = {};
		vec3f_t scale = vec3f_t::one;
	};

	SFG_DEFINE_TYPE_ID(component_transform_t);

	struct component_name_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_name";

		char text[64];
	};

	SFG_DEFINE_TYPE_ID(component_name_t);

	struct component_mesh_renderer_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_mesh_renderer";

		inplace_vector_t<resource_handle_t, 16> materials = {};
		resource_handle_t						mesh	  = NULL_RESOURCE_HANDLE;
	};

	SFG_DEFINE_TYPE_ID(component_mesh_renderer_t);

	struct component_camera_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_camera";

		f32 fov_degrees = 60.0f;
		f32 near_plane	= 0.1f;
		f32 far_plane	= 1000.0f;
		i8	priority	= 0;
	};

	SFG_DEFINE_TYPE_ID(component_camera_t);

	enum class light_type_e : u8
	{
		directional,
		point,
		spot,
		area,
	};

	SFG_DEFINE_TYPE_ID(light_type_e);

	struct component_light_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_light";

		color_t		 color				  = color_t::white;
		vec2f_t		 area_size			  = {1.0f, 1.0f};
		f32			 intensity			  = 1.0f;
		f32			 range				  = 10.0f;
		f32			 inner_cone_degrees	  = 30.0f;
		f32			 outer_cone_degrees	  = 45.0f;
		f32			 shadow_near_plane	  = 0.1f;
		f32			 shadow_bias		  = 0.001f;
		f32			 shadow_normal_bias	  = 0.01f;
		u16			 shadow_resolution	  = 1024;
		light_type_e type				  = light_type_e::point;
		u8			 shadow_cascade_count = 4;
		u8			 cast_shadows		  = 0;
		u8			 two_sided			  = 0;
	};

	SFG_DEFINE_TYPE_ID(component_light_t);

	enum class tonemap_mode_e : u8
	{
		aces,
		reinhard,
		none,
	};

	SFG_DEFINE_TYPE_ID(tonemap_mode_e);

	struct post_process_ssao_t
	{
		f32 radius_world			 = 0.75f;
		f32 bias					 = 0.04f;
		f32 intensity				 = 1.25f;
		f32 power					 = 1.25f;
		f32 random_rotation_strength = 1.5f;
		u32 direction_count			 = 8;
		u32 step_count				 = 6;
		u8	enabled					 = 1;
	};

	SFG_DEFINE_TYPE_ID(post_process_ssao_t);

	struct post_process_bloom_t
	{
		f32 strength	  = 0.025f;
		f32 filter_radius = 0.012f;
		u8	enabled		  = 1;
	};

	SFG_DEFINE_TYPE_ID(post_process_bloom_t);

	struct component_post_process_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_post_process";

		post_process_ssao_t	 ssao				  = {};
		post_process_bloom_t bloom				  = {};
		f32					 exposure_ev		  = 0.0f;
		f32					 saturation			  = 1.0f;
		f32					 temperature		  = 0.0f;
		f32					 tint				  = 0.0f;
		f32					 reinhard_white_point = 6.0f;
		tonemap_mode_e		 tonemap_mode		  = tonemap_mode_e::reinhard;
	};

	SFG_DEFINE_TYPE_ID(component_post_process_t);

	struct component_skybox_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_skybox";

		resource_handle_t skybox_asset = NULL_RESOURCE_HANDLE;
		f32				  intensity	   = 1.0f;
		f32				  exposure	   = 1.0f;
	};

	SFG_DEFINE_TYPE_ID(component_skybox_t);

	struct component_prefab_reference_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_prefab_reference";

		resource_handle_t prefab  = NULL_RESOURCE_HANDLE;
		bool			  is_root = false;
	};

	SFG_DEFINE_TYPE_ID(component_prefab_reference_t);

	struct component_entity_tags_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_entity_tags";

		u64 tags = 0;

		bool operator==(const component_entity_tags_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_entity_tags_t);

	struct component_physical_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_physical";

		vec3f_t				  local_position			= vec3f_t::zero;
		quat_t				  local_rotation			= quat_t::identity;
		vec3f_t				  half_extent				= {0.5f, 0.5f, 0.5f};
		resource_handle_t	  physical_material			= NULL_RESOURCE_HANDLE;
		resource_handle_t	  collision_mesh			= NULL_RESOURCE_HANDLE;
		f32					  mass						= 1.0f;
		f32					  gravity_factor			= 1.0f;
		f32					  linear_damping			= 0.05f;
		f32					  angular_damping			= 0.05f;
		f32					  radius					= 0.5f;
		f32					  half_height				= 0.5f;
		physics_shape_type_e  shape						= physics_shape_type_e::box;
		physics_motion_type_e motion_type				= physics_motion_type_e::static_body;
		u8					  collision_layer			= 0;
		u8					  is_sensor					= 0;
		u8					  motion_quality_continuous = 0;
		u8					  allow_sleep				= 1;

		bool operator==(const component_physical_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_physical_t);

	struct component_compound_shape_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_compound_shape";

		vec3f_t				 local_position = vec3f_t::zero;
		quat_t				 local_rotation = quat_t::identity;
		vec3f_t				 half_extent	= {0.5f, 0.5f, 0.5f};
		resource_handle_t	 collision_mesh = NULL_RESOURCE_HANDLE;
		f32					 radius			= 0.5f;
		f32					 half_height	= 0.5f;
		physics_shape_type_e shape			= physics_shape_type_e::box;

		bool operator==(const component_compound_shape_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_compound_shape_t);

	struct component_fixed_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_fixed_constraint";

		quat_t		  local_rotation  = quat_t::identity;
		quat_t		  target_rotation = quat_t::identity;
		vec3f_t		  local_point	  = vec3f_t::zero;
		vec3f_t		  target_point	  = vec3f_t::zero;
		entity_guid_t target_entity	  = NULL_ENTITY_GUID;
		u8			  enabled		  = 1;

		bool operator==(const component_fixed_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_fixed_constraint_t);

	struct component_distance_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_distance_constraint";

		vec3f_t		  local_point	   = vec3f_t::zero;
		vec3f_t		  target_point	   = vec3f_t::zero;
		entity_guid_t target_entity	   = NULL_ENTITY_GUID;
		f32			  min_distance	   = -1.0f;
		f32			  max_distance	   = -1.0f;
		f32			  spring_frequency = 0.0f;
		f32			  spring_damping   = 0.5f;
		u8			  enabled		   = 1;

		bool operator==(const component_distance_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_distance_constraint_t);

	struct component_point_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_point_constraint";

		vec3f_t		  local_point	= vec3f_t::zero;
		vec3f_t		  target_point	= vec3f_t::zero;
		entity_guid_t target_entity = NULL_ENTITY_GUID;
		u8			  enabled		= 1;

		bool operator==(const component_point_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_point_constraint_t);

	struct component_hinge_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_hinge_constraint";

		vec3f_t		  local_point		  = vec3f_t::zero;
		vec3f_t		  target_point		  = vec3f_t::zero;
		vec3f_t		  local_hinge_axis	  = vec3f_t::up;
		vec3f_t		  target_hinge_axis	  = vec3f_t::up;
		vec3f_t		  local_normal_axis	  = vec3f_t::right;
		vec3f_t		  target_normal_axis  = vec3f_t::right;
		entity_guid_t target_entity		  = NULL_ENTITY_GUID;
		f32			  limit_min_degrees	  = -180.0f;
		f32			  limit_max_degrees	  = 180.0f;
		f32			  spring_frequency	  = 0.0f;
		f32			  spring_damping	  = 0.5f;
		f32			  max_friction_torque = 0.0f;
		u8			  enabled			  = 1;

		bool operator==(const component_hinge_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_hinge_constraint_t);

	struct component_cone_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_cone_constraint";

		vec3f_t		  local_point			  = vec3f_t::zero;
		vec3f_t		  target_point			  = vec3f_t::zero;
		vec3f_t		  local_twist_axis		  = vec3f_t::right;
		vec3f_t		  target_twist_axis		  = vec3f_t::right;
		entity_guid_t target_entity			  = NULL_ENTITY_GUID;
		f32			  half_cone_angle_degrees = 0.0f;
		u8			  enabled				  = 1;

		bool operator==(const component_cone_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_cone_constraint_t);

	struct component_slider_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_slider_constraint";

		vec3f_t		  local_point		 = vec3f_t::zero;
		vec3f_t		  target_point		 = vec3f_t::zero;
		vec3f_t		  local_slider_axis	 = vec3f_t::right;
		vec3f_t		  target_slider_axis = vec3f_t::right;
		vec3f_t		  local_normal_axis	 = vec3f_t::up;
		vec3f_t		  target_normal_axis = vec3f_t::up;
		entity_guid_t target_entity		 = NULL_ENTITY_GUID;
		f32			  limit_min			 = -1000000.0f;
		f32			  limit_max			 = 1000000.0f;
		f32			  spring_frequency	 = 0.0f;
		f32			  spring_damping	 = 0.5f;
		f32			  max_friction_force = 0.0f;
		u8			  enabled			 = 1;

		bool operator==(const component_slider_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_slider_constraint_t);

	struct component_swing_twist_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_swing_twist_constraint";

		vec3f_t		  local_point					 = vec3f_t::zero;
		vec3f_t		  target_point					 = vec3f_t::zero;
		vec3f_t		  local_twist_axis				 = vec3f_t::right;
		vec3f_t		  target_twist_axis				 = vec3f_t::right;
		vec3f_t		  local_plane_axis				 = vec3f_t::up;
		vec3f_t		  target_plane_axis				 = vec3f_t::up;
		entity_guid_t target_entity					 = NULL_ENTITY_GUID;
		f32			  normal_half_cone_angle_degrees = 0.0f;
		f32			  plane_half_cone_angle_degrees	 = 0.0f;
		f32			  twist_min_angle_degrees		 = 0.0f;
		f32			  twist_max_angle_degrees		 = 0.0f;
		f32			  max_friction_torque			 = 0.0f;
		u8			  enabled						 = 1;

		bool operator==(const component_swing_twist_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_swing_twist_constraint_t);

	struct component_six_dof_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_six_dof_constraint";

		quat_t		  local_rotation			 = quat_t::identity;
		quat_t		  target_rotation			 = quat_t::identity;
		vec3f_t		  local_point				 = vec3f_t::zero;
		vec3f_t		  target_point				 = vec3f_t::zero;
		vec3f_t		  translation_limit_min		 = {-1000000.0f, -1000000.0f, -1000000.0f};
		vec3f_t		  translation_limit_max		 = {1000000.0f, 1000000.0f, 1000000.0f};
		vec3f_t		  rotation_limit_min_degrees = {-180.0f, -180.0f, -180.0f};
		vec3f_t		  rotation_limit_max_degrees = {180.0f, 180.0f, 180.0f};
		vec3f_t		  max_translation_friction	 = vec3f_t::zero;
		vec3f_t		  max_rotation_friction		 = vec3f_t::zero;
		entity_guid_t target_entity				 = NULL_ENTITY_GUID;
		u8			  enabled					 = 1;

		bool operator==(const component_six_dof_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_six_dof_constraint_t);

	struct component_pulley_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_pulley_constraint";

		vec3f_t		  local_body_point	 = vec3f_t::zero;
		vec3f_t		  fixed_point		 = vec3f_t::zero;
		vec3f_t		  target_body_point	 = vec3f_t::zero;
		vec3f_t		  target_fixed_point = vec3f_t::zero;
		entity_guid_t target_entity		 = NULL_ENTITY_GUID;
		f32			  ratio				 = 1.0f;
		f32			  min_length		 = 0.0f;
		f32			  max_length		 = -1.0f;
		u8			  enabled			 = 1;

		bool operator==(const component_pulley_constraint_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_pulley_constraint_t);

	struct physics_vehicle_wheel_t
	{
		vec3f_t position				  = vec3f_t::zero;
		vec3f_t suspension_direction	  = {0.0f, -1.0f, 0.0f};
		vec3f_t steering_axis			  = vec3f_t::up;
		vec3f_t wheel_up				  = vec3f_t::up;
		vec3f_t wheel_forward			  = vec3f_t::forward;
		f32		suspension_min_length	  = 0.3f;
		f32		suspension_max_length	  = 0.5f;
		f32		suspension_preload_length = 0.0f;
		f32		suspension_frequency	  = 1.5f;
		f32		suspension_damping		  = 0.5f;
		f32		radius					  = 0.3f;
		f32		width					  = 0.1f;
		f32		inertia					  = 0.9f;
		f32		angular_damping			  = 0.2f;
		f32		max_steer_angle_degrees	  = 70.0f;
		f32		max_brake_torque		  = 1500.0f;
		f32		max_hand_brake_torque	  = 4000.0f;

		bool operator==(const physics_vehicle_wheel_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(physics_vehicle_wheel_t);

	struct physics_vehicle_differential_t
	{
		i32 left_wheel_index	= -1;
		i32 right_wheel_index	= -1;
		f32 differential_ratio	= 3.42f;
		f32 engine_torque_ratio = 1.0f;
		f32 limited_slip_ratio	= 1.4f;

		bool operator==(const physics_vehicle_differential_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(physics_vehicle_differential_t);

	struct component_vehicle_constraint_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_vehicle_constraint";

		inplace_vector_t<physics_vehicle_wheel_t, 16>		wheels							= {};
		inplace_vector_t<physics_vehicle_differential_t, 8> differentials					= {};
		vec3f_t												up								= vec3f_t::up;
		vec3f_t												forward							= vec3f_t::forward;
		f32													max_pitch_roll_angle_degrees	= 180.0f;
		f32													differential_limited_slip_ratio = 1.4f;
		u8													collision_layer					= 0;
		u8													enabled							= 1;
	};

	SFG_DEFINE_TYPE_ID(component_vehicle_constraint_t);

	struct component_character_mover_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_character_mover";

		vec3f_t shape_offset				   = vec3f_t::zero;
		f32		radius						   = 0.4f;
		f32		half_height					   = 0.9f;
		f32		max_slope_degrees			   = 50.0f;
		f32		step_up						   = 0.4f;
		f32		step_down					   = 0.5f;
		f32		min_step_forward			   = 0.02f;
		f32		step_forward_test			   = 0.15f;
		f32		mass						   = 70.0f;
		f32		max_strength				   = 100.0f;
		f32		padding						   = 0.02f;
		f32		predictive_contact_distance	   = 0.1f;
		f32		penetration_recovery_speed	   = 1.0f;
		u8		collision_layer				   = 0;
		u8		enhanced_internal_edge_removal = 0;

		bool operator==(const component_character_mover_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(component_character_mover_t);

	enum class debug_widgets_enum : u8
	{
		debug_widgets_enum_a,
		debug_widgets_enum_b,
	};

	enum class debug_widgets_enum2 : u32
	{
		debug_widgets_enum2_a,
		debug_widgets_enum2_b,
	};

	struct debug_struct2_t
	{
		f32 f32_value = 2.0f;
		u32 u32_value = 20;
	};

	SFG_DEFINE_TYPE_ID(debug_struct2_t);

	struct debug_struct_t
	{
		vec3f_t			vec3_value = {1.0f, 2.0f, 3.0f};
		f32				f32_value  = 1.0f;
		debug_struct2_t test	   = {};
	};

	SFG_DEFINE_TYPE_ID(debug_struct_t);

	struct component_debug_widgets_t
	{
		static inline constexpr const char* DEBUG_NAME = "debug_widgets_component";

		inplace_vector_t<u32, 4> inplace_vector_value				  = {1, 2, 3};
		debug_struct_t			 debug_struct_value					  = {};
		debug_struct2_t			 debug_struct2_value				  = {};
		resource_handle_t		 audio_handle_value					  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 font_handle_value					  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 mesh_handle_value					  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 skeleton_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 animation_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 material_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 shader_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 texture_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 texture_sampler_handle_value		  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 physical_material_handle_value		  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 prefab_handle_value				  = NULL_RESOURCE_HANDLE;
		resource_handle_t		 animation_state_machine_handle_value = NULL_RESOURCE_HANDLE;
		resource_handle_t		 hdr_skybox_handle_value			  = NULL_RESOURCE_HANDLE;
		entity_guid_t			 entity_guid_value					  = NULL_ENTITY_GUID;
		quat_t					 quat_value							  = {};
		color_t					 color_value						  = color_t::red;
		f32						 f32_value							  = 1.0f;
		i32						 i32_value							  = -32;
		u32						 u32_value							  = 32;
		u32						 text_id_value						  = ECS_INVALID_INDEX;
		u32						 enum32_value						  = 1;
		i8						 i8_value							  = -8;
		u8						 u8_value							  = 8;
		u8						 bool8_value						  = 1;
		u8						 enum8_value						  = 1;
	};

	SFG_DEFINE_TYPE_ID(component_debug_widgets_t);

	struct component_alive_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_alive";
	};

	SFG_DEFINE_TYPE_ID(component_alive_t);

	struct component_disabled_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_disabled";
	};

	SFG_DEFINE_TYPE_ID(component_disabled_t);

	struct component_no_serialize_t
	{
		static inline constexpr const char* DEBUG_NAME = "component_no_serialize";
	};

	SFG_DEFINE_TYPE_ID(component_no_serialize_t);

	struct engine_component_reflection_t
	{
		engine_component_reflection_t();
	};

	inline engine_component_reflection_t g_reflect_engine_component;
}
