#pragma once
#include "../pch.h"
#include <thread>
#include <atomic>

#define WM_USER_FILE_LOAD_START (WM_USER + 101)
#define WM_USER_FILE_LOADED     (WM_USER + 102)

// ============================================================
//  FileIO.h — File loading, saving, and dialog helpers
//
//  All functions are free-standing so they can operate on any
//  HWND edit control. Future: add autosave, recent files, etc.
// ============================================================

namespace FileIO {
    struct LoadResult {
        std::wstring path;
        std::vector<wchar_t>* wBuf;
        DWORD fileSize;
    };

    // Load a UTF-8 file into the RichEdit control, then apply
    // the default color. Now runs asynchronously.
    void LoadFile(HWND hEdit, LPCWSTR path, std::wstring& currentFilePath);

    // Save RichEdit content as UTF-8. Updates currentFilePath.
    void SaveFile(HWND hEdit, LPCWSTR path, std::wstring& currentFilePath);

    // Show an Open File dialog and call LoadFile if confirmed.
    void PromptOpen(HWND hOwner, HWND hEdit, std::wstring& currentFilePath);

    // Show a Save As dialog and call SaveFile if confirmed.
    void PromptSaveAs(HWND hOwner, HWND hEdit, std::wstring& currentFilePath);

} // namespace FileIO
