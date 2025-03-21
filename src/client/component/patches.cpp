#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace patches
{
	namespace
	{
		std::string GetExecutableName()
		{
			char path[MAX_PATH];
			if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0)
			{
				std::string fullPath(path);
				size_t pos = fullPath.find_last_of("\\/");
				return (pos == std::string::npos) ? fullPath : fullPath.substr(pos + 1);
			}
			return "";
		}

		utils::hook::detour relaunch_hook;
		void relaunch_stub(const char* filename, const char* params)
		{
			if (filename == "s2_sp64_ship.exe"s || filename == "s2_mp64_ship.exe"s)
			{
				static char buf[MAX_PATH];
				memset(buf, 0, sizeof(buf));

				auto exe_name = GetExecutableName();
				assert(exe_name.size() < MAX_PATH);
				memcpy(buf, exe_name.data(), exe_name.size());

				filename = buf;

				params = utils::string::va("%s -%s", params, game::environment::is_sp() ? "multiplayer" : "singleplayer");

				relaunch_hook.invoke<void>(filename, params);
				return;
			}

			relaunch_hook.invoke<void>(filename, params);
		}

		utils::hook::detour com_error_hook;
		void com_error_stub(const int error, const char* msg, ...)
		{
			char buffer[2048]{};
			va_list ap;

			va_start(ap, msg);
			vsnprintf_s(buffer, _TRUNCATE, msg, ap);
			va_end(ap);

			printf("Error: %s\n", buffer);

			com_error_hook.invoke<void>(error, "%s", buffer);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_load() override
		{
			relaunch_hook.create(SELECT_VALUE(0x1404B5170, 0x140715B10), relaunch_stub);
		}

		void post_unpack() override
		{
			com_error_hook.create(SELECT_VALUE(0x14043CA00, 0x140073AA0), com_error_stub);
		}
	};
}

REGISTER_COMPONENT(patches::component)