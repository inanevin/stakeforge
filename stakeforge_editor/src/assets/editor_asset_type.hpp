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

#include <sfg/vendor/nhlohmann/json_fwd.hpp>

#include <sfg/common/size_definitions.hpp>
#include <sfg/runtime/resources/resource_type.hpp>

namespace sfg
{
#define SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_ANY_RESOURCE "editor_reflection_asset_subtype_any_resource"_hs
#define SFG_EDITOR_REFLECTION_ASSET_SUB_TYPE_ID_WORLD		 "editor_reflection_asset_subtype_world"_hs

	enum class editor_asset_type_e : u8
	{
		invalid,
		audio,
		font,
		mesh,
		skeleton,
		animation,
		material,
		shader,
		texture,
		texture_sampler,
		physical_material,
		prefab,
		animation_graph,
		cubemap,
		physics_collision_mesh,
		sprite,
		curve,
		ragdoll,
		animation_library,
		world,
		count,
	};

	void				to_json(nlohmann::json& j, const editor_asset_type_e& t);
	void				from_json(const nlohmann::json& j, editor_asset_type_e& t);
	editor_asset_type_e editor_asset_type_from_resource_type(resource_type_e type);
	editor_asset_type_e editor_asset_type_from_reflection_sub_type_id(sid_t sub_type_id);
}
