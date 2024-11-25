#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <winsock2.h> 
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <gdiplus.h>
#include <memory>
#include <dwmapi.h>
#include <vector>
#include <string>
#include <shellapi.h>

// Include authentication components
#include "auth/auth.h"
#include "ui/signup_panel.h"
#include "ui/login_panel.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")


// Global variables
HWND g_hwnd = nullptr;
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGIOutputDuplication* g_deskDupl = nullptr;
NOTIFYICONDATA g_nid = {};
bool g_isMinimized = false;

// Authentication UI components
std::unique_ptr<SignupPanel> g_signupPanel;
std::unique_ptr<LoginPanel> g_loginPanel;
bool g_isAuthenticated = false;

// Navigation IDs
const int ID_NAV_ACTIVATION = 4001;
const int ID_NAV_CONFIGURATION = 4002;
const int ID_NAV_ASSISTANCE = 4003;
const int ID_NAV_FEED = 4004;
const int ID_NAV_FEEDBACK = 4005;

// Constants for window messages
const UINT WM_TRAYICON = WM_USER + 1;
const UINT IDM_RESTORE = 3000;
const UINT IDM_EXIT = 3001;

// UI Elements
std::vector<HWND> g_navButtons;
const int HEADER_HEIGHT = 40;
const COLORREF HEADER_COLOR = RGB(50, 50, 50);
const COLORREF HEADER_TEXT_COLOR = RGB(255, 255, 255);
const COLORREF ACTIVE_TAB_COLOR = RGB(70, 70, 70);
HBRUSH g_headerBrush = nullptr;
HBRUSH g_activeTabBrush = nullptr;
HFONT g_headerFont = nullptr;
int g_activeTab = ID_NAV_ACTIVATION;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void ShowContextMenu(HWND hwnd, POINT pt);
void CreateHeaderControls(HWND hwnd);
void DrawHeader(HDC hdc);
void ShowTabContent(HWND hwnd, int tabId);
void DrawTabContent(HDC hdc, const RECT& contentRect, int tabId);
void InitializeAuthUI(HWND hwnd);
void ShowAuthenticationStatus(HDC hdc, const RECT& rect);

void InitializeLocation() {
    // Initialize location service in a separate thread
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        LocationService::getInstance().getLocationInfo();
        return 0;
    }, nullptr, 0, nullptr);
}

// Initialize DirectX
bool InitializeDirectX() {
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, featureLevels, 1, D3D11_SDK_VERSION,
        &g_device, &featureLevel, &g_context
    );

    if (FAILED(hr)) return false;

    return true;
}

void CreateTrayIcon(HWND hwnd) {
    g_nid = {};
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"MeetAssist");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void ShowContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_RESTORE, L"Restore");
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
        pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
}

void InitializeAuthUI(HWND hwnd) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Calculate content area (below header)
    int contentX = 0;
    int contentY = HEADER_HEIGHT;
    int contentWidth = clientRect.right - clientRect.left;
    int contentHeight = clientRect.bottom - HEADER_HEIGHT;

    // Initialize panels
    g_signupPanel = std::make_unique<SignupPanel>();
    g_loginPanel = std::make_unique<LoginPanel>();

    // Create panels
    g_signupPanel->Create(hwnd, contentX, contentY, contentWidth, contentHeight);
    g_loginPanel->Create(hwnd, contentX, contentY, contentWidth, contentHeight);

    // Initially show signup panel if not authenticated
    if (!AuthenticationManager::getInstance().isUserLoggedIn()) {
        g_signupPanel->Show();
        g_loginPanel->Hide();
    } else {
        g_isAuthenticated = true;
        g_signupPanel->Hide();
        g_loginPanel->Hide();
    }
}

void CreateHeaderControls(HWND hwnd) {
    g_headerFont = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Segoe UI"
    );

    g_headerBrush = CreateSolidBrush(HEADER_COLOR);
    g_activeTabBrush = CreateSolidBrush(ACTIVE_TAB_COLOR);

    const struct {
        int id;
        const wchar_t* text;
    } buttons[] = {
        {ID_NAV_ACTIVATION, L"Activation"},
        {ID_NAV_CONFIGURATION, L"Configuration"},
        {ID_NAV_ASSISTANCE, L"Assistance"},
        {ID_NAV_FEED, L"Feed"},
        {ID_NAV_FEEDBACK, L"Feedback"}
    };

    int buttonWidth = 120;
    int buttonHeight = 30;
    int buttonSpacing = 5;
    int startX = 10;
    int buttonY = (HEADER_HEIGHT - buttonHeight) / 2;

    for (const auto& btn : buttons) {
        HWND hButton = CreateWindowW(
            L"BUTTON", btn.text,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            startX, buttonY, buttonWidth, buttonHeight,
            hwnd, (HMENU)(INT_PTR)btn.id,
            GetModuleHandleW(nullptr), nullptr
        );

        SendMessageW(hButton, WM_SETFONT, (WPARAM)g_headerFont, TRUE);
        g_navButtons.push_back(hButton);
        startX += buttonWidth + buttonSpacing;
    }
}

