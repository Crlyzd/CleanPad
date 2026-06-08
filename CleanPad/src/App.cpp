#include "App.h"
#include "UI/Theme.h"
#include "UI/Menu.h" // NOLINT
#include "Utils/Shortcuts.h"
#include "../resource.h"

// ============================================================
//  App.cpp — Application lifecycle and message handling
// ============================================================

App::App()
    : m_hMain(nullptr)
    , m_hInst(nullptr)
    , m_isAlwaysOnTop(false)
    , m_isDragging(false)
    , m_dragStartPoint({0, 0})
{}

// ============================================================
//  Run — Initialize, show window, run message loop
// ============================================================
int App::Run(HINSTANCE hInstance, int nCmdShow) {
    m_hInst = hInstance;

    // Initialize shared fonts before any window is created
    Theme::InitFonts();

    if (!RegisterClasses())        return 1;
    if (!CreateMainWindow(nCmdShow)) return 1;

    // Handle command-line file argument (drag-to-open or association)
    int    argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1) {
        FileIO::LoadFile(m_editor.GetHwnd(), argv[1], m_currentFilePath);
    }
    LocalFree(argv);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!Shortcuts::Process(&msg, this)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Theme::DestroyFonts();
    return (int)msg.wParam;
}

// ============================================================
//  RegisterClasses — main window + menu popup
// ============================================================
bool App::RegisterClasses() {
    HICON hIcon = LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_ICON1));

    WNDCLASSEXW wc   = { sizeof(wc) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = m_hInst;
    wc.hIcon         = hIcon;
    wc.hIconSm       = hIcon;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(Theme::COL_BG_MAIN);
    wc.lpszClassName = WC_CLEANPAD_MAIN;
    if (!RegisterClassExW(&wc)) return false;

    Menu::RegisterClass(m_hInst);
    return true;
}

// ============================================================
//  CreateMainWindow — centered, no caption, resizable
// ============================================================
bool App::CreateMainWindow(int nCmdShow) {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW    = 300;
    int winH    = 376;
    int xPos    = (screenW - winW) / 2;
    int yPos    = (screenH - winH) / 2;

    m_hMain = CreateWindowW(
        WC_CLEANPAD_MAIN, APP_NAME,
        WS_POPUP | WS_THICKFRAME | WS_VISIBLE,
        xPos, yPos, winW, winH,
        nullptr, nullptr, m_hInst, this);   // 'this' → GWLP_USERDATA thunk

    if (!m_hMain) return false;

    ShowWindow(m_hMain, nCmdShow);
    UpdateWindow(m_hMain);
    return true;
}

// ============================================================
//  WndProc — static thunk → forwards to HandleMessage
// ============================================================
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* pApp = nullptr;

    if (msg == WM_NCCREATE) {
        // Store the App* passed as lpCreateParams
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pApp = reinterpret_cast<App*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
        pApp->m_hMain = hWnd;
    } else {
        pApp = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pApp) return pApp->HandleMessage(hWnd, msg, wParam, lParam);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
//  HandleMessage — instance-level message dispatch
// ============================================================
LRESULT App::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        OnCreate(hWnd);
        break;

    case WM_SIZE: {
        RECT rc; GetClientRect(hWnd, &rc);
        OnSize(rc.right, rc.bottom);
        break;
    }

    case WM_PAINT:
        OnPaint(hWnd);
        break;

    // ---- Window dragging via middle mouse button on top bar ----
    case WM_MBUTTONDOWN: {
        POINT pt; GetCursorPos(&pt);
        POINT cli = pt; ScreenToClient(hWnd, &cli);
        if (cli.y <= TOP_BAR_HEIGHT) {
            SetCapture(hWnd);
            m_isDragging   = true;
            m_dragStartPoint = pt;
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (m_isDragging) {
            POINT ptNow; GetCursorPos(&ptNow);
            RECT  rcWin; GetWindowRect(hWnd, &rcWin);
            SetWindowPos(hWnd, nullptr,
                rcWin.left + (ptNow.x - m_dragStartPoint.x),
                rcWin.top  + (ptNow.y - m_dragStartPoint.y),
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
            m_dragStartPoint = ptNow;
        }
        break;
    }
    case WM_MBUTTONUP:
        ReleaseCapture();
        m_isDragging = false;
        break;

    // ---- Top-bar click → show popup menu ----
    case WM_LBUTTONUP: {
        if (m_isDragging) break;
        POINT pt; GetCursorPos(&pt);
        POINT cli = pt; ScreenToClient(hWnd, &cli);
        if (cli.y <= TOP_BAR_HEIGHT && cli.x <= FILE_BTN_WIDTH) {
            OnLButtonUp(hWnd, cli);
        }
        break;
    }

    // ---- Menu commands ----
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if      (id == IDM_OPEN)   Open();
        else if (id == IDM_SAVE)   Save();
        else if (id == IDM_SAVEAS) SaveAs();
        break;
    }

    // ---- File Loading Background Messages ----
    case WM_USER_FILE_LOAD_START: {
        m_isLoading = true;
        m_loadProgress = 0.0f;
        SetTimer(hWnd, 1, 16, nullptr); // ~60fps animation
        break;
    }
    case WM_USER_FILE_LOADED: {
        auto* res = reinterpret_cast<FileIO::LoadResult*>(lParam);
        
        // Stop animation
        KillTimer(hWnd, 1);
        m_isLoading = false;
        
        // Set text
        SetWindowText(m_editor.GetHwnd(), res->wBuf->data());
        
        // Apply coloring
        m_editor.ApplyDefaultColor();

        // 3MB limit for word wrap
        if (res->fileSize > 3 * 1024 * 1024) {
            m_editor.SetWordWrap(false);
        } else {
            m_editor.SetWordWrap(true);
        }

        m_currentFilePath = res->path;

        // Cleanup
        delete res->wBuf;
        delete res;

        // Redraw top bar to clear the progress line
        RECT rcBar = { 0, 0, 10000, TOP_BAR_HEIGHT };
        InvalidateRect(hWnd, &rcBar, FALSE);
        break;
    }

    case WM_TIMER: {
        if (wParam == 1 && m_isLoading) {
            m_loadProgress += 0.05f;
            if (m_loadProgress > 1.0f) {
                m_loadProgress = 0.0f; // loop animation
            }
            RECT rcBar = { 0, TOP_BAR_HEIGHT - 2, 10000, TOP_BAR_HEIGHT };
            InvalidateRect(hWnd, &rcBar, FALSE);
        }
        break;
    }

    // ---- Ctrl+Wheel zoom ----
    case WM_MOUSEWHEEL:
        OnMouseWheel(wParam);
        break;

    // ---- Resize grip (bottom-right corner) ----
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hWnd, &pt);
            RECT  rc; GetClientRect(hWnd, &rc);
            if (pt.x >= rc.right - 20 && pt.y >= rc.bottom - 20)
                return HTBOTTOMRIGHT;
        }
        return hit;
    }

    // ---- Drag-and-drop file ----
    case WM_DROPFILES:
        OnDropFiles((HDROP)wParam);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
