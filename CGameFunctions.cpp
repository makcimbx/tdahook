#define _CRT_SECURE_NO_WARNINGS
#include "CGameFunctions.h"

bool CGameFunctions::m_fixCharacters = false;
char CGameFunctions::m_lastLine[TEXT_BUFFER];
int  CGameFunctions::m_lastLinePosition = 0;
char CGameFunctions::m_textBuffer[TEXT_BUFFER];

CResourceManager* CGameFunctions::m_resources = NULL;

const char* (__fastcall* CGameFunctions::Orig_TranslateText) (void*, void*, void*) = NULL;
void (__fastcall* CGameFunctions::Orig_TranslateTextUI) (void*, const char*) = NULL;

const char* __fastcall CGameFunctions::TranslateText(void* a1, void* a2, void* a3)
{
	auto text = Orig_TranslateText(a1, a2, a3);

	if (*text == '\x00' || (*text == '\x05' && *(text + 1) == '\x00'))
	{
		return text;
	}

	m_fixCharacters = false;
	strncpy(m_lastLine, text, TEXT_BUFFER);
	m_lastLinePosition = 0;
	bool result = m_resources->TranslateText(text, m_textBuffer, TEXT_BUFFER);

	if (result)
	{
		if (m_lastLine[0] == '\x81' && m_lastLine[1] == '\x79')
		{
			m_fixCharacters = true;
		}

		return m_textBuffer;
	}
	else
	{
		return text;
	}
}

void __fastcall CGameFunctions::TranslateTextUI(void* a1, const char* text)
{
	if (*text == '\x00' || (*text == '\x05' && *(text + 1) == '\x00'))
	{
		return Orig_TranslateTextUI(a1, text);
	}

	m_fixCharacters = false;
	strncpy(m_lastLine, text, TEXT_BUFFER);
	m_lastLinePosition = 0;
	bool result = m_resources->TranslateUserInterface(text, m_textBuffer, TEXT_BUFFER);

	if (result)
	{
		if (m_lastLine[0] == '\x81' && m_lastLine[1] == '\x79')
		{
			m_fixCharacters = true;
		}

		return Orig_TranslateTextUI(a1, m_textBuffer);
	}
	else
	{
		return Orig_TranslateTextUI(a1, text);
	}
}