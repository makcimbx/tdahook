#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include "CGameFunctions.h"
#include "encoding.h"
#include <experimental/filesystem>

char CGameFunctions::m_textBuffer[TEXT_BUFFER];

CResourceManager* CGameFunctions::m_resources = NULL;

const char* (__fastcall* CGameFunctions::Orig_TranslateText) (void*, void*, void*) = NULL;
void(__fastcall* CGameFunctions::Orig_TranslateTextUI) (void*, const char*) = NULL;
void* (__fastcall* CGameFunctions::Orig_CreateFont) (void*, const char*, const char*) = NULL;

void* __fastcall CGameFunctions::CreateFontHook(void* a1, const char* a2, const char* a3)
{
	auto pathToTtfInUTF16 = CUtils::GetGameDirectory(L"\\MLFont.ttf");
	auto fontExist = std::experimental::filesystem::exists(pathToTtfInUTF16);
	if (fontExist)
	{
		auto pathToNewFontInUTF8 = encoding::utf16_to_utf8(pathToTtfInUTF16);
		return Orig_CreateFont(a1, a2, pathToNewFontInUTF8.c_str());
	}
	else
	{
		return Orig_CreateFont(a1, a2, a3);
	}
}

const char* __fastcall CGameFunctions::TranslateText(void* a1, void* a2, void* a3)
{
	auto text = Orig_TranslateText(a1, a2, a3);

	bool result = m_resources->TranslateText(text, m_textBuffer, TEXT_BUFFER);

	if (result)
	{
		return m_textBuffer;
	}
	else
	{
		return text;
	}
}

void __fastcall CGameFunctions::TranslateTextUI(void* a1, const char* text)
{
	bool result = m_resources->TranslateUserInterface(text, m_textBuffer, TEXT_BUFFER);

	if (result)
	{
		return Orig_TranslateTextUI(a1, m_textBuffer);
	}
	else
	{
		return Orig_TranslateTextUI(a1, text);
	}
}