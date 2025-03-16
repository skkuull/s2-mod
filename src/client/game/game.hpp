#pragma once

#include "structs.hpp"

#define PROTOCOL 2

namespace game
{
	extern uint64_t base_address;

	template <typename T>
	class symbol
	{
	public:
		symbol(const size_t address)
			: address_(reinterpret_cast<T*>(address))
		{
		}

		T* get() const
		{
			return reinterpret_cast<T*>(reinterpret_cast<uint64_t>(address_));
		}

		operator T* () const
		{
			return this->get();
		}

		T* operator->() const
		{
			return this->get();
		}

	private:
		T* address_;
	};

	namespace environment
	{
		bool is_dedi();
	}
}

size_t reverse_b(const size_t ptr);
size_t reverse_b(const void* ptr);

#include "symbols.hpp"
