#include "core/vendor/util/json_utils.hpp"

#include "core/vendor/nhlohmann/json.hpp"
#include "core/gfx/common/shader_description.hpp"

namespace sfg
{
	void json_util::apply_changes(const nlohmann::json& before, const nlohmann::json& after, nlohmann::json& target)
	{
		if (before == after)
			return;

		if (before.is_object() && after.is_object() && target.is_object())
		{
			for (const auto& field : before.items())
			{
				if (!after.contains(field.key()))
					target.erase(field.key());
			}

			for (const auto& field : after.items())
			{
				if (!before.contains(field.key()))
					target[field.key()] = field.value();
				else if (target.contains(field.key()))
					apply_changes(before.at(field.key()), field.value(), target[field.key()]);
				else if (before.at(field.key()) != field.value())
					target[field.key()] = field.value();
			}

			return;
		}

		if (before.is_array() && after.is_array() && target.is_array() && before.size() == after.size())
		{
			for (size_t i = 0; i < after.size() && i < target.size(); ++i)
				apply_changes(before[i], after[i], target[i]);

			return;
		}

		target = after;
	}
}
