#pragma once
#include <utils/nt.hpp>

namespace loader
{
	utils::nt::library load_binary(const std::string& filename);

	void set_import_resolver(const std::function<void* (const std::string&, const std::string&)>& resolver);
}