#include <std_include.hpp>
#include "game.hpp"
#include "dvars.hpp"

#include <utils/flags.hpp>
#include <utils/string.hpp>
#include <utils/io.hpp>
#include <utils/nt.hpp>
#include <utils/json.hpp>

namespace game
{
	uint64_t base_address = (uint64_t)GetModuleHandleA(0);;

	auto* const cmd_textArray = reinterpret_cast<cmd_text*>(0x14BD83728);

	void Cbuf_AddText(int localClientNum, const char* text)
	{
		Sys_EnterCriticalSection(193);

		auto* cmd_texts = &cmd_textArray[localClientNum];
		auto text_length = static_cast<int>(strlen(text));

		if (cmd_texts->cmdsize + text_length < cmd_texts->maxsize)
		{
			memcpy(&cmd_texts->data[cmd_texts->cmdsize], text, text_length + 1);
			cmd_texts->cmdsize += text_length;
		}

		Sys_LeaveCriticalSection(193);
	}

	const auto list_json = utils::nt::load_resource(DVAR_LIST);
	const auto list = nlohmann::json::parse(list_json);

	void command_execute(int localClientNum, std::string text)
	{
		if (!list.is_array()) return;

		for (const auto& dvar_info : list)
		{
			if (dvar_info.is_array() && dvar_info.size() == 2)
			{
				const auto name = dvar_info[1].get<std::string>();    // name
				const auto dvar_id = dvar_info[0].get<std::string>(); // id

				if (!name.empty() && !dvar_id.empty())
				{
					text = utils::string::replace(utils::string::to_lower(text), name, dvar_id);
				}
			}
		}

		Cbuf_AddText(localClientNum, utils::string::va("%s \n", text.data()));
	}

	int Cmd_Argc()
	{
		return cmd_args->argc[cmd_args->nesting];
	}

	const char* Cmd_Argv(const int index)
	{
		return cmd_args->argv[cmd_args->nesting][index];
	}

	int SV_Cmd_Argc()
	{
		return sv_cmd_args->argc[sv_cmd_args->nesting];
	}

	const char* SV_Cmd_Argv(const int index)
	{
		return sv_cmd_args->argv[sv_cmd_args->nesting][index];
	}

	namespace environment
	{
		launcher::mode mode = launcher::mode::none;

		launcher::mode translate_surrogate(const launcher::mode _mode)
		{
			switch (_mode)
			{
			case launcher::mode::survival:
			case launcher::mode::zombies:
				return launcher::mode::multiplayer;
			default:
				return _mode;
			}
		}

		launcher::mode get_real_mode()
		{
			if (mode == launcher::mode::none)
			{
				throw std::runtime_error("Launcher mode not valid. Something must be wrong.");
			}

			return mode;
		}

		launcher::mode get_mode()
		{
			return translate_surrogate(get_real_mode());
		}

		bool is_sp()
		{
			return get_mode() == launcher::mode::singleplayer;
		}

		bool is_mp()
		{
			return get_mode() == launcher::mode::multiplayer;
		}

		bool is_dedi()
		{
			return get_mode() == launcher::mode::server;
		}

		void set_mode(const launcher::mode _mode)
		{
			mode = _mode;
		}

		std::string get_string()
		{
			const auto current_mode = get_real_mode();
			switch (current_mode)
			{
			case launcher::mode::server:
				return "Dedicated Server";

			case launcher::mode::multiplayer:
				return "Multiplayer";

			case launcher::mode::zombies:
				return "Zombies";

			case launcher::mode::singleplayer:
				return "Singleplayer";

			case launcher::mode::none:
				return "None";

			default:
				return "Unknown (" + std::to_string(static_cast<int>(mode)) + ")";
			}
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
