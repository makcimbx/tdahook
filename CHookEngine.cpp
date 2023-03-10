#include "CHookEngine.h"
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>

BOOL CHookEngine::HookGame()
{
	CGameFunctions::m_resources = this->m_res;

	auto process = GetCurrentProcess();
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	m_logger->WriteLine("Initializing hook");
	HMODULE hmoduleOfProcess = GetModuleHandle(0);

	byte pattern[] = { 0x44, 0x8B , 0x09 , 0xE9 , 0x08 , 0x00 , 0x00 , 0x00  , 0xCC , 0xCC , 0xCC , 0xCC , 0xCC , 0xCC , 0xCC , 0xCC };
	UINT_PTR ptr = this->FindMemoryPattern("xxxxx???xxxxxxxx", pattern, ((UINT_PTR)hmoduleOfProcess), 0x360000);
	if (ptr != 0)
	{
		m_logger->WriteLine("Pattern finder found location of TranslateText function: ").WritePointer(ptr);
	}
	else
	{
		m_logger->WriteLine("TranslateText Function not found");
	}

	// Total Eclipse
	byte patternUI[] = { 0x48, 0x8B, 0xC4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0x68, 0x88, 0x48, 0x81, 0xEC, 0x40, 0x01, 0x00, 0x00, 0x48, 0xC7, 0x45, 0xA8 };
	UINT_PTR ptrUI = this->FindMemoryPattern("xxxxxxxxxxxxxxxxxxxxxxxxxxxxx", patternUI, ((UINT_PTR)hmoduleOfProcess), 0x240000);
	if (ptrUI != 0)
	{
		m_logger->WriteLine("Pattern finder found location of TE TranslateTextUI function: ").WritePointer(ptrUI);
		// TE 1.0.27
		DWORD dwProtect;
		auto backlogptr = ((UINT_PTR)hmoduleOfProcess) + 0x35A846;
		if (VirtualProtect((PVOID&)backlogptr, 1, PAGE_EXECUTE_READWRITE, &dwProtect))
		{
			memset((PVOID&)backlogptr, 0x66, 1);
			VirtualProtect((PVOID&)backlogptr, 1, dwProtect, &dwProtect);
			m_logger->WriteLine("TE Backlog Fixed");
		}
		else
		{
			m_logger->WriteLine("TE Backlog Fix Failed");
		}
	}
	else
	{
		// TDA
		byte patternUItda[] = { 0x48, 0x8B, 0xC4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0x68, 0x98, 0x48, 0x81, 0xEC, 0x30, 0x01, 0x00, 0x00, 0x48, 0xC7, 0x45, 0x98 };
		ptrUI = this->FindMemoryPattern("xxxxxxxxxxxxxxxxxxxxxxxxxxxxx", patternUItda, ((UINT_PTR)hmoduleOfProcess), 0x240000);
		if (ptrUI != 0)
		{
			m_logger->WriteLine("Pattern finder found location of TDA TranslateTextUI function: ").WritePointer(ptrUI);
		}
		else
		{
			m_logger->WriteLine("TranslateTextUI Function not found");
		}

	}

	// TDA Font
	byte patternFont[] = { 0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C, 0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x48, 0x8B, 0x41, 0x10, 0x49, 0x8B, 0xF8, 0x48, 0x8B, 0xDA, 0x48, 0x8B };
	UINT_PTR ptrFont = this->FindMemoryPattern("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", patternFont, ((UINT_PTR)hmoduleOfProcess), 0x240000);
	if (ptrFont != 0)
	{
		m_logger->WriteLine("Pattern finder found location of TDA CreateFont function: ").WritePointer(ptrFont);
	}
	else
	{
		// TE
		byte patternFontTE[] = { 0x4C, 0x8B, 0xDC, 0x57, 0x48, 0x83, 0xEC, 0x60, 0x49, 0xC7, 0x43, 0xC8, 0xFE, 0xFF, 0xFF, 0xFF, 0x49, 0x89, 0x5B, 0x20, 0x48, 0x8B, 0x05, 0xF5, 0xE8, 0x8C, 0x00, 0x48, 0x33, 0xC4, 0x48, 0x89 };
		ptrFont = this->FindMemoryPattern("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", patternFontTE, ((UINT_PTR)hmoduleOfProcess), 0x200000);
		if (ptrFont != 0)
		{
			m_logger->WriteLine("Pattern finder found location of TE CreateFont function: ").WritePointer(ptrFont);
		}
		else
		{
			// TDA New Font
			byte patternFontTDA[] = { 0x40, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x48, 0xC7, 0x44, 0x24, 0x20, 0xFE, 0xFF, 0xFF, 0xFF, 0x48, 0x89, 0x5C, 0x24, 0x78, 0x48, 0x8B, 0x05, 0xC5, 0x5E, 0x71, 0x00, 0x48, 0x33, 0xC4, 0x48, 0x89 };
			ptrFont = this->FindMemoryPattern("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", patternFontTDA, ((UINT_PTR)hmoduleOfProcess), 0x240000);
			if (ptrFont != 0)
			{
				m_logger->WriteLine("Pattern finder found location of TDA CreateFont function: ").WritePointer(ptrFont);
			}
			else
			{
				m_logger->WriteLine("CreateFont Function not found");
			}
		}
	}

	// Magic
	m_logger->WriteLine("Hooking game");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (ptr != 0) DetourAttach(&(PVOID&)ptr, CGameFunctions::TranslateText);
	if (ptrUI != 0) DetourAttach(&(PVOID&)ptrUI, CGameFunctions::TranslateTextUI);
	if (ptrFont != 0) DetourAttach(&(PVOID&)ptrFont, CGameFunctions::CreateFontHook);

	if (DetourTransactionCommit() != ERROR_SUCCESS)
	{
		m_logger->WriteLine("DetourTransactionCommit failed ...");
		return FALSE;
	}

	// Save old functions
	CGameFunctions::Orig_TranslateText = (const char* (__fastcall*)(void*, void*, void*)) ptr;
	CGameFunctions::Orig_TranslateTextUI = (void (__fastcall*)(void*, const char*)) ptrUI;
	CGameFunctions::Orig_CreateFont = (void* (__fastcall*)(void*, const char*, const char*)) ptrFont;

	m_logger->WriteLine("Hooking done");

	return TRUE;
}