//  WM_ handler helpers
// ============================================================

void App::OnCreate(HWND hWnd) {
    DragAcceptFiles(hWnd, TRUE);
    Theme::EnableDarkMode(hWnd);
    m_editor.Create(hWnd, m_hInst);
}

void App::OnSize(int w, int h) {
    m_editor.Resize(w, h);
}

void App::OnPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    // --- Top bar ---
    RECT rcBar = { 0, 0, ps.rcPaint.right, TOP_BAR_HEIGHT };
    HBRUSH hBr = CreateSolidBrush(Theme::COL_BG_BAR);
    FillRect(hdc, &rcBar, hBr);
    DeleteObject(hBr);

    // --- Dot button (●) ---
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Theme::COL_TEXT);
    SelectObject(hdc, Theme::g_hFontUI);
    RECT rcBtn = { 12, 0, FILE_BTN_WIDTH + 12, TOP_BAR_HEIGHT };
    DrawText(hdc, L"\x25CF", -1, &rcBtn, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // --- Loading Progress Line ---
    if (m_isLoading) {
        int width = ps.rcPaint.right;
        int barWidth = width / 3;
        int startX = (int)(m_loadProgress * (width + barWidth)) - barWidth;
        
        RECT rcProgress = { startX, TOP_BAR_HEIGHT - 2, startX + barWidth, TOP_BAR_HEIGHT };
        HBRUSH hProgBr = CreateSolidBrush(Theme::COL_TEXT);
        FillRect(hdc, &rcProgress, hProgBr);
        DeleteObject(hProgBr);
    }

    EndPaint(hWnd, &ps);
}

void App::OnLButtonUp(HWND hWnd, POINT clientPt) {
    // Spawn the popup menu just below the top bar
    POINT ptMenu = { 12, TOP_BAR_HEIGHT };
    ClientToScreen(hWnd, &ptMenu);

    // Height = number of items × item height (30px each, 3 items)
    CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WC_CLEANPAD_MENU, nullptr,
        WS_POPUP | WS_VISIBLE | CS_DROPSHADOW,
        ptMenu.x, ptMenu.y, 150, 90,
        hWnd, nullptr, m_hInst, nullptr);
}

void App::OnDropFiles(HDROP hDrop) {
    wchar_t szFile[MAX_PATH];
    DragQueryFile(hDrop, 0, szFile, MAX_PATH);
    FileIO::LoadFile(m_editor.GetHwnd(), szFile, m_currentFilePath);
    DragFinish(hDrop);
}

void App::OnMouseWheel(WPARAM wParam) {
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        int delta = (GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? +1 : -1;
        m_editor.AdjustZoom(delta);
    }
}

// ============================================================
//  Command Methods (called by Shortcuts and Menu)
// ============================================================

void App::Save() {
    m_currentFilePath.empty()
        ? FileIO::PromptSaveAs(m_hMain, m_editor.GetHwnd(), m_currentFilePath)
        : FileIO::SaveFile(m_editor.GetHwnd(), m_currentFilePath.c_str(), m_currentFilePath);
}

void App::SaveAs() {
    FileIO::PromptSaveAs(m_hMain, m_editor.GetHwnd(), m_currentFilePath);
}

void App::Open() {
    FileIO::PromptOpen(m_hMain, m_editor.GetHwnd(), m_currentFilePath);
}

void App::NewWindow() {
    wchar_t szExe[MAX_PATH];
    if (GetModuleFileName(nullptr, szExe, MAX_PATH)) {
        ShellExecute(nullptr, L"open", szExe, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void App::ToggleAlwaysOnTop() {
    m_isAlwaysOnTop = !m_isAlwaysOnTop;
    SetWindowPos(m_hMain,
        m_isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void App::ToggleBold()      { m_editor.ToggleFormatting(CFM_BOLD); }
void App::ToggleItalic()    { m_editor.ToggleFormatting(CFM_ITALIC); }
void App::ToggleUnderline() { m_editor.ToggleFormatting(CFM_UNDERLINE); }
void App::CopyToClipboard() { m_editor.CopyToClipboard(m_hMain); }
