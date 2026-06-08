#include "Menu.h"
#include "Theme.h"

// ============================================================
//  Menu.cpp — Custom Popup Menu window
//
//  To add more items: append to g_menuItems[] and adjust the
//  window height in Window.cpp where the menu is spawned.
// ============================================================

namespace Menu {

    // --- Menu item definitions (label + command ID) ---
    struct MenuItem { const wchar_t* label; int cmdId; };
    static const MenuItem g_menuItems[] = {
        { L"Open",    IDM_OPEN   },
        { L"Save",    IDM_SAVE   },
        { L"Save As", IDM_SAVEAS },
    };
    static const int ITEM_COUNT  = ARRAYSIZE(g_menuItems);
    static const int ITEM_HEIGHT = 30;   // px per menu row

    // --------------------------------------------------------
    //  RegisterClass
    //  Must be called once from App::Init() before the menu
    //  window is ever spawned.
    // --------------------------------------------------------
    void RegisterClass(HINSTANCE hInst) {
        WNDCLASSEXW wcex    = { sizeof(wcex) };
        wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
        wcex.lpfnWndProc    = MenuWndProc;
        wcex.hInstance      = hInst;
        wcex.hbrBackground  = CreateSolidBrush(Theme::COL_MENU_BG);
        wcex.lpszClassName  = WC_CLEANPAD_MENU;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wcex);
    }

    // --------------------------------------------------------
    //  MenuWndProc
    //  Handles painting, hover highlight, click dispatch, and
    //  auto-close on focus loss or keyboard input.
    // --------------------------------------------------------
    LRESULT CALLBACK MenuWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Track which item the mouse is hovering over (-1 = none)
        static int s_hoverIndex = -1;

        switch (msg) {

        case WM_CREATE:
            // Capture mouse so we receive clicks outside the menu
            SetCapture(hWnd);
            s_hoverIndex = -1;
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            // --- Background ---
            HBRUSH hBg = CreateSolidBrush(Theme::COL_MENU_BG);
            FillRect(hdc, &rcClient, hBg);
            DeleteObject(hBg);

            SetBkMode(hdc, TRANSPARENT);

            // Use a locally-created font matching UI font spec
            // (Theme::g_hFontUI cannot be used safely across threads)
            HFONT hFont = CreateFont(
                -12, 0, 0, 0, FW_BOLD, 0, 0, 0,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Noto Sans");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            // --- Draw each menu item ---
            for (int i = 0; i < ITEM_COUNT; i++) {
                RECT rcItem = { 15, i * ITEM_HEIGHT, rcClient.right, (i + 1) * ITEM_HEIGHT };

                if (i == s_hoverIndex) {
                    // Highlight fill (full width)
                    RECT rcFill = { 0, i * ITEM_HEIGHT, rcClient.right, (i + 1) * ITEM_HEIGHT };
                    HBRUSH hSel = CreateSolidBrush(Theme::COL_MENU_SEL);
                    FillRect(hdc, &rcFill, hSel);
                    DeleteObject(hSel);
                    SetTextColor(hdc, RGB(255, 255, 255));
                } else {
                    SetTextColor(hdc, Theme::COL_TEXT);
                }

                DrawText(hdc, g_menuItems[i].label, -1, &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc;
            GetClientRect(hWnd, &rc);
            int newIdx = PtInRect(&rc, pt) ? (pt.y / ITEM_HEIGHT) : -1;
            // Clamp to valid range
            if (newIdx >= ITEM_COUNT) newIdx = -1;
            if (newIdx != s_hoverIndex) {
                s_hoverIndex = newIdx;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

        case WM_LBUTTONUP: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc;
            GetClientRect(hWnd, &rc);
            if (PtInRect(&rc, pt)) {
                int idx = pt.y / ITEM_HEIGHT;
                if (idx >= 0 && idx < ITEM_COUNT) {
                    // Post the command to the parent (main window)
                    PostMessage(GetParent(hWnd), WM_COMMAND, g_menuItems[idx].cmdId, 0);
                }
            }
            ReleaseCapture();
            DestroyWindow(hWnd);
            break;
        }

        case WM_LBUTTONDOWN: {
            // Click outside menu area → dismiss
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc;
            GetClientRect(hWnd, &rc);
            if (!PtInRect(&rc, pt)) {
                ReleaseCapture();
                DestroyWindow(hWnd);
            }
            break;
        }

        case WM_KEYDOWN:
        case WM_KILLFOCUS:
            ReleaseCapture();
            DestroyWindow(hWnd);
            break;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

} // namespace Menu
