// Copyright (c) 2025 Inan Evin
#pragma once

#include "resource_handle.hpp"
#include "resource_type.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <cstddef>

namespace sfg
{
#define DEFAULT_ALBEDO_TEXTURE_ASSET_GUID				 1002
#define DEFAULT_ORM_TEXTURE_ASSET_GUID					 1003
#define DEFAULT_NORMAL_TEXTURE_ASSET_GUID				 1004
#define DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID				 1005
#define DEFAULT_PHYSICAL_MATERIAL_ASSET_GUID			 1008
#define DEFAULT_LINEAR_SAMPLER_ASSET_GUID				 1009
#define DEFAULT_NEAREST_SAMPLER_ASSET_GUID				 1010
#define DEFAULT_ANISOTROPIC_SAMPLER_ASSET_GUID			 1011
#define DEFAULT_QWANTANI_DUSK_CUBEMAP_ASSET_GUID		 1012
#define DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID		 1013
#define DEFAULT_NEAREST_SAMPLER_REPEAT_ASSET_GUID		 1014
#define DEFAULT_MESH_CUBE_GUID							 1015
#define DEFAULT_MESH_SPHERE_GUID						 1016
#define DEFAULT_MESH_CYLINDER_GUID						 1017
#define DEFAULT_MESH_CAPSULE_GUID						 1018
#define DEFAULT_ANISOTROPIC_SAMPLER_REPEAT_ASSET_GUID	 1019
#define GIZMO_MESH_TRANSLATION							 1020
#define GIZMO_MESH_ROTATION								 1021
#define GIZMO_MESH_SCALE								 1022
#define DEFAULT_CUBE_SKYBOX_SHADER_ASSET_GUID			 1025
#define DEFAULT_GRADIENT_SKYBOX_SHADER_ASSET_GUID		 1026
#define DEFAULT_CUBE_SKYBOX_MATERIAL_ASSET_GUID			 1027
#define DEFAULT_GRADIENT_SKYBOX_MATERIAL_ASSET_GUID		 1028
#define DEFAULT_LIT_SHADER_ASSET_GUID					 1029
#define DEFAULT_OBJECT_UNLIT_SHADER_ASSET_GUID			 1030
#define DEFAULT_OPAQUE_MATERIAL_ASSET_GUID				 1031
#define DEFAULT_OPAQUE_UNLIT_MATERIAL_ASSET_GUID		 1032
#define DEFAULT_TRANSPARENT_MATERIAL_ASSET_GUID			 1033
#define DEFAULT_TRANSPARENT_UNLIT_MATERIAL_ASSET_GUID	 1034
#define DEFAULT_GRID_DARK_TEXTURE_ASSET_GUID			 1035
#define DEFAULT_GRID_GREEN_TEXTURE_ASSET_GUID			 1036
#define DEFAULT_GRID_LIGHT_TEXTURE_ASSET_GUID			 1037
#define DEFAULT_GRID_ORANGE_TEXTURE_ASSET_GUID			 1038
#define DEFAULT_GRID_PURPLE_TEXTURE_ASSET_GUID			 1039
#define DEFAULT_GRID_RED_TEXTURE_ASSET_GUID				 1040
#define DEFAULT_SPRITE_LIT_SHADER_ASSET_GUID			 1041
#define DEFAULT_SPRITE_LIT_MATERIAL_ASSET_GUID			 1042
#define DEFAULT_SPRITE_UNLIT_SHADER_ASSET_GUID			 1043
#define DEFAULT_SPRITE_UNLIT_MATERIAL_ASSET_GUID		 1044
#define DEFAULT_PARTICLE_SHADER_ASSET_GUID				 1045
#define DEFAULT_PARTICLE_MATERIAL_ASSET_GUID			 1046
#define DEFAULT_MESH_PLANE_GUID							 1047
#define DEFAULT_GRID_MATERIAL_ASSET_GUID				 1048
#define DEFAULT_SPRITE_CIRCLE_ASSET_GUID				 1049
#define DEFAULT_POST_PROCESS_SHADER_ASSET_GUID			 1050
#define DEFAULT_POST_PROCESS_MATERIAL_ASSET_GUID		 1051
#define DEFAULT_GRID_DARK_MATERIAL_ASSET_GUID			 1052
#define DEFAULT_GRADIENT_DARK_SKYBOX_MATERIAL_ASSET_GUID 1053

