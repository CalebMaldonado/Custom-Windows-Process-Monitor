// TestProcessMonitor.cpp : A simple Windows GUI that lists the running processes.
//
// Built with the plain Win32 API (no extra libraries). The window has:
//   - a "Refresh" button that reloads the process list
//   - a status line showing how many processes were found
//   - a table (ListView) with Name, PID, Parent PID, and Thread count

#include <Windows.h>
#include <CommCtrl.h>   // "Common controls" such as the ListView table
#include <TlHelp32.h>   // CreateToolhelp32Snapshot, Process32FirstW, Process32NextW
#include <string>

// Tell the linker to include the common controls library.
#pragma comment(lib, "Comctl32.lib")

// Ask Windows for the modern look for buttons and tables (otherwise they look like Windows 95).
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// IDs for the controls. Windows tells us "control #102 was clicked", so each
// control needs a unique number.

constexpr int ID_LISTVIEW       = 101;
constexpr int ID_REFRESH_BUTTON = 102;
constexpr int ID_STATUS_TEXT    = 103;

// FAKE secret for git/GitHub practice. Pretend this is a real API key that must never be pushed.


// Handles ("HWND" = handle to a window) for the controls, so any function can use them.
// In Win32, every button, label, and table is technically a small "window".
HWND g_listView      = nullptr;
HWND g_refreshButton = nullptr;
HWND g_statusText    = nullptr;

// Adds one column header to the ListView table.

void AddColumn(HWND listView, int index, const wchar_t* title, int width)
{
    LVCOLUMNW column = {};
    column.mask     = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;  // which fields below we filled in
    column.pszText  = const_cast<wchar_t*>(title);
    column.cx       = width;
    column.iSubItem = index;
    ListView_InsertColumn(listView, index, &column);
}

// Takes a snapshot of all running processes and fills the table with them.
// This is the same logic as the original console version, but instead of
// printing with wcout, each process becomes a row in the table.

void LoadProcesses()
{
    // Pause drawing while we rebuild the list so the table doesn't flicker.
    SendMessageW(g_listView, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_listView);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        SetWindowTextW(g_statusText, L"Failed to get a snapshot of the running processes.");
        SendMessageW(g_listView, WM_SETREDRAW, TRUE, 0);
        return;
    }

    PROCESSENTRY32W pe32 = {};
    pe32.dwSize = sizeof(pe32);  // Windows requires this to be set before the first call

    int count = 0;

    if (Process32FirstW(snapshot, &pe32)) {
        do {
            // Column 0 (the process name) is the "item" itself.
            LVITEMW item = {};
            item.mask    = LVIF_TEXT;
            item.iItem   = count;
            item.pszText = pe32.szExeFile;
            int row = ListView_InsertItem(g_listView, &item);

            // Columns 1, 2, 3 are "sub-items". Numbers must be converted to text first.
            std::wstring pid       = std::to_wstring(pe32.th32ProcessID);
            std::wstring parentPid = std::to_wstring(pe32.th32ParentProcessID);
            std::wstring threads   = std::to_wstring(pe32.cntThreads);

            ListView_SetItemText(g_listView, row, 1, pid.data());
            ListView_SetItemText(g_listView, row, 2, parentPid.data());
            ListView_SetItemText(g_listView, row, 3, threads.data());

            count++;
        } while (Process32NextW(snapshot, &pe32));

        // Show only the first 8 characters of the key so we can see it is being used.
        std::wstring status = L"Processes running: " + std::to_wstring(count)
                            + L"   (key: " + std::wstring(API_SECRET_KEY).substr(0, 8) + L"...)";
        SetWindowTextW(g_statusText, status.c_str());
    }
    else {
        SetWindowTextW(g_statusText, L"Failed to get the first process.");
    }

    // Always release the snapshot when done (the original version forgot this on success).
    CloseHandle(snapshot);

    // Turn drawing back on and repaint the table.
    SendMessageW(g_listView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_listView, nullptr, TRUE);
}

// Positions the controls. Called whenever the window is resized so the table
// always fills the available space.

void LayoutControls(int windowWidth, int windowHeight)
{
    const int margin = 10;
    //          control          x                y       width                       height
    MoveWindow(g_refreshButton,  margin,          margin, 100,                        30,                            TRUE);
    MoveWindow(g_statusText,     margin + 110,    margin + 7, windowWidth - 130,      20,                            TRUE);
    MoveWindow(g_listView,       margin,          50,     windowWidth - 2 * margin,   windowHeight - 50 - margin,    TRUE);
}

// The "window procedure". Windows calls this function every time something
// happens to our window (created, resized, button clicked, closed, ...).
// Each event is a "message" (WM_ = Window Message).
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {

    case WM_CREATE: {
        // The window was just created: build the controls inside it.
        HINSTANCE hInstance = reinterpret_cast<CREATESTRUCTW*>(lParam)->hInstance;
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        g_refreshButton = CreateWindowExW(
            0, L"BUTTON", L"Refresh",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0,                       // real position is set in LayoutControls
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_REFRESH_BUTTON)),
            hInstance, nullptr);

        g_statusText = CreateWindowExW(
            0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_STATUS_TEXT)),
            hInstance, nullptr);

        g_listView = CreateWindowExW(
            0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,  // LVS_REPORT = table with columns
            0, 0, 0, 0,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LISTVIEW)),
            hInstance, nullptr);

        if (!g_refreshButton || !g_statusText || !g_listView) {
            MessageBoxW(hwnd, L"Failed to create the window controls.", L"Error", MB_ICONERROR);
            return -1;  // returning -1 from WM_CREATE cancels window creation
        }

        // Use the normal Windows UI font instead of the old default bitmap font.
        SendMessageW(g_refreshButton, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(g_statusText,    WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(g_listView,      WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

        // Click anywhere on a row to select the whole row; draw grid lines.
        ListView_SetExtendedListViewStyle(g_listView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        AddColumn(g_listView, 0, L"Process Name", 300);
        AddColumn(g_listView, 1, L"PID",          100);
        AddColumn(g_listView, 2, L"Parent PID",   100);
        AddColumn(g_listView, 3, L"Threads",      100);

        LoadProcesses();
        return 0;
    }

    case WM_SIZE:
        // The window was resized. LOWORD/HIWORD pull the new width/height out of lParam.
        LayoutControls(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        // A control was used. LOWORD(wParam) is the ID of the control.
        if (LOWORD(wParam) == ID_REFRESH_BUTTON) {
            LoadProcesses();
        }
        return 0;

    case WM_DESTROY:
        // The window was closed: tell the message loop in wWinMain to stop.
        PostQuitMessage(0);
        return 0;
    }

    // Anything we don't handle ourselves gets Windows' default behavior
    // (moving, minimizing, painting the title bar, etc.).
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

// Program entry point. GUI programs start at wWinMain instead of main.

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // 1. Load the common controls (needed for the ListView table).
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    // 2. Register a "window class": a template describing our main window,
    //    most importantly which function (WndProc) handles its messages.
    const wchar_t CLASS_NAME[] = L"ProcessMonitorWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);  // standard light-grey background

    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register the window class.", L"Error", MB_ICONERROR);
        return 1;
    }

    // 3. Create the actual window from that template.
    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Process Monitor",
        WS_OVERLAPPEDWINDOW,                 // normal window: title bar, resize, min/max/close
        CW_USEDEFAULT, CW_USEDEFAULT,        // let Windows pick the position
        800, 600,                            // width, height
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create the main window.", L"Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 4. The message loop: wait for events (clicks, key presses, resizes) and
    //    hand each one to WndProc. Runs until PostQuitMessage is called.
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
