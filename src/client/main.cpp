#include <std_include.hpp>
#include "loader/loader.hpp"
#include "launcher/launcher.hpp"
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <utils/string.hpp>
#include <utils/io.hpp>
#include <utils/properties.hpp>
#include <utils/flags.hpp>

DECLSPEC_NORETURN void WINAPI exit_hook(const int code)
{
	component_loader::pre_destroy();
	exit(code);
}

DWORD_PTR WINAPI set_thread_affinity_mask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask)
{
	component_loader::post_unpack();
	return SetThreadAffinityMask(hThread, dwThreadAffinityMask);
}

launcher::mode detect_mode_from_arguments()
{
	if (utils::flags::has_flag("dedicated"))
	{
		return launcher::mode::server;
	}

	if (utils::flags::has_flag("multiplayer"))
	{
		return launcher::mode::multiplayer;
	}

	if (utils::flags::has_flag("singleplayer"))
	{
		return launcher::mode::singleplayer;
	}

	return launcher::mode::none;
}

void apply_aslr_patch(std::string* data)
{
	// sp binary, mp binary
	if (data->size() != 0xE46800 && data->size() != 0x12EFA00)
	{
		printf("%llu", data->size());
		throw std::runtime_error("File size mismatch, bad game files");
	}

	auto* dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(&data->at(0));
	auto* nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(&data->at(dos_header->e_lfanew));
	auto* optional_header = &nt_headers->OptionalHeader;

	if (optional_header->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)
	{
		optional_header->DllCharacteristics &= ~(IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE);
	}
}

void get_aslr_patched_binary(std::string* binary, std::string* data)
{
	const auto patched_binary = (utils::properties::get_appdata_path() / "bin" / *binary).generic_string();

	try
	{
		apply_aslr_patch(data);
		if (!utils::io::file_exists(patched_binary) && !utils::io::write_file(patched_binary, *data, false))
		{
			throw std::runtime_error("Could not write file");
		}
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(
			utils::string::va("Could not create aslr patched binary for %s! %s",
				binary->data(), e.what())
		);
	}

	*binary = patched_binary;
}

FARPROC load_binary(const launcher::mode mode)
{
	utils::nt::library self;

	loader::set_import_resolver([self](const std::string& library, const std::string& function) -> void*
	{
		if (function == "ExitProcess")
		{
			return exit_hook;
		}
		else if (function == "SetThreadAffinityMask")
		{
			return set_thread_affinity_mask;
		}

		return component_loader::load_import(library, function);
	});

	std::string binary;
	switch (mode)
	{
	case launcher::mode::server:
	case launcher::mode::multiplayer:
		binary = "s2_mp64_ship.exe";
		break;
	case launcher::mode::singleplayer:
		binary = "s2_sp64_ship.exe";
		break;
	case launcher::mode::none:
	default:
		throw std::runtime_error("Invalid game mode!");
	}

	std::string data;
	if (!utils::io::read_file(binary, &data))
	{
		throw std::runtime_error(utils::string::va(
			"Failed to read game binary (%s)!\nPlease copy the s2-mod.exe into your Call of Duty: WWII installation folder and run it from there.",
			binary.data()));
	}

	get_aslr_patched_binary(&binary, &data);

	const auto proc = loader::load_binary(binary);
	auto* const peb = reinterpret_cast<PPEB>(__readgsqword(0x60));
	peb->Reserved3[1] = proc.get_ptr();
	static_assert(offsetof(PEB, Reserved3[1]) == 0x10);

	return FARPROC(proc.get_ptr() + proc.get_relative_entry_point());
}

void remove_crash_file()
{
	utils::io::remove_file("__s2Exe");
}

void enable_dpi_awareness()
{
	const utils::nt::library user32{ "user32.dll" };
	const auto set_dpi = user32
		? user32.get_proc<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT)>("SetProcessDpiAwarenessContext")
		: nullptr;
	if (set_dpi)
	{
		set_dpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}
}

void limit_parallel_dll_loading()
{
	const utils::nt::library self;
	const auto registry_path = R"(Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\)" + self.
		get_name();

	HKEY key = nullptr;
	if (RegCreateKeyA(HKEY_LOCAL_MACHINE, registry_path.data(), &key) == ERROR_SUCCESS)
	{
		RegCloseKey(key);
	}

	key = nullptr;
	if (RegOpenKeyExA(
		HKEY_LOCAL_MACHINE, registry_path.data(), 0,
		KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	{
		return;
	}

	DWORD value = 1;
	RegSetValueExA(key, "MaxLoaderThreads", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));

	RegCloseKey(key);
}

int main()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	FARPROC entry_point;
	enable_dpi_awareness();

	// This requires admin privilege
	limit_parallel_dll_loading();

	srand(uint32_t(time(nullptr)));
	remove_crash_file();

	{
		component_loader::sort();

		auto premature_shutdown = true;
		const auto _ = gsl::finally([&premature_shutdown]()
		{
			if (premature_shutdown)
			{
				component_loader::pre_destroy();
			}
		});

		try
		{
			if (!component_loader::post_start()) return EXIT_FAILURE;

			auto mode = detect_mode_from_arguments();
			if (mode == launcher::mode::none)
			{
				const launcher launcher;
				mode = launcher.run();
				if (mode == launcher::mode::none) return 0;
			}

			game::environment::set_mode(mode);

			entry_point = load_binary(mode);
			if (!entry_point)
			{
				throw std::runtime_error("Unable to load binary into memory");
			}

			if (!component_loader::post_load()) return EXIT_FAILURE;

			premature_shutdown = false;
		}
		catch (std::exception& e)
		{
			MessageBoxA(nullptr, e.what(), "ERROR", MB_ICONERROR);
			return EXIT_FAILURE;
		}
	}

	return static_cast<int>(entry_point());
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PSTR, _In_ int)
{
	return main();
}
