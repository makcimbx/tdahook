#define _CRT_SECURE_NO_WARNINGS
#include "CGameFunctions.h"

bool CGameFunctions::m_fixCharacters = false;
char CGameFunctions::m_lastLine[TEXT_BUFFER];
int  CGameFunctions::m_lastLinePosition = 0;
char CGameFunctions::m_textBuffer[TEXT_BUFFER];

CResourceManager* CGameFunctions::m_resources = NULL;

const char* (__fastcall* CGameFunctions::Orig_TranslateText) (void*, void*, void*) = NULL;

const char* __fastcall CGameFunctions::TranslateText(void* a1, void* a2, void* a3)
{
	auto text = Orig_TranslateText(a1, a2, a3);

	std::cout << "Text: " << text << '\n';

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