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

#include <sfg/io/assert.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <utility>

namespace sfg
{
	template <typename T, typename SIZE_TYPE = u32, typename TAG = T> class tree_t final
	{
	public:
		using handle_t = pool_handle_t<SIZE_TYPE, TAG>;

		struct node_t
		{
			T		  value;
			handle_t  parent;
			handle_t  first_child;
			handle_t  last_child;
			handle_t  next_sibling;
			handle_t  prev_sibling;
			SIZE_TYPE child_count = 0;
		};

	private:
		using pool_t = dynamic_gen_pool_t<node_t, SIZE_TYPE, TAG>;

	public:
		tree_t()								   = default;
		~tree_t()								   = default;
		tree_t(const tree_t&)					   = delete;
		tree_t& operator=(const tree_t&)		   = delete;
		tree_t(tree_t&& other) noexcept			   = default;
		tree_t& operator=(tree_t&& other) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		inline void reserve(SIZE_TYPE capacity)
		{
			_nodes.reserve(capacity);
		}

		inline void clear()
		{
			_nodes.clear();
		}

		inline void resize_zero()
		{
			_nodes.resize_zero();
		}

		// -----------------------------------------------------------------------------
		// nodes
		// -----------------------------------------------------------------------------

		template <typename... Args> inline handle_t emplace(Args&&... args)
		{
			return _nodes.emplace(node_t{.value = T(std::forward<Args>(args)...)});
		}

		inline void remove(handle_t handle)
		{
			node_t& n = _nodes.get(handle);
			SFG_ASSERT(n.child_count == 0);
			detach(handle);
			_nodes.remove(handle);
		}

		inline void remove_subtree(handle_t handle)
		{
			SFG_ASSERT(_nodes.is_valid(handle));

			handle_t current = handle;
			while (!current.is_null())
			{
				node_t& n = _nodes.get(current);
				if (!n.first_child.is_null())
				{
					current = n.first_child;
					continue;
				}

				const bool	   done	  = current == handle;
				const handle_t next	  = n.next_sibling;
				const handle_t parent = n.parent;
				detach(current);
				_nodes.remove(current);

				if (done)
					break;

				current = !next.is_null() ? next : parent;
			}
		}

		inline void attach(handle_t parent, handle_t child)
		{
			SFG_ASSERT(_nodes.is_valid(parent));
			SFG_ASSERT(_nodes.is_valid(child));
			SFG_ASSERT(!(parent == child));
			SFG_ASSERT(!is_ancestor(child, parent));

			node_t& c = _nodes.get(child);
			if (!c.parent.is_null())
				detach(child);

			node_t& p	   = _nodes.get(parent);
			c.parent	   = parent;
			c.prev_sibling = p.last_child;
			c.next_sibling = {};

			if (!p.last_child.is_null())
				_nodes.get(p.last_child).next_sibling = child;
			else
				p.first_child = child;

			p.last_child = child;
			p.child_count++;
		}

		inline void detach(handle_t child)
		{
			node_t& c = _nodes.get(child);
			if (c.parent.is_null())
				return;

			node_t& p = _nodes.get(c.parent);
			if (!c.prev_sibling.is_null())
				_nodes.get(c.prev_sibling).next_sibling = c.next_sibling;
			else
				p.first_child = c.next_sibling;

			if (!c.next_sibling.is_null())
				_nodes.get(c.next_sibling).prev_sibling = c.prev_sibling;
			else
				p.last_child = c.prev_sibling;

			p.child_count--;
			c.parent	   = {};
			c.prev_sibling = {};
			c.next_sibling = {};
		}

		// -----------------------------------------------------------------------------
		// traversal
		// -----------------------------------------------------------------------------

		// Visits only the direct children of parent, from first child to last child.
		template <typename FN> inline void for_each_child(handle_t parent, FN&& fn) const
		{
			SFG_ASSERT(_nodes.is_valid(parent));

			handle_t child = _nodes.get(parent).first_child;
			while (!child.is_null())
			{
				fn(child);
				child = _nodes.get(child).next_sibling;
			}
		}

		// Visits root first, then each descendant before moving to the next sibling.
		template <typename FN> inline void for_each_depth_first(handle_t root, FN&& fn) const
		{
			SFG_ASSERT(_nodes.is_valid(root));

			handle_t  current = root;
			SIZE_TYPE depth	  = 0;
			while (!current.is_null())
			{
				fn(current, depth);

				const handle_t child = _nodes.get(current).first_child;
				if (!child.is_null())
				{
					current = child;
					depth++;
					continue;
				}

				while (!(current == root))
				{
					const handle_t next = _nodes.get(current).next_sibling;
					if (!next.is_null())
					{
						current = next;
						break;
					}

					current = _nodes.get(current).parent;
					depth--;
				}

				if (current == root)
					current = {};
			}
		}

		// Visits descendants before their parent, useful for bottom-up work.
		template <typename FN> inline void for_each_depth_first_post_order(handle_t root, FN&& fn) const
		{
			SFG_ASSERT(_nodes.is_valid(root));

			handle_t  current = root;
			SIZE_TYPE depth	  = 0;
			while (!current.is_null())
			{
				const handle_t child = _nodes.get(current).first_child;
				if (!child.is_null())
				{
					current = child;
					depth++;
					continue;
				}

				while (!current.is_null())
				{
					fn(current, depth);

					if (current == root)
						return;

					const handle_t next = _nodes.get(current).next_sibling;
					if (!next.is_null())
					{
						current = next;
						break;
					}

					current = _nodes.get(current).parent;
					depth--;
				}
			}
		}

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		inline bool is_valid(handle_t handle) const
		{
			return _nodes.is_valid(handle);
		}

		inline bool is_ancestor(handle_t ancestor, handle_t handle) const
		{
			SFG_ASSERT(_nodes.is_valid(ancestor));
			SFG_ASSERT(_nodes.is_valid(handle));

			handle_t parent = _nodes.get(handle).parent;
			while (!parent.is_null())
			{
				if (parent == ancestor)
					return true;

				parent = _nodes.get(parent).parent;
			}

			return false;
		}

		inline SIZE_TYPE size() const
		{
			return _nodes.size();
		}

		inline SIZE_TYPE capacity() const
		{
			return _nodes.capacity();
		}

		inline bool empty() const
		{
			return _nodes.empty();
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const node_t& node(handle_t handle) const
		{
			return _nodes.get(handle);
		}

		inline T& value(handle_t handle)
		{
			return _nodes.get(handle).value;
		}

		inline const T& value(handle_t handle) const
		{
			return _nodes.get(handle).value;
		}

		inline handle_t parent(handle_t handle) const
		{
			return _nodes.get(handle).parent;
		}

		inline handle_t first_child(handle_t handle) const
		{
			return _nodes.get(handle).first_child;
		}

		inline handle_t last_child(handle_t handle) const
		{
			return _nodes.get(handle).last_child;
		}

		inline handle_t next_sibling(handle_t handle) const
		{
			return _nodes.get(handle).next_sibling;
		}

		inline handle_t prev_sibling(handle_t handle) const
		{
			return _nodes.get(handle).prev_sibling;
		}

		inline SIZE_TYPE child_count(handle_t handle) const
		{
			return _nodes.get(handle).child_count;
		}

		inline typename pool_t::handle_iterator_t begin_handle() const
		{
			return _nodes.begin_handle();
		}

		inline typename pool_t::handle_iterator_t end_handle() const
		{
			return _nodes.end_handle();
		}

	private:
		pool_t _nodes;
	};
}
