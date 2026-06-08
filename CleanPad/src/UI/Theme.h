#pragma once
#include "../pch.h"

// ============================================================
//  Theme.h — Colors, Fonts, and Dark Mode
//
//  Expansion point for future Glassmorph/blur UI. To add new
//  themes: extend ThemeColors and add an Init variant.
// ============================================================

namespace Theme {

    // --- Color Palette (Dark theme defaults) ---
    extern const COLORREF COL_BG_MAIN;   // Editor background
    extern const COLORREF COL_BG_BAR;    // Top bar background
    extern const COLORREF COL_TEXT;      // Default text color
    extern const COLORREF COL_MENU_BG;   // Menu background
    extern const COLORREF COL_MENU_SEL;  // Menu hover/selected

    // --- Font handles (created once, shared by modules) ---
    extern HFONT g_hFontUI;    // Top-bar / menu font (bold, small)
    extern HFONT g_hFontEdit;  // Editor font (regular, larger)

    // Initialize both fonts based on screen DPI
    void InitFonts();

    // Release GDI font objects – call on WM_DESTROY
    void DestroyFonts();

    // Apply Windows 10/11 dark mode to a window
    void EnableDarkMode(HWND hWnd);

    // Show a themed warning dialog
    void ShowWarning(HWND hParent, HINSTANCE hInst, LPCWSTR text);

} // namespace Theme