	class resource_manager_t;
	class istream_t;
	class ostream_t;
	class resource_file_system_t;

#define MAX_DEBUG_NAME_SIZE 128
#define MAX_SKELETON_BONES	192
	static_assert(MAX_SKELETON_BONES != 0);

	struct skeleton_mask_t
	{
		static constexpr size_t SZ		  = (MAX_SKELETON_BONES + 63) / 64;
		u64						masks[SZ] = {0};

		inline skeleton_mask_t operator|(const skeleton_mask_t& other) const
		{
			skeleton_mask_t m = {};

			for (u32 i = 0; i < SZ; i++)
			{
				m.masks[i] = masks[i] | other.masks[i];
			}

			return m;
		}

		inline skeleton_mask_t& operator|=(const skeleton_mask_t& other)
		{
			for (u32 i = 0; i < SZ; i++)
			{
				this->masks[i] |= other.masks[i];
			}

			return *this;
		}

		inline bool masked(u32 joint_index) const
		{
			const u32 idx	= joint_index / 64;
			const u32 local = joint_index % 64;

			return (masks[idx] & (1llu << local)) != 0;
		}

		inline bool empty() const
		{
			for (u32 i = 0; i < SZ; i++)
			{
				if (masks[i] != 0)
					return false;
			}
			return true;
		}
	};

	inline constexpr u32 make_resource_wire_magic(char c0, char c1, char c2, char c3)
	{
		const u32 b0 = static_cast<u32>(static_cast<u8>(c0)) << 24;
		const u32 b1 = static_cast<u32>(static_cast<u8>(c1)) << 16;
		const u32 b2 = static_cast<u32>(static_cast<u8>(c2)) << 8;
		const u32 b3 = static_cast<u32>(static_cast<u8>(c3));
		return b0 | b1 | b2 | b3;
	}

#pragma pack(push, 1)
	struct resource_dependency_t
	{
		resource_handle_t handle;
		resource_type_e	  type;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct resource_header_t
	{
		char			debug_name[MAX_DEBUG_NAME_SIZE] = {};
		resource_type_e type							= resource_type_e::invalid;
		u32				magic							= 0;
		u32				version							= 0;
		u64				source_tick						= 0;
		u64				file_source_ticks				= 0;
		u32				dependency_count				= 0;

		void	  set_debug_name(const char* name);
		void	  set_debug_name_from_path(const char* full_path);
		ostream_t make_stream(const ostream_t& payload) const;
		void	  serialize(ostream_t& stream) const;
		void	  deserialize(istream_t& stream);
	};
#pragma pack(pop)

	enum class resource_state_e : u8
	{
		failed,
		ready,
	};

	struct resource_entry_t
	{
		sid_t			 hash			  = 0;
		u64				 source_ticks	  = 0;
		chunk_handle32_t runtime		  = {};
		chunk_handle32_t internals		  = {};
		chunk_handle32_t debug_name		  = {};
		chunk_handle32_t dependencies	  = {};
		u32				 ref_count		  = 0;
		u32				 dependency_count = 0;
		resource_state_e state			  = resource_state_e::failed;
		resource_type_e	 type			  = resource_type_e::invalid;
	};

	struct resource_context_t
	{
		resource_manager_t& resource_manager;
	};

	struct resource_type_desc_t
	{
		using load_fn_t			= bool (*)(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		using runtime_load_fn_t = bool (*)(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		using unload_fn_t		= void (*)(resource_entry_t& entry, resource_context_t& ctx);

		resource_type_e type				= resource_type_e::invalid;
		u32				runtime_size		= 0;
		u32				runtime_alignment	= 0;
		u32				internals_size		= 0;
		u32				internals_alignment = 0;
		u32				wire_magic			= 0;
		u32				wire_version		= 0;

		load_fn_t		  load		   = nullptr;
		runtime_load_fn_t runtime_load = nullptr;
		unload_fn_t		  unload	   = nullptr;
	};

	extern const resource_type_desc_t* const g_resource_type_descs[RESOURCE_TYPE_MAX];

	const char* resource_state_to_string(resource_state_e state);
	const char* resource_type_to_string(resource_type_e type);

	inline const resource_type_desc_t* find_resource_type_desc(resource_type_e type)
	{
		const u8 t = static_cast<u8>(type);
		if (t >= RESOURCE_TYPE_MAX)
			return nullptr;
		return g_resource_type_descs[t];
	}

	struct resource_type_reflection_t
	{
		resource_type_reflection_t();
	};

	inline resource_type_reflection_t g_reflect_resource_type;
}
