#pragma once
#include "pch.h"
#include "Editor/Editor.h"
#include "Utils/FileIO.h"

// ============================================================
//  App.h — Top-level application state and command methods
//
//  App owns all sub-modules and exposes named command methods
//  so that Shortcuts and the menu can call them without knowing
//  about Windows messages directly.
// ============================================================

class App {
public:
    App();

    // Initialize and show the window. Returns the WM_QUIT wParam.
    int Run(HINSTANCE hInstance, int nCmdShow);

    // ----------------------------------------------------------
    //  Commands — called by Shortcuts and Menu
    // ----------------------------------------------------------
    void Save();
    void SaveAs();
    void Open();
    void NewWindow();
    void ToggleAlwaysOnTop();
    void ToggleBold();
    void ToggleItalic();
    void ToggleUnderline();
    void CopyToClipboard();

    // Accessor for Shortcuts module to use SendMessage
    HWND GetEditHwnd() const { return m_editor.GetHwnd(); }

private:
    // --- Handles ---
    HWND      m_hMain;
    HINSTANCE m_hInst;

    // --- Sub-modules ---
    Editor m_editor;

    // --- State ---
    std::wstring m_currentFilePath;
    bool         m_isAlwaysOnTop;
    bool         m_isDragging;
    POINT        m_dragStartPoint;
    bool         m_isLoading = false;
    float        m_loadProgress = 0.0f;

    // --- Window management ---
    bool RegisterClasses();
    bool CreateMainWindow(int nCmdShow);

    // --- Message routing ---
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // --- WM_ handler helpers ---
    void OnCreate(HWND hWnd);
    void OnSize(int w, int h);
    void OnPaint(HWND hWnd);
    void OnLButtonUp(HWND hWnd, POINT clientPt);
    void OnDropFiles(HDROP hDrop);
    void OnMouseWheel(WPARAM wParam);
};