BOOL CHookEngine::UnhookGame()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (CGameFunctions::Orig_TranslateText != 0) DetourDetach(&(PVOID&)CGameFunctions::Orig_TranslateText, CGameFunctions::TranslateText);
	if (CGameFunctions::Orig_TranslateTextUI != 0) DetourDetach(&(PVOID&)CGameFunctions::Orig_TranslateTextUI, CGameFunctions::TranslateTextUI);
	if (CGameFunctions::Orig_CreateFont != 0) DetourDetach(&(PVOID&)CGameFunctions::Orig_CreateFont, CGameFunctions::CreateFontHook);

	return (DetourTransactionCommit() == ERROR_SUCCESS);
}

// Based on www.gamedeception.net/threads/20592-Incredible-optimized-FindPattern
bool CHookEngine::MemoryCompare(const byte* data, const byte* datamask, const char* mask)
{
	for (; *mask; ++data, ++datamask, ++mask)
	{
		if (!strcmp(mask, "xxxx"))
		{
			if (*(UINT32*)data != *(UINT32*)datamask)
			{
				return FALSE;
			}

			data += 3, datamask += 3, mask += 3;
			continue;
		}

		if (!strcmp(mask, "xx"))
		{
			if (*(UINT16*)data != *(UINT16*)datamask)
			{
				return FALSE;
			}

			data++, datamask++, mask++;
			continue;
		}

		if (*mask == 'x' && *data != *datamask)
		{
			return false;
		}
	}

	return (*mask) == 0;
}

UINT_PTR CHookEngine::FindMemoryPattern(const char* mask, byte* datamask, UINT_PTR start, UINT_PTR length)
{
	UINT_PTR end = start + length;

	for (UINT_PTR i = start; i < end; i++)
	{
		if (this->MemoryCompare((byte*)i, datamask, mask))
		{
			return i;
		}
	}

	return 0;
}
