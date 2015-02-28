#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "ProjectConstants.h"

static void printf_MessageBox(const char* format, ...)
{
	va_list vl;
	va_start(vl, format);

	char buffer[4096];

	_vsnprintf_s(buffer, _countof(buffer), _countof(buffer) - 1, format, vl);

	MessageBoxA(NULL, buffer, PROJECT_FULL_NAME, MB_OK);
	va_end(vl);
}

static void printf_MessageBox(const wchar_t * format, ...)
{
	va_list vl;
	va_start(vl, format);

	wchar_t buffer[4096];

	_vsnwprintf_s(buffer, _countof(buffer), _countof(buffer) - 1, format, vl);

	MessageBoxW(NULL, buffer, L"MessageBox", MB_OK);
	va_end(vl);
}

static void OutputFormattedDebugString(const char* format, ...)
{
	va_list vl;
	va_start(vl, format);

	char buffer[4096];

	_vsnprintf_s(buffer, _countof(buffer), _countof(buffer) - 1, format, vl);

	OutputDebugString(buffer);
	va_end(vl);
}