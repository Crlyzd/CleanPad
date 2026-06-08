#pragma once
#include "../pch.h"

// ============================================================
//  Menu.h — Custom popup file menu
//
//  Registers and handles the CleanPadMenuClass window used for
//  the drop-down that appears when clicking the ● button.
//  Future: add more menu items by extending g_menuItems.
// ============================================================

namespace Menu {

    // Register the menu window class. Call once before using.
    void RegisterClass(HINSTANCE hInst);

    // The window procedure for the popup menu window.
    LRESULT CALLBACK MenuWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

} // namespace Menu
