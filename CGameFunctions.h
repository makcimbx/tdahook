#pragma once

#include "CResourceManager.h"
#include "CUtils.h"

#define TEXT_BUFFER 8192

class CGameFunctions {
public:
	// Common functions
	static const char* __fastcall TranslateText(void* a1, void* a2, void* a3);
	static void __fastcall TranslateTextUI(void* a1, const char* text);

	// rUGP real functions
	static const char* (__fastcall* Orig_TranslateText) (void*, void*, void*);
	static void (__fastcall* Orig_TranslateTextUI) (void*, const char*);

	static CResourceManager* m_resources;

private:
	static bool m_fixCharacters;
	static char m_lastLine[TEXT_BUFFER];
	static int  m_lastLinePosition;
	static char m_textBuffer[TEXT_BUFFER];
};