#include "FileIO.h"
#include "../UI/Theme.h"

// ============================================================
//  FileIO.cpp — UTF-8 file reading and writing
// ============================================================

namespace FileIO {

    // --------------------------------------------------------
    //  LoadFile
    //  Reads a file as raw bytes, converts UTF-8 → UTF-16,
    //  and sets the RichEdit text. Handles empty files safely.
    // --------------------------------------------------------
    void LoadFile(HWND hEdit, LPCWSTR path, std::wstring& currentFilePath) {
        HANDLE hFile = CreateFile(
            path, GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

        if (hFile == INVALID_HANDLE_VALUE) return;

        DWORD size = GetFileSize(hFile, nullptr);
        if (size > 0) {
            std::vector<char> buf(size + 1, '\0');
            DWORD bytesRead = 0;
            if (ReadFile(hFile, buf.data(), size, &bytesRead, nullptr)) {
                // UTF-8 → UTF-16 conversion
                int wLen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), -1, nullptr, 0);
                std::vector<wchar_t> wBuf(wLen);
                MultiByteToWideChar(CP_UTF8, 0, buf.data(), -1, wBuf.data(), wLen);

                SetWindowText(hEdit, wBuf.data());

                // Stamp the default text color so imported plain text looks right
                CHARFORMAT2 cf = { sizeof(cf) };
                cf.dwMask      = CFM_COLOR;
                cf.crTextColor = Theme::COL_TEXT;
                SendMessage(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
            }
        } else {
            SetWindowText(hEdit, L"");
        }

        CloseHandle(hFile);
        currentFilePath = path;
    }

    // --------------------------------------------------------
    //  SaveFile
    //  Gets the full RichEdit content via EM_GETTEXTEX,
    //  converts UTF-16 → UTF-8, and writes it to disk.
    // --------------------------------------------------------
    void SaveFile(HWND hEdit, LPCWSTR path, std::wstring& currentFilePath) {
        HANDLE hFile = CreateFile(
            path, GENERIC_WRITE, 0,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (hFile == INVALID_HANDLE_VALUE) return;

        // Get text length (with CRLF line endings)
        GETTEXTLENGTHEX gtl = { GTL_DEFAULT | GTL_USECRLF, 1200 };
        int len = (int)SendMessage(hEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);

        std::vector<wchar_t> wBuf(len + 1, L'\0');
        GETTEXTEX gt = {
            (DWORD)((len + 1) * sizeof(wchar_t)),
            GT_DEFAULT | GT_USECRLF,
            1200, nullptr, nullptr
        };
        SendMessage(hEdit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)wBuf.data());

        // UTF-16 → UTF-8 conversion
        int uLen = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, nullptr, 0, nullptr, nullptr);
        if (uLen > 1) {  // uLen includes the null terminator
            std::vector<char> uBuf(uLen);
            WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, uBuf.data(), uLen, nullptr, nullptr);

            DWORD written = 0;
            WriteFile(hFile, uBuf.data(), (DWORD)(uLen - 1), &written, nullptr);
        }

        CloseHandle(hFile);
        currentFilePath = path;
    }

    // --------------------------------------------------------
    //  PromptOpen
    //  Shows the standard Open dialog filtered to .txt files.
    // --------------------------------------------------------
    void PromptOpen(HWND hOwner, HWND hEdit, std::wstring& currentFilePath) {
        wchar_t szFile[MAX_PATH] = { 0 };
        OPENFILENAME ofn         = { sizeof(ofn) };
        ofn.hwndOwner            = hOwner;
        ofn.lpstrFile            = szFile;
        ofn.nMaxFile             = MAX_PATH;
        ofn.lpstrFilter          = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.Flags                = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn)) {
            LoadFile(hEdit, ofn.lpstrFile, currentFilePath);
        }
    }

    // --------------------------------------------------------
    //  PromptSaveAs
    //  Shows the Save As dialog; auto-appends .txt if the user
    //  doesn't specify an extension.
    // --------------------------------------------------------
    void PromptSaveAs(HWND hOwner, HWND hEdit, std::wstring& currentFilePath) {
        wchar_t szFile[MAX_PATH] = { 0 };
        OPENFILENAME ofn         = { sizeof(ofn) };
        ofn.hwndOwner            = hOwner;
        ofn.lpstrFile            = szFile;
        ofn.nMaxFile             = MAX_PATH;
        ofn.lpstrFilter          = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.lpstrDefExt          = L"txt";
        ofn.Flags                = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn)) {
            SaveFile(hEdit, ofn.lpstrFile, currentFilePath);
        }
    }

} // namespace FileIO
