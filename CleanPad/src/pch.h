#pragma once

// ============================================================
//  pch.h — Shared Pre-Compiled Header for CleanPad
//  All Windows SDK includes and shared constants live here.
// ============================================================

// --- Windows targeting ---
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00  // Target Windows 10+
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// --- Windows Headers (order matters) ---
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <richedit.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>

// --- STL ---
#include <string>
#include <vector>

// --- Library Links ---
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

// ============================================================
//  Shared UI Constants
// ============================================================
#define ID_EDIT_CONTROL  101
#define TOP_BAR_HEIGHT   35
#define FILE_BTN_WIDTH   60

// --- Menu Command IDs ---
#define IDM_OPEN    2001
#define IDM_SAVE    2002
#define IDM_SAVEAS  2003

// --- Window Class Names ---
#define WC_CLEANPAD_MAIN  L"CleanPadClass"
#define WC_CLEANPAD_MENU  L"CleanPadMenuClass"

// --- App Name ---
#define APP_NAME  L"CleanPad"
