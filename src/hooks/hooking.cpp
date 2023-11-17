#include "hooks/hooking.hpp"

#include "common.hpp"
#include "gui/gui.hpp"
#include "gui/renderer.hpp"
#include "memory/module.hpp"
#include "pointers.hpp"
#include "rorr/gm/CInstance_hooks.hpp"
#include "rorr/gm/Code_Execute_hook.hpp"

#include <MinHook.h>

namespace big
{
	static LPTOP_LEVEL_EXCEPTION_FILTER hook_SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
	{
		LOG(FATAL) << "incoming:" << HEX_TO_UPPER(lpTopLevelExceptionFilter);

		//const auto previous = g_hooking->get_original<hook_SetUnhandledExceptionFilter>()(lpTopLevelExceptionFilter);

		return nullptr;
	}

	hooking::hooking()
	{
		// The instances in that vector at this point should only be the "lazy" global variable hooks
		// aka the ones that don't have their m_target assigned
		for (auto& detour_hook_helper : m_detour_hook_helpers)
		{
			detour_hook_helper.m_detour_hook->set_target_and_create_hook(detour_hook_helper.m_on_hooking_available());
		}

		detour_hook_helper::add<hook_SetUnhandledExceptionFilter>("SUEF", &SetUnhandledExceptionFilter);

		detour_hook_helper::add<gm::hook_Code_Execute>("CE", g_pointers->m_rorr.m_code_execute);

		detour_hook_helper::add<gm::hook_CInstance_ctor>("CIC", g_pointers->m_rorr.m_cinstance_ctor);
		detour_hook_helper::add<gm::hook_CInstance_dctor>("CID", g_pointers->m_rorr.m_cinstance_dctor);

		detour_hook_helper::add<gm::hook_CObjectGM_AddInstance>("COGMAI", g_pointers->m_rorr.m_cobjectgm_add_instance);
		detour_hook_helper::add<gm::hook_CObjectGM_RemoveInstance>("COGMRI", g_pointers->m_rorr.m_cobjectgm_remove_instance);

		g_hooking = this;
	}

	hooking::~hooking()
	{
		if (m_enabled)
		{
			disable();
		}

		g_hooking = nullptr;
	}

	void hooking::enable()
	{
		for (auto& detour_hook_helper : m_detour_hook_helpers)
		{
			detour_hook_helper.m_detour_hook->enable();
		}

		MH_ApplyQueued();

		m_enabled = true;
	}

	void hooking::disable()
	{
		m_enabled = false;

		for (auto& detour_hook_helper : m_detour_hook_helpers)
		{
			detour_hook_helper.m_detour_hook->disable();
		}

		MH_ApplyQueued();

		m_detour_hook_helpers.clear();
	}

	hooking::detour_hook_helper::~detour_hook_helper()
	{
	}

	void hooking::detour_hook_helper::enable_hook_if_hooking_is_already_running()
	{
		if (g_hooking && g_hooking->m_enabled)
		{
			if (m_on_hooking_available)
			{
				m_detour_hook->set_target_and_create_hook(m_on_hooking_available());
			}

			m_detour_hook->enable();
			MH_ApplyQueued();
		}
	}
}
