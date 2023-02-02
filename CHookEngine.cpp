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
	}
	else
	{
		// TDA
		byte patternUItda[] = { 0x48, 0x8B, 0xC4, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0x68, 0x98, 0x48, 0x81, 0xEC, 0x30, 0x01, 0x00, 0x00, 0x48, 0xC7, 0x45, 0x98 };
		ptrUI = this->FindMemoryPattern("xxxxxxxxxxxxxxxxxxxxxxxxxxxxx", patternUItda, ((UINT_PTR)hmoduleOfProcess), 0x240000);
		if (ptrUI != 0)
		{
			m_logger->WriteLine("Pattern finder found location of TDA TranslateTextUI function: ").WritePointer(ptr);
		}
		else
		{
			m_logger->WriteLine("TranslateTextUI Function not found");
		}

	}

	// Magic
	m_logger->WriteLine("Hooking game");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (ptr != 0) DetourAttach(&(PVOID&)ptr, CGameFunctions::TranslateText);
	if (ptrUI != 0) DetourAttach(&(PVOID&)ptrUI, CGameFunctions::TranslateTextUI);

	if (DetourTransactionCommit() != ERROR_SUCCESS)
	{
		m_logger->WriteLine("DetourTransactionCommit failed ...");
		return FALSE;
	}

	// Save old functions
	CGameFunctions::Orig_TranslateText = (const char* (__fastcall*)(void*, void*, void*)) ptr;
	CGameFunctions::Orig_TranslateTextUI = (void (__fastcall*)(void*, const char*)) ptrUI;

	m_logger->WriteLine("Hooking done");

	return TRUE;
}

BOOL CHookEngine::UnhookGame()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (CGameFunctions::Orig_TranslateText != 0) DetourDetach(&(PVOID&)CGameFunctions::Orig_TranslateText, CGameFunctions::TranslateText);
	if (CGameFunctions::Orig_TranslateTextUI != 0) DetourDetach(&(PVOID&)CGameFunctions::Orig_TranslateTextUI, CGameFunctions::TranslateTextUI);

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
