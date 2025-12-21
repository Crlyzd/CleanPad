#include "Cleanpad.h"
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

// --- Colors ---
const COLORREF COL_BG_MAIN = RGB(30, 30, 30);
const COLORREF COL_BG_BAR = RGB(18, 18, 18);
const COLORREF COL_TEXT = RGB(180, 180, 180);
const COLORREF COL_MENU_BG = RGB(45, 45, 48);
const COLORREF COL_MENU_SEL = RGB(80, 80, 80);

// --- Menu IDs ---
#define IDM_OPEN   2001
#define IDM_SAVE   2002
#define IDM_SAVEAS 2003

Cleanpad::Cleanpad()
    : m_hMain(NULL), m_hEdit(NULL), m_hInst(NULL),
    m_hFontUI(NULL), m_hFontEdit(NULL),
    m_isAlwaysOnTop(false), m_isDragging(false),
    m_dragStartPoint({ 0, 0 })
{
    // Create Fonts immediately or in InitializeUI
    HDC hdc = GetDC(NULL);
    long lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    long lfHeightUI = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    m_hFontUI = CreateFont(lfHeightUI, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Noto Sans");

    m_hFontEdit = CreateFont(lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Noto Sans");
}

Cleanpad::~Cleanpad() {
    if (m_hFontUI) DeleteObject(m_hFontUI);
    if (m_hFontEdit) DeleteObject(m_hFontEdit);
}

// --- Dark Mode Logic [cite: 6-9] ---
void Cleanpad::EnableDarkMode(HWND hWnd) {
    HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxtheme) {
        typedef DWORD(WINAPI* FnSetPreferredAppMode)(int appMode);
        typedef BOOL(WINAPI* FnAllowDarkModeForWindow)(HWND hWnd, BOOL allow);

        auto SetPreferredAppMode = (FnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        auto AllowDarkModeForWindow = (FnAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));

        if (SetPreferredAppMode) SetPreferredAppMode(2);
        if (AllowDarkModeForWindow) AllowDarkModeForWindow(hWnd, TRUE);
        FreeLibrary(hUxtheme);
    }
    SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
}

// --- File I/O (Modernized) [cite: 31-50] ---
void Cleanpad::LoadFile(LPCWSTR path) {
    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD size = GetFileSize(hFile, NULL);
    if (size > 0) {
        std::vector<char> buffer(size + 1); // Safer than new char[]
        DWORD read;
        if (ReadFile(hFile, buffer.data(), size, &read, NULL)) {
            buffer[size] = 0;
            int wLen = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, NULL, 0);
            std::vector<wchar_t> wBuffer(wLen);
            MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, wBuffer.data(), wLen);

            SetWindowText(m_hEdit, wBuffer.data());
            ApplyTextColor();
        }
    }
    else {
        SetWindowText(m_hEdit, L"");
    }
    CloseHandle(hFile);
    m_currentFilePath = path;
}

void Cleanpad::SaveFile(LPCWSTR path) {
    HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    GETTEXTLENGTHEX gtl = { GTL_DEFAULT | GTL_USECRLF, 1200 };
    int len = (int)SendMessage(m_hEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);

    std::vector<wchar_t> wBuffer(len + 1);
    GETTEXTEX gt = { (DWORD)((len + 1) * sizeof(wchar_t)), GT_DEFAULT | GT_USECRLF, 1200, NULL, NULL };

    SendMessage(m_hEdit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)wBuffer.data());

    int uLen = WideCharToMultiByte(CP_UTF8, 0, wBuffer.data(), -1, NULL, 0, NULL, NULL);
    if (uLen > 0) {
        std::vector<char> uBuffer(uLen);
        WideCharToMultiByte(CP_UTF8, 0, wBuffer.data(), -1, uBuffer.data(), uLen, NULL, NULL);

        DWORD written;
        if (uLen > 1) WriteFile(hFile, uBuffer.data(), uLen - 1, &written, NULL);
    }
    CloseHandle(hFile);
    m_currentFilePath = path;
}

// --- Clipboard (Modernized) [cite: 15-31] ---
DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
    std::vector<BYTE>* pData = (std::vector<BYTE>*)dwCookie;
    pData->insert(pData->end(), pbBuff, pbBuff + cb);
    *pcb = cb;
    return 0;
}

