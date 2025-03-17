#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace uwp
{
	HRESULT XStoreQueryGameLicenseResult(void* async, game::XStoreGameLicense* license)
	{
		utils::hook::invoke<HRESULT>(SELECT_VALUE(0x14063E0A4, 0x14088ECEC), async, license);
		license->isActive = true;
		license->isTrial = false;

		return S_OK;
	}

	class component final : public component_interface
	{
	public:
		void post_load() override
		{
			// Patch licnese check
			utils::hook::call(SELECT_VALUE(0x140037BF2, 0x14017D362), XStoreQueryGameLicenseResult);
		}

		component_priority priority() override
		{
			return component_priority::uwp;
		}
	};
}

REGISTER_COMPONENT(uwp::component)