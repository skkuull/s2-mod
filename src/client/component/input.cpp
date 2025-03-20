#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>

namespace input
{
	namespace
	{
		utils::hook::detour cl_char_event_hook;
		utils::hook::detour cl_key_event_hook;
		utils::hook::detour cl_mouse_move_hook;

		void cl_char_event_stub(const int local_client_num, const int key)
		{
			cl_char_event_hook.invoke<void>(local_client_num, key);
		}

		void cl_key_event_stub(const int local_client_num, const int key, const int down)
		{

			cl_key_event_hook.invoke<void>(local_client_num, key, down);
		}

		void cl_mouse_move_stub(const int local_client_num, int x, int y)
		{

			cl_mouse_move_hook.invoke<void>(local_client_num, x, y);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			cl_char_event_hook.create(SELECT_VALUE(0x140231E40, 0x1403E92A0), cl_char_event_stub);
			cl_key_event_hook.create(SELECT_VALUE(0x1402320C0, 0x1403E9500), cl_key_event_stub);

#ifdef DEBUG
			cl_mouse_move_hook.create(SELECT_VALUE(0x14016B140, 0x1402C8BF0), cl_mouse_move_stub);
#endif
		}
	};
}

REGISTER_COMPONENT(input::component)