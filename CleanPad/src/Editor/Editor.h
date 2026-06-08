#pragma once
#include "../pch.h"

// ============================================================
//  Editor.h — RichEdit control wrapper
//
//  Owns the HWND for the MSFTEDIT rich-edit control and
//  exposes clean methods for text formatting, color, and
//  clipboard. Future: syntax highlighting, word-count, etc.
// ============================================================

class Editor {
public:
    Editor();

    // Create and attach the RichEdit control as a child of hParent.
    // Must be called during WM_CREATE of the main window.
    bool Create(HWND hParent, HINSTANCE hInst);

    // Resize the edit control to fill the client area below the bar.
    void Resize(int clientWidth, int clientHeight);

    // Retrieve the underlying HWND (needed by FileIO and App).
    HWND GetHwnd() const { return m_hEdit; }

    // --- Formatting ---
    // Toggle bold / italic / underline on the current selection.
    void ToggleFormatting(DWORD effect);

    // Apply the default text color to ALL text in the control.
    void ApplyDefaultColor();

    // --- Clipboard ---
    // Copy selection as both RTF and plain Unicode text.
    void CopyToClipboard(HWND hOwner);

    // --- Zoom (Ctrl+Wheel) ---
    // Delta: +1 = zoom in, -1 = zoom out.
    void AdjustZoom(int delta);

private:
    HWND m_hEdit;
    int  m_zoomNumerator;   // Current zoom % (denominator always 100)
};
