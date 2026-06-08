#pragma once
#include "../pch.h"

// ============================================================
//  Shortcuts.h — Keyboard shortcut dispatch
//
//  ProcessShortcuts runs in the message loop and intercepts
//  WM_KEYDOWN messages before they reach TranslateMessage.
//  Add new hotkeys here without touching any other module.
// ============================================================

// Forward declaration — App is defined in App.h
class App;

namespace Shortcuts {

    // Called from the main message loop for every MSG.
    // Returns true if the message was consumed (skip Translate/Dispatch).
    bool Process(MSG* msg, App* app);

} // namespace Shortcuts
