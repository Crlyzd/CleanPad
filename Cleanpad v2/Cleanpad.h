#pragma once

// 1. Windows Compatibility Definitions
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef UNICODE
#define UNICODE
#endif

// 2. Windows Headers (Order is critical!)
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <richedit.h>
#include <string>
#include <vector>

// 3. Logic Constants
#define ID_EDIT_CONTROL 101
#define TOP_BAR_HEIGHT 35
#define FILE_BTN_WIDTH 60

class Cleanpad {
public:
    Cleanpad();
    ~Cleanpad();

    // The only public function needed to start the app
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    // Window Handles & State
    HWND m_hMain;
    HWND m_hEdit;
    HINSTANCE m_hInst;
    std::wstring m_currentFilePath;

    // UI Resources
    HFONT m_hFontUI;
    HFONT m_hFontEdit;
    bool m_isAlwaysOnTop;
    bool m_isDragging;
    POINT m_dragStartPoint;

    // Core Logic Helpers
    void EnableDarkMode(HWND hWnd);

    // File Operations
    void LoadFile(LPCWSTR path);
    void SaveFile(LPCWSTR path);
    void PromptOpen();
    void PromptSaveAs();

    // Editor Tools
    void ToggleFormatting(DWORD effect);
    void CopyToClipboard();
    void ApplyTextColor();

    // Message Handlers
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Instance Message Handler
    LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Keyboard Shortcuts
    bool ProcessShortcuts(MSG* msg);
};