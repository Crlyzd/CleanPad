#pragma once
#include "../pch.h"

// ============================================================
//  FileIO.h — File loading, saving, and dialog helpers
//
//  All functions are free-standing so they can operate on any
//  HWND edit control. Future: add autosave, recent files, etc.
// ============================================================

namespace FileIO {

    // Load a UTF-8 file into the RichEdit control, then apply
    // default text color. Updates currentFilePath on success.
    void LoadFile(HWND hEdit, LPCWSTR path, std::wstring& currentFilePath);

    // Save RichEdit content as UTF-8. Updates currentFilePath.
    void SaveFile(HWND hEdit, LPCWSTR path, std::wstring& currentFilePath);

    // Show an Open File dialog and call LoadFile if confirmed.
    void PromptOpen(HWND hOwner, HWND hEdit, std::wstring& currentFilePath);

    // Show a Save As dialog and call SaveFile if confirmed.
    void PromptSaveAs(HWND hOwner, HWND hEdit, std::wstring& currentFilePath);

} // namespace FileIO
