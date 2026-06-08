#include "Editor.h"
#include "../UI/Theme.h"

// ============================================================
//  Editor.cpp — RichEdit control implementation
// ============================================================

// Helper: stream callback used when copying RTF to clipboard
static DWORD CALLBACK RtfStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
    auto* pData = reinterpret_cast<std::vector<BYTE>*>(dwCookie);
    pData->insert(pData->end(), pbBuff, pbBuff + cb);
    *pcb = cb;
    return 0;
}

// ============================================================

Editor::Editor()
    : m_hEdit(nullptr)
    , m_zoomNumerator(100)
{}

// --------------------------------------------------------
//  Create
//  Loads the Msftedit.dll, creates the RichEdit control,
//  and applies default appearance settings.
// --------------------------------------------------------
bool Editor::Create(HWND hParent, HINSTANCE hInst) {
    // Load the modern RichEdit library (RichEdit 4.1+)
    LoadLibrary(L"Msftedit.dll");

    m_hEdit = CreateWindowEx(
        0, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_WANTRETURN,
        0, TOP_BAR_HEIGHT, 100, 100,   // sized properly in Resize()
        hParent, (HMENU)ID_EDIT_CONTROL, hInst, nullptr);

    if (!m_hEdit) return false;

    // --- Dark mode & theme ---
    Theme::EnableDarkMode(m_hEdit);

    // Wrap words to the window width (no horizontal scroll)
    SendMessage(m_hEdit, EM_SETTARGETDEVICE, NULL, 0);

    // Background colour
    SendMessage(m_hEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)Theme::COL_BG_MAIN);

    // Font
    SendMessage(m_hEdit, WM_SETFONT, (WPARAM)Theme::g_hFontEdit, TRUE);

    // Default text colour (applies to all existing and new text)
    ApplyDefaultColor();

    // We only need change notifications for dirty-flag tracking in future
    SendMessage(m_hEdit, EM_SETEVENTMASK, 0, ENM_CHANGE);

    return true;
}

// --------------------------------------------------------
//  Resize
//  Fills the client area below the top bar.
// --------------------------------------------------------
void Editor::Resize(int clientWidth, int clientHeight) {
    SetWindowPos(m_hEdit, nullptr,
        0, TOP_BAR_HEIGHT,
        clientWidth, clientHeight - TOP_BAR_HEIGHT,
        SWP_NOZORDER);
}

// --------------------------------------------------------
//  ToggleFormatting
//  Reads current selection format and XORs the requested
//  effect bit so repeated presses toggle on/off.
// --------------------------------------------------------
void Editor::ToggleFormatting(DWORD effect) {
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
    SendMessage(m_hEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    if (effect == CFM_BOLD)      cf.dwEffects ^= CFE_BOLD;
    if (effect == CFM_ITALIC)    cf.dwEffects ^= CFE_ITALIC;
    if (effect == CFM_UNDERLINE) cf.dwEffects ^= CFE_UNDERLINE;

    cf.dwMask = effect;
    SendMessage(m_hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// --------------------------------------------------------
//  ApplyDefaultColor
//  Stamps the theme text colour over all content. Called
//  after loading a file so imported plain text looks right.
// --------------------------------------------------------
void Editor::ApplyDefaultColor() {
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask      = CFM_COLOR;
    cf.crTextColor = Theme::COL_TEXT;
    SendMessage(m_hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

// --------------------------------------------------------
//  CopyToClipboard
//  Places both RTF and Unicode plain text on the clipboard
//  so the user can paste into rich or plain-text targets.
// --------------------------------------------------------
void Editor::CopyToClipboard(HWND hOwner) {
    if (!OpenClipboard(hOwner)) return;
    EmptyClipboard();

    // --- 1. RTF ---
    std::vector<BYTE> rtfData;
    EDITSTREAM esRTF = { (DWORD_PTR)&rtfData, 0, RtfStreamCallback };
    SendMessage(m_hEdit, EM_STREAMOUT, SF_RTF | SFF_SELECTION, (LPARAM)&esRTF);

    if (!rtfData.empty()) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, rtfData.size() + 1);
        if (hMem) {
            void* p = GlobalLock(hMem);
            if (p) {
                memcpy(p, rtfData.data(), rtfData.size());
                static_cast<char*>(p)[rtfData.size()] = '\0';
                GlobalUnlock(hMem);
                static UINT cfRTF = RegisterClipboardFormat(L"Rich Text Format");
                SetClipboardData(cfRTF, hMem);
            } else {
                GlobalFree(hMem);
            }
        }
    }

    // --- 2. Plain Unicode text (selected range only) ---
    CHARRANGE cr;
    SendMessage(m_hEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    int len = cr.cpMax - cr.cpMin;
    if (len > 0) {
        std::vector<wchar_t> buf(len + 1);
        TEXTRANGE tr = { cr, buf.data() };
        SendMessage(m_hEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
        if (hMem) {
            void* p = GlobalLock(hMem);
            if (p) {
                memcpy(p, buf.data(), (len + 1) * sizeof(wchar_t));
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            } else {
                GlobalFree(hMem);
            }
        }
    }

    CloseClipboard();
}

// --------------------------------------------------------
//  AdjustZoom
//  delta = +1 → zoom in 10%, delta = -1 → zoom out 10%.
//  Clamped to [10 %, 500 %].
// --------------------------------------------------------
void Editor::AdjustZoom(int delta) {
    m_zoomNumerator += delta * 10;
    if (m_zoomNumerator < 10)  m_zoomNumerator = 10;
    if (m_zoomNumerator > 500) m_zoomNumerator = 500;
    SendMessage(m_hEdit, EM_SETZOOM, (WPARAM)m_zoomNumerator, 100);
}
