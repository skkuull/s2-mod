#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// min and max is required by gdi, therefore NOMINMAX won't work
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <string>
#include <functional>
#include <filesystem>

namespace utils::nt
{
	class library final
	{
	public:
		static library load(const std::string& name);
		static library load(const std::filesystem::path& path);
		static library get_by_address(void* address);

		library();
		explicit library(const std::string& name);
		explicit library(HMODULE handle);

		library(const library& a) : module_(a.module_)
		{
		}

		bool operator!=(const library& obj) const { return !(*this == obj); };
		bool operator==(const library& obj) const;

		operator bool() const;
		operator HMODULE() const;

		void unprotect() const;
		[[nodiscard]] void* get_entry_point() const;
		[[nodiscard]] size_t get_relative_entry_point() const;

		[[nodiscard]] bool is_valid() const;
		[[nodiscard]] std::string get_name() const;
		[[nodiscard]] std::filesystem::path get_path() const;
		[[nodiscard]] std::filesystem::path get_folder() const;
		[[nodiscard]] std::uint8_t* get_ptr() const;
		void free();

		[[nodiscard]] HMODULE get_handle() const;

		template <typename T>
		[[nodiscard]] T get_proc(const char* process) const
		{
			if (!this->is_valid()) T{};
			return reinterpret_cast<T>(GetProcAddress(this->module_, process));
		}

		template <typename T>
		[[nodiscard]] T get_proc(const std::string& process) const
		{
			return get_proc<T>(process.data());
		}

		template <typename T>
		std::function<T> get(const std::string& process) const
		{
			if (!this->is_valid()) return std::function<T>();
			return static_cast<T*>(this->get_proc<void*>(process));
		}

		template <typename T, typename... Args>
		T invoke(const std::string& process, Args ... args) const
		{
			auto method = this->get<T(__cdecl)(Args ...)>(process);
			if (method) return method(args...);
			return T();
		}

		template <typename T, typename... Args>
		T invoke_pascal(const std::string& process, Args ... args) const
		{
			auto method = this->get<T(__stdcall)(Args ...)>(process);
			if (method) return method(args...);
			return T();
		}

		template <typename T, typename... Args>
		T invoke_this(const std::string& process, void* this_ptr, Args ... args) const
		{
			auto method = this->get<T(__thiscall)(void*, Args ...)>(this_ptr, process);
			if (method) return method(args...);
			return T();
		}

		[[nodiscard]] std::vector<PIMAGE_SECTION_HEADER> get_section_headers() const;

		[[nodiscard]] PIMAGE_NT_HEADERS get_nt_headers() const;
		[[nodiscard]] PIMAGE_DOS_HEADER get_dos_header() const;
		[[nodiscard]] PIMAGE_OPTIONAL_HEADER get_optional_header() const;

		[[nodiscard]] void** get_iat_entry(const std::string& module_name, std::string proc_name) const;
		[[nodiscard]] void** get_iat_entry(const std::string& module_name, const char* proc_name) const;

	private:
		HMODULE module_;
	};

	bool is_wine();

	__declspec(noreturn) void raise_hard_exception();
	std::string load_resource(int id);

	void relaunch_self(const std::string& extra_command_line = "", bool override_command_line = false);
	__declspec(noreturn) void terminate(uint32_t code = 0);
}