void Cleanpad::CopyToClipboard() {
    if (!OpenClipboard(m_hMain)) return;
    EmptyClipboard();

    // 1. RTF
    std::vector<BYTE> rtfData;
    EDITSTREAM esRTF = { (DWORD_PTR)&rtfData, 0, EditStreamCallback };
    SendMessage(m_hEdit, EM_STREAMOUT, SF_RTF | SFF_SELECTION, (LPARAM)&esRTF);

    if (!rtfData.empty()) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, rtfData.size() + 1);
        if (hMem) {
            void* pMem = GlobalLock(hMem);
            if (pMem) {
                memcpy(pMem, rtfData.data(), rtfData.size());
                ((char*)pMem)[rtfData.size()] = 0;
                GlobalUnlock(hMem);
                static UINT cfRTF = RegisterClipboardFormat(L"Rich Text Format");
                SetClipboardData(cfRTF, hMem);
            }
            else GlobalFree(hMem);
        }
    }

    // 2. Plain Text
    CHARRANGE cr;
    SendMessage(m_hEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    int len = cr.cpMax - cr.cpMin;
    if (len > 0) {
        std::vector<wchar_t> buffer(len + 1);
        TEXTRANGE tr = { cr, buffer.data() };
        SendMessage(m_hEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
        if (hMem) {
            void* pMem = GlobalLock(hMem);
            if (pMem) {
                memcpy(pMem, buffer.data(), (len + 1) * sizeof(wchar_t));
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
            else GlobalFree(hMem);
        }
    }
    CloseClipboard();
}

// --- UI Helpers ---
void Cleanpad::ApplyTextColor() {
    CHARFORMAT2 cf = { sizeof(cf) };
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = COL_TEXT;
    SendMessage(m_hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

void Cleanpad::ToggleFormatting(DWORD effect) {
    CHARFORMAT2 cf = { sizeof(cf) };
    cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
    SendMessage(m_hEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    if (effect == CFM_BOLD) cf.dwEffects ^= CFE_BOLD;
    if (effect == CFM_ITALIC) cf.dwEffects ^= CFE_ITALIC;
    if (effect == CFM_UNDERLINE) cf.dwEffects ^= CFE_UNDERLINE;

    cf.dwMask = effect;
    SendMessage(m_hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// --- Shortcuts [cite: 121-132] ---
bool Cleanpad::ProcessShortcuts(MSG* msg) {
    if (msg->message != WM_KEYDOWN) return false;
    if (msg->wParam == VK_ESCAPE) { PostQuitMessage(0); return true; }

    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (!ctrl) return false;

    switch (msg->wParam) {
    case 'T':
        m_isAlwaysOnTop = !m_isAlwaysOnTop;
        SetWindowPos(m_hMain, m_isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return true;
    case 'S': m_currentFilePath.empty() ? PromptSaveAs() : SaveFile(m_currentFilePath.c_str()); return true;
    case 'O': PromptOpen(); return true;
    case 'A': SendMessage(m_hEdit, EM_SETSEL, 0, -1); return true;
    case 'B': ToggleFormatting(CFM_BOLD); return true;
    case 'I': ToggleFormatting(CFM_ITALIC); return true;
    case 'U': ToggleFormatting(CFM_UNDERLINE); return true;
    case 'C': CopyToClipboard(); return true;
    }
    return false;
}

// --- Main Logic & Thunk ---

int Cleanpad::Run(HINSTANCE hInstance, int nCmdShow) {
    m_hInst = hInstance;
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    // Register Main Window Class
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance,
                         hIcon, LoadCursor(NULL, IDC_ARROW), // <--- Uses hIcon
                         CreateSolidBrush(COL_BG_MAIN), NULL, L"CleanpadClass",
                         hIcon }; // <--- Uses hIcon (Small)
    RegisterClassExW(&wcex);

    // Register Menu Window Class
    WNDCLASSEXW wcexMenu = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW, MenuWndProc, 0, 0, hInstance,
                             NULL, LoadCursor(NULL, IDC_ARROW), CreateSolidBrush(COL_MENU_BG), NULL, L"CleanpadMenuClass", NULL };
    RegisterClassExW(&wcexMenu);

    // --- CENTERING LOGIC ---
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 300;
    int winH = 376;
    int xPos = (screenW - winW) / 2;
    int yPos = (screenH - winH) / 2;
    // -----------------------

    // Create the Window at the calculated X, Y position
    m_hMain = CreateWindowW(L"CleanpadClass", L"Cleanpad", WS_POPUP | WS_THICKFRAME | WS_VISIBLE,
        xPos, yPos, winW, winH, NULL, NULL, hInstance, this);

    if (!m_hMain) return FALSE;

    ShowWindow(m_hMain, nCmdShow);
    UpdateWindow(m_hMain);
    
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1) {
        LoadFile(argv[1]);
    }
    LocalFree(argv);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!ProcessShortcuts(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

// Static wrapper to get the class instance
LRESULT CALLBACK Cleanpad::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Cleanpad* pApp = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pApp = (Cleanpad*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pApp);
    }
    else {
        pApp = (Cleanpad*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (pApp) return pApp->HandleMessage(hWnd, message, wParam, lParam);
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// The Real Logic Handler
LRESULT Cleanpad::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        DragAcceptFiles(hWnd, TRUE);
        LoadLibrary(L"Msftedit.dll");
        m_hEdit = CreateWindowEx(0, MSFTEDIT_CLASS, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_WANTRETURN,
            0, TOP_BAR_HEIGHT, 100, 100, hWnd, (HMENU)ID_EDIT_CONTROL, GetModuleHandle(NULL), NULL);
        EnableDarkMode(m_hEdit);
        SendMessage(m_hEdit, EM_SETTARGETDEVICE, NULL, 0);
        SendMessage(m_hEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)COL_BG_MAIN);
        SendMessage(m_hEdit, WM_SETFONT, (WPARAM)m_hFontEdit, TRUE);
        ApplyTextColor();
        SendMessage(m_hEdit, EM_SETEVENTMASK, 0, ENM_CHANGE);
        break;

    case WM_SIZE: {
        RECT rcClient; GetClientRect(hWnd, &rcClient);
        SetWindowPos(m_hEdit, NULL, 0, TOP_BAR_HEIGHT, rcClient.right, rcClient.bottom - TOP_BAR_HEIGHT, SWP_NOZORDER);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rcTop = { 0, 0, ps.rcPaint.right, TOP_BAR_HEIGHT };
        HBRUSH hBrushBar = CreateSolidBrush(COL_BG_BAR);
        FillRect(hdc, &rcTop, hBrushBar); DeleteObject(hBrushBar);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COL_TEXT);
        SelectObject(hdc, m_hFontUI);
        RECT rcBtn = { 12, 0, FILE_BTN_WIDTH + 12, TOP_BAR_HEIGHT };
        DrawText(hdc, L"\x25CF", -1, &rcBtn, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_LBUTTONUP: {
        if (m_isDragging) break;
        POINT pt; GetCursorPos(&pt);
        POINT clientPt = pt; ScreenToClient(hWnd, &clientPt);
        if (clientPt.y <= TOP_BAR_HEIGHT && clientPt.x <= FILE_BTN_WIDTH) {
            POINT ptMenu = { 12, TOP_BAR_HEIGHT }; ClientToScreen(hWnd, &ptMenu);
            CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"CleanpadMenuClass", NULL, WS_POPUP | WS_VISIBLE | CS_DROPSHADOW,
                ptMenu.x, ptMenu.y, 150, 90, hWnd, NULL, m_hInst, NULL);
        }
        break;
    }
    case WM_MBUTTONDOWN: {
        POINT pt; GetCursorPos(&pt);
        POINT clientPt = pt; ScreenToClient(hWnd, &clientPt);
        if (clientPt.y <= TOP_BAR_HEIGHT) {
            SetCapture(hWnd); m_isDragging = true; m_dragStartPoint = pt;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (m_isDragging) {
            POINT ptCurrent; GetCursorPos(&ptCurrent);
            RECT rcWin; GetWindowRect(hWnd, &rcWin);
            SetWindowPos(hWnd, NULL, rcWin.left + (ptCurrent.x - m_dragStartPoint.x),
                rcWin.top + (ptCurrent.y - m_dragStartPoint.y), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            m_dragStartPoint = ptCurrent;
        }
        break;
    }
    case WM_MBUTTONUP: ReleaseCapture(); m_isDragging = false; break;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDM_OPEN) PromptOpen();
        else if (id == IDM_SAVE) m_currentFilePath.empty() ? PromptSaveAs() : SaveFile(m_currentFilePath.c_str());
        else if (id == IDM_SAVEAS) PromptSaveAs();
        break;
    }
    case WM_MOUSEWHEEL: {
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            static int zoomNum = 100;
            zoomNum += (GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? 10 : -10;
            if (zoomNum < 10) zoomNum = 10; if (zoomNum > 500) zoomNum = 500;
            SendMessage(m_hEdit, EM_SETZOOM, zoomNum, 100);
            return 0;
        }
        break;
    }
    case WM_DROPFILES: {
        wchar_t szFile[MAX_PATH];
        HDROP hDrop = (HDROP)wParam;
        DragQueryFile(hDrop, 0, szFile, MAX_PATH);
        LoadFile(szFile);
        DragFinish(hDrop);
        break;
    }
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hWnd, &pt);
            RECT rc; GetClientRect(hWnd, &rc);
            if (pt.x >= rc.right - 20 && pt.y >= rc.bottom - 20) return HTBOTTOMRIGHT;
        }
        return hit;
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Menu Procedure (Kept simple and static) [cite: 59-84]
LRESULT CALLBACK Cleanpad::MenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static int hoverIndex = -1;
    const wchar_t* items[] = { L"Open", L"Save", L"Save As" };
    switch (message) {
    case WM_CREATE: SetCapture(hWnd); break;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rcClient; GetClientRect(hWnd, &rcClient);
        HBRUSH hBrushBg = CreateSolidBrush(COL_MENU_BG); FillRect(hdc, &rcClient, hBrushBg); DeleteObject(hBrushBg);
        SetBkMode(hdc, TRANSPARENT);

        // We need the font here. For simplicity in this split, we recreate or use system font.
        // In a full framework, we'd access the main app's font.
        HFONT hFontLocal = CreateFont(-12, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"Noto Sans");
        SelectObject(hdc, hFontLocal);

        for (int i = 0; i < 3; i++) {
            RECT rcItem = { 15, i * 30, rcClient.right, (i + 1) * 30 };
            if (i == hoverIndex) {
                HBRUSH hBrushSel = CreateSolidBrush(COL_MENU_SEL);
                RECT rcFill = { 0, i * 30, rcClient.right, (i + 1) * 30 };
                FillRect(hdc, &rcFill, hBrushSel); DeleteObject(hBrushSel);
                SetTextColor(hdc, RGB(255, 255, 255));
            }
            else SetTextColor(hdc, COL_TEXT);
            DrawText(hdc, items[i], -1, &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        DeleteObject(hFontLocal);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_MOUSEMOVE: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        RECT rc; GetClientRect(hWnd, &rc);
        int newIndex = PtInRect(&rc, pt) ? pt.y / 30 : -1;
        if (newIndex != hoverIndex) { hoverIndex = newIndex; InvalidateRect(hWnd, NULL, FALSE); }
        break;
    }
    case WM_LBUTTONUP: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        RECT rc; GetClientRect(hWnd, &rc);
        if (PtInRect(&rc, pt)) {
            int index = pt.y / 30;
            int cmd = (index == 0) ? IDM_OPEN : (index == 1) ? IDM_SAVE : (index == 2) ? IDM_SAVEAS : 0;
            if (cmd) PostMessage(GetParent(hWnd), WM_COMMAND, cmd, 0);
        }
        ReleaseCapture(); DestroyWindow(hWnd);
        break;
    }
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
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
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void Cleanpad::PromptOpen() {
    OPENFILENAME ofn = { sizeof(ofn) };
    wchar_t szFile[260] = { 0 };
    ofn.hwndOwner = m_hMain;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn)) LoadFile(ofn.lpstrFile);
}

void Cleanpad::PromptSaveAs() {
    OPENFILENAME ofn = { sizeof(ofn) };
    wchar_t szFile[260] = { 0 };
    ofn.hwndOwner = m_hMain;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";

    // NEW: Default extension if user types none
    ofn.lpstrDefExt = L"txt";

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        SaveFile(ofn.lpstrFile);
    }
}