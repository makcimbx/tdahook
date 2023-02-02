#pragma once

#include "CResourceManager.h"
#include "CUtils.h"

#define TEXT_BUFFER 8192

class CGameFunctions {
public:
	static const char* __fastcall TranslateText(void* a1, void* a2, void* a3);
	static void __fastcall TranslateTextUI(void* a1, const char* text);

	static const char* (__fastcall* Orig_TranslateText) (void*, void*, void*);
	static void (__fastcall* Orig_TranslateTextUI) (void*, const char*);

	static CResourceManager* m_resources;

private:
	static char m_textBuffer[TEXT_BUFFER];
};