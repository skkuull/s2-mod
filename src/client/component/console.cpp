#include <std_include.hpp>

#include "loader/component_loader.hpp"

#include <utils/string.hpp>
#include <utils/thread.hpp>

#include "game/game.hpp"

#include "console.hpp"

#include "version.hpp"

namespace console
{
	static std::atomic_bool ingame = false;
	static std::atomic_bool exit = false;

	DWORD WINAPI console_thread(LPVOID)
	{
		ShowWindow(GetConsoleWindow(), SW_SHOW);
		SetConsoleTitleA("S2-Mod: " VERSION);

		std::string cmd;
		exit = false;

		while (!exit)
		{
			std::getline(std::cin, cmd);
			if (ingame)
			{
				game::Cbuf_AddText(0, utils::string::va("%s \n", cmd.data()));
			}
		}

		return 0;
	}

	class component final : public component_interface
	{
	public:
		component()
		{
			ShowWindow(GetConsoleWindow(), SW_SHOW);
		}

		void post_start() override
		{
			const auto handle = CreateThread(0, 0, console::console_thread, 0, 0, 0);
			utils::thread::set_name(handle, "Console");
		}

		void post_unpack() override
		{
			console::ingame = true;
		}

		void pre_destroy() override
		{
			console::ingame = false;
			console::exit = true;
		}
	};
}

REGISTER_COMPONENT(console::component)