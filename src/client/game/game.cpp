#include <std_include.hpp>
#include "game.hpp"
#include "dvars.hpp"

#include "component/console/console.hpp"

#include <utils/flags.hpp>
#include <utils/string.hpp>
#include <utils/io.hpp>

namespace game
{
	uint64_t base_address;

	namespace environment
	{
		bool is_dedi()
		{
			static const auto dedicated = utils::flags::has_flag("dedicated");
			return dedicated;
		}
	}
}

size_t reverse_b(const size_t ptr)
{
	return ptr - game::base_address;
}

size_t reverse_b(const void* ptr)
{
	return reverse_b(reinterpret_cast<size_t>(ptr));
}
