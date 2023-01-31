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
	UINT_PTR ptr = this->FindMemoryPattern("xxxxx???xxxxxxxx", pattern, ((UINT_PTR)hmoduleOfProcess), 0x320000);
	if (ptr != 0)
	{
		m_logger->WriteLine("Pattern finder found location of TranslateText function: ").WritePointer(ptr);
	}
	else
	{
		m_logger->WriteLine("TranslateText Function not found");
	}

	// Magic
	m_logger->WriteLine("Hooking game");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (ptr != 0) DetourAttach(&(PVOID&)ptr, CGameFunctions::TranslateText);

	if (DetourTransactionCommit() != ERROR_SUCCESS)
	{
		m_logger->WriteLine("DetourTransactionCommit failed ...");
		return FALSE;
	}

	// Save old functions
	CGameFunctions::Orig_TranslateText = (const char* (__fastcall*)(void*, void*, void*)) ptr;

	m_logger->WriteLine("Hooking done");

	return TRUE;
}

BOOL CHookEngine::UnhookGame()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (CGameFunctions::Orig_TranslateText != 0) DetourDetach(&(PVOID&)CGameFunctions::Orig_TranslateText, CGameFunctions::TranslateText);

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
