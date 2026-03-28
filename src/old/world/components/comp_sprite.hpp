#pragma once

#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "resources/common_resources.hpp"
#include "memory/pool_handle.hpp"

namespace SFG
{
	class world;

	class comp_sprite
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// comp
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_material(world& w, resource_handle material);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline resource_handle get_material() const
		{
			return _material;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header   = {};
		resource_handle	 _material = {};
	};

	REFLECT_TYPE(comp_sprite);
}
