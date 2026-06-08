#include "Shortcuts.h"
#include "../App.h"

// ============================================================
//  Shortcuts.cpp — Keyboard shortcut dispatch
// ============================================================

namespace Shortcuts {

    // --------------------------------------------------------
    //  Process
    //  Checks each WM_KEYDOWN for a known hotkey and calls
    //  the matching App method. Returns true if consumed.
    //
    //  To add a new shortcut:
    //    1. Add a case here with the virtual-key code.
    //    2. Add the corresponding public method to App.
    // --------------------------------------------------------
    bool Process(MSG* msg, App* app) {
        if (msg->message != WM_KEYDOWN) return false;

        // ESC → Quit
        if (msg->wParam == VK_ESCAPE) {
            PostQuitMessage(0);
            return true;
        }

        bool ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;

        if (!ctrl) return false;

        switch (msg->wParam) {

        case 'S':
            // Ctrl+Shift+S → Save As; Ctrl+S → Save (or Save As if unsaved)
            shift ? app->SaveAs() : app->Save();
            return true;

        case 'O':
            app->Open();
            return true;

        case 'N':
            app->NewWindow();
            return true;

        case 'T':
            app->ToggleAlwaysOnTop();
            return true;

        case 'A':
            // Select All (native RichEdit behaviour)
            SendMessage(app->GetEditHwnd(), EM_SETSEL, 0, -1);
            return true;

        case 'B':
            app->ToggleBold();
            return true;

        case 'I':
            app->ToggleItalic();
            return true;

        case 'U':
            app->ToggleUnderline();
            return true;

        case 'C':
            app->CopyToClipboard();
            return true;
        }

        return false;
    }

} // namespace Shortcuts
