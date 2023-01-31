#pragma once

#include <windows.h>
#pragma comment(lib, "detours.lib")
#include "detours.h"

#include "CLogger.h"
#include "CResourceManager.h"
#include "CGameFunctions.h"

struct HookInfo {
	int version;
	UINT_PTR address;
};

class CHookEngine {
public:
	BOOL HookGame();
	BOOL UnhookGame();

	void SetLogger(CLogger* logger) { m_logger = logger; };
	void SetResourceManager(CResourceManager* res) { m_res = res; };

private:
	CLogger* m_logger;
	CResourceManager* m_res;

	UINT_PTR FindMemoryPattern(const char* mask, byte* datamask, UINT_PTR start, UINT_PTR length);
	bool MemoryCompare(const byte* data, const byte* datamask, const char* mask);
};