void ShowAuthenticationStatus(HDC hdc, const RECT& rect) {
    if (g_isAuthenticated) {
        std::wstring email = std::wstring(AuthenticationManager::getInstance().getCurrentUserEmail().begin(),
                                        AuthenticationManager::getInstance().getCurrentUserEmail().end());
        std::wstring status = L"Activated\nEmail: " + email;
        
        SelectObject(hdc, g_headerFont);
        SetTextColor(hdc, RGB(0, 128, 0));
        SetBkMode(hdc, TRANSPARENT);
        
        RECT textRect = rect;
        textRect.left += 20;
        textRect.top += 20;
        DrawTextW(hdc, status.c_str(), -1, &textRect, DT_LEFT | DT_WORDBREAK);
    }
}

void ShowTabContent(HWND hwnd, int tabId) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    clientRect.top = HEADER_HEIGHT;
    InvalidateRect(hwnd, &clientRect, TRUE);
    g_activeTab = tabId;

    // Handle authentication UI visibility
    if (tabId == ID_NAV_ACTIVATION) {
        if (!g_isAuthenticated) {
            if (g_signupPanel && g_loginPanel) {
                // Show appropriate panel based on registration state
                if (AuthenticationManager::getInstance().getCurrentUserEmail().empty()) {
                    g_signupPanel->Show();
                    g_loginPanel->Hide();
                } else {
                    g_signupPanel->Hide();
                    g_loginPanel->Show();
                }
            }
        } else {
            if (g_signupPanel) g_signupPanel->Hide();
            if (g_loginPanel) g_loginPanel->Hide();
        }
    } else {
        // Hide auth panels when switching to other tabs
        if (g_signupPanel) g_signupPanel->Hide();
        if (g_loginPanel) g_loginPanel->Hide();
    }

    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

bool CreateAppWindow() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MeetAssistPart1";
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClassExW(&wc))
        return false;

    g_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST,
        L"MeetAssistPart1", L"MeetAssist",
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    if (!g_hwnd)
        return false;

    CreateHeaderControls(g_hwnd);
    InitializeAuthUI(g_hwnd);
    InitializeLocation();  // Add this line
    
    SetWindowDisplayAffinity(g_hwnd, WDA_EXCLUDEFROMCAPTURE);
    SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(g_hwnd, SW_SHOW);
    CreateTrayIcon(g_hwnd);

    return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Draw header
            RECT headerRect;
            GetClientRect(hwnd, &headerRect);
            headerRect.bottom = HEADER_HEIGHT;
            FillRect(hdc, &headerRect, g_headerBrush);

            // Draw content area
            RECT contentRect;
            GetClientRect(hwnd, &contentRect);
            contentRect.top = HEADER_HEIGHT;
            FillRect(hdc, &contentRect, (HBRUSH)(COLOR_WINDOW + 1));

            // Draw specific tab content
            if (g_activeTab == ID_NAV_ACTIVATION && g_isAuthenticated) {
                ShowAuthenticationStatus(hdc, contentRect);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_NAV_ACTIVATION:
                case ID_NAV_CONFIGURATION:
                case ID_NAV_ASSISTANCE:
                case ID_NAV_FEED:
                case ID_NAV_FEEDBACK:
                    ShowTabContent(hwnd, LOWORD(wParam));
                    return 0;

                case IDM_RESTORE:
                    ShowWindow(hwnd, SW_SHOW);
                    g_isMinimized = false;
                    return 0;

                case IDM_EXIT:
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;

        case WM_SYSCOMMAND:
            switch (wParam & 0xFFF0) {
                case SC_MINIMIZE:
                    ShowWindow(hwnd, SW_HIDE);
                    g_isMinimized = true;
                    return 0;
                case SC_CLOSE:
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;

        case WM_TRAYICON:
            switch (lParam) {
                case WM_RBUTTONUP: {
                    POINT pt;
                    GetCursorPos(&pt);
                    ShowContextMenu(hwnd, pt);
                    return 0;
                }
                case WM_LBUTTONUP:
                    if (g_isMinimized) {
                        ShowWindow(hwnd, SW_SHOW);
                        g_isMinimized = false;
                    }
                    return 0;
            }
            break;

        case WM_DESTROY:
            DeleteObject(g_headerBrush);
            DeleteObject(g_activeTabBrush);
            DeleteObject(g_headerFont);
            g_navButtons.clear();
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Initialize COM for the application
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create window and initialize DirectX
    if (!CreateAppWindow() || !InitializeDirectX()) {
        MessageBoxW(nullptr, L"Failed to initialize application", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // Check if authentication is already active
    g_isAuthenticated = AuthenticationManager::getInstance().isUserLoggedIn();

    // Message loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        // Handle tab and other navigation keys
        if (!IsDialogMessageW(g_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Check authentication status changes
        bool currentAuthStatus = AuthenticationManager::getInstance().isUserLoggedIn();
        if (g_isAuthenticated != currentAuthStatus) {
            g_isAuthenticated = currentAuthStatus;
            if (g_activeTab == ID_NAV_ACTIVATION) {
                ShowTabContent(g_hwnd, ID_NAV_ACTIVATION);
            }
        }
    }

    // Cleanup
    if (g_deskDupl) {
        g_deskDupl->Release();
        g_deskDupl = nullptr;
    }
    
    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }

    // Cleanup GDI+ and COM
    Gdiplus::GdiplusShutdown(gdiplusToken);
    CoUninitialize();

    return static_cast<int>(msg.wParam);
}