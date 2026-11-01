/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include <sfg/core/common/type_id.hpp>
#include <sfg/core/math/color.hpp>
#include <sfg/core/math/quat.hpp>
#include <sfg/core/math/vec2f.hpp>
#include <sfg/core/math/vec2u16.hpp>
#include <sfg/core/math/vec3f.hpp>
#include <sfg/core/math/vec4f.hpp>
#include <sfg/core/runtime/resources/resource_handle.hpp>
#include <sfg/core/runtime/world/ecs_defs.hpp>

namespace game
{
	/*
		demonstrates the allowed types for game components to have, how to define enums, nest types etc.
		in a nutshell, the idea is that we like POD types.
		you won't be able to register a component or type into the component registry if the type is not:
		- trivially copyable
		- standard layout
	*/

	using sfg::entity_guid_t;

	enum class example_mode_e : u32
	{
		idle,
		patrol,
		chase,
	};

	enum class example_ability_e : u32
	{
		move = 1 << 0,
		jump = 1 << 1,
	};

	struct example_settings_t
	{
		f32	 strength = 1.0f;
		bool enabled  = true;
	};

	struct example_integer_list_t
	{
		static inline constexpr size_t CAPACITY = 8;

		size_t count			= 3;
		i32	   values[CAPACITY] = {1, 2, 3};
	};

	struct component_example_t
	{
		static inline constexpr size_t SAMPLE_COUNT = 4;
		static inline constexpr size_t PRESET_COUNT = 2;

		example_integer_list_t integers				 = {};
		u64					   unsigned_64			 = 64;
		i64					   signed_64			 = -64;
		entity_guid_t		   target				 = NULL_ENTITY_GUID;
		sfg::resource_handle_t mesh					 = NULL_RESOURCE_HANDLE;
		sfg::quat_t			   rotation				 = sfg::quat_t::identity;
		sfg::vec4f_t		   vector_4				 = sfg::vec4f_t::zero;
		sfg::color_t		   tint					 = sfg::color_t::white;
		sfg::vec3f_t		   vector_3				 = sfg::vec3f_t::zero;
		sfg::vec2f_t		   vector_2				 = sfg::vec2f_t::zero;
		example_settings_t	   settings				 = {};
		example_settings_t	   presets[PRESET_COUNT] = {};
		i32					   samples[SAMPLE_COUNT] = {10, 20, 30, 40};
		u32					   unsigned_32			 = 32;
		i32					   signed_32			 = -32;
		f32					   amount				 = 0.5f;
		example_mode_e		   mode					 = example_mode_e::idle;
		u32					   abilities			 = static_cast<u32>(example_ability_e::move);
		sfg::vec2u16_t		   grid					 = sfg::vec2u16_t::zero;
		u16					   unsigned_16			 = 16;
		i16					   signed_16			 = -16;
		u8					   unsigned_8			 = 8;
		i8					   signed_8				 = -8;
		bool				   enabled				 = true;
		char				   label[64]			 = "Example";
	};
}

namespace sfg
{
	SFG_DEFINE_TYPE_ID(game::example_mode_e);
	SFG_DEFINE_TYPE_ID(game::example_settings_t);
	SFG_DEFINE_TYPE_ID(game::component_example_t);
}
