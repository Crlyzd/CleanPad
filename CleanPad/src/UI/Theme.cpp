#include "Theme.h"

// ============================================================
//  Theme.cpp — Color constants, font management, dark mode
// ============================================================

namespace Theme {

    // --- Color Definitions ---
    const COLORREF COL_BG_MAIN  = RGB(30,  30,  30);
    const COLORREF COL_BG_BAR   = RGB(18,  18,  18);
    const COLORREF COL_TEXT     = RGB(180, 180, 180);
    const COLORREF COL_MENU_BG  = RGB(45,  45,  48);
    const COLORREF COL_MENU_SEL = RGB(80,  80,  80);

    // --- Font Handles (start NULL; set by InitFonts) ---
    HFONT g_hFontUI   = nullptr;
    HFONT g_hFontEdit = nullptr;

    // --------------------------------------------------------
    //  InitFonts
    //  Creates DPI-aware fonts used across the whole app.
    //  Call once during WM_CREATE / App::Init().
    // --------------------------------------------------------
    void InitFonts() {
        HDC hdc = GetDC(nullptr);
        // Point sizes → logical pixels (DPI-aware)
        long pxEdit = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        long pxUI   = -MulDiv(9,  GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(nullptr, hdc);

        // UI font: bold, small — used in the top bar
        g_hFontUI = CreateFont(
            pxUI, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Noto Sans");

        // Edit font: normal weight — used in the RichEdit control
        g_hFontEdit = CreateFont(
            pxEdit, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Noto Sans");
    }

    // --------------------------------------------------------
    //  DestroyFonts
    //  Release GDI objects. Call on WM_DESTROY.
    // --------------------------------------------------------
    void DestroyFonts() {
        if (g_hFontUI)   { DeleteObject(g_hFontUI);   g_hFontUI   = nullptr; }
        if (g_hFontEdit) { DeleteObject(g_hFontEdit); g_hFontEdit = nullptr; }
    }

    // --------------------------------------------------------
    //  EnableDarkMode
    //  Applies Win10/11 dark mode to a window via uxtheme.dll
    //  private ordinals (safe — checked before calling).
    // --------------------------------------------------------
    void EnableDarkMode(HWND hWnd) {
        HMODULE hUx = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (hUx) {
            using FnSetMode  = DWORD(WINAPI*)(int);
            using FnAllowWnd = BOOL(WINAPI*)(HWND, BOOL);

            auto SetMode  = (FnSetMode) GetProcAddress(hUx, MAKEINTRESOURCEA(135));
            auto AllowWnd = (FnAllowWnd)GetProcAddress(hUx, MAKEINTRESOURCEA(133));

            if (SetMode)  SetMode(2);            // ForceDark
            if (AllowWnd) AllowWnd(hWnd, TRUE);
            FreeLibrary(hUx);
        }
        SetWindowTheme(hWnd, L"DarkMode_Explorer", nullptr);

        // DWM dark title bar (Windows 11)
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    }

} // namespace Theme
