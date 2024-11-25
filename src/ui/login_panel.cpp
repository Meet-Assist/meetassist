#include "login_panel.h"
#include <commctrl.h>
#include <windowsx.h>

LoginPanel::LoginPanel() 
    : m_hwnd(nullptr)
    , m_parentHwnd(nullptr)
    , m_tokenEdit(nullptr)
    , m_loginButton(nullptr)
    , m_statusLabel(nullptr)
    , m_isVisible(false)
{
}

LoginPanel::~LoginPanel() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

void LoginPanel::Create(HWND parentHwnd, int x, int y, int width, int height) {
    m_parentHwnd = parentHwnd;

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = LoginPanel::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MeetAssistLoginPanel";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    // Create panel window
    m_hwnd = CreateWindowExW(
        0,
        L"MeetAssistLoginPanel",
        L"",
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parentHwnd,
        nullptr,
        GetModuleHandleW(nullptr),
        this
    );

    // Calculate control positions
    int currentY = PADDING;
    int editWidth = width - (2 * PADDING);
    int buttonWidth = 120;

    // Create token instruction label
    CreateWindowExW(
        0, L"STATIC",
        L"Enter your activation token:",
        WS_CHILD | WS_VISIBLE,
        PADDING, currentY,
        editWidth, LABEL_HEIGHT,
        m_hwnd, nullptr,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += LABEL_HEIGHT + 5;

    // Create token edit control
    m_tokenEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD,  // Using password style for token
        PADDING, currentY,
        editWidth, EDIT_HEIGHT,
        m_hwnd, (HMENU)(INT_PTR)ID_TOKEN_EDIT,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += EDIT_HEIGHT + PADDING;

    // Create login button
    m_loginButton = CreateWindowExW(
        0, L"BUTTON",
        L"Activate",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        (width - buttonWidth) / 2, currentY,
        buttonWidth, BUTTON_HEIGHT,
        m_hwnd, (HMENU)ID_LOGIN_BUTTON,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += BUTTON_HEIGHT + PADDING;

    // Create status label
    m_statusLabel = CreateWindowExW(
        0, L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE,
        PADDING, currentY,
        editWidth, LABEL_HEIGHT * 2,
        m_hwnd, (HMENU)ID_STATUS_LABEL,
        GetModuleHandleW(nullptr), nullptr
    );

    // Set default font for all controls
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    EnumChildWindows(m_hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
    }, (LPARAM)hFont);

    m_isVisible = true;
}

LRESULT CALLBACK LoginPanel::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LoginPanel* panel = nullptr;

    if (message == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        panel = reinterpret_cast<LoginPanel*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
    } else {
        panel = reinterpret_cast<LoginPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (panel) {
        return panel->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT LoginPanel::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;
            if (hwndStatic == m_statusLabel) {
                // Set color based on status (red for error, green for success)
                SetTextColor(hdcStatic, m_statusText.find(L"Error") != std::wstring::npos ? 
                           RGB(255, 0, 0) : RGB(0, 128, 0));
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            break;
        }

        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                ValidateAndLogin();
                return 0;
            }
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void LoginPanel::OnCommand(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
        case ID_LOGIN_BUTTON:
            ValidateAndLogin();
            break;

        case ID_TOKEN_EDIT:
            if (HIWORD(wParam) == EN_CHANGE) {
                // Clear status message when user starts typing
                SetWindowTextW(m_statusLabel, L"");
            }
            break;
    }
}

void LoginPanel::ValidateAndLogin() {
    // Get token text
    int textLength = GetWindowTextLengthW(m_tokenEdit) + 1;
    std::vector<wchar_t> buffer(textLength);
    GetWindowTextW(m_tokenEdit, buffer.data(), textLength);
    std::wstring token(buffer.data());

    // Convert wstring to string for the auth manager
    std::string tokenStr(token.begin(), token.end());

    // Attempt to login
    if (AuthenticationManager::getInstance().loginWithToken(tokenStr)) {
        ShowSuccess(L"Activation successful! Welcome to MeetAssist.");
        
        // Optional: Notify parent window of successful login
        PostMessageW(m_parentHwnd, WM_COMMAND, MAKEWPARAM(ID_LOGIN_BUTTON, 0), 0);
        
        // Clear the token field
        SetWindowTextW(m_tokenEdit, L"");
    } else {
        ShowError(L"Invalid token. Please check and try again.");
    }
}

void LoginPanel::ShowError(const std::wstring& message) {
    m_statusText = L"Error: " + message;
    SetWindowTextW(m_statusLabel, m_statusText.c_str());
    InvalidateRect(m_statusLabel, nullptr, TRUE);
}

void LoginPanel::ShowSuccess(const std::wstring& message) {
    m_statusText = message;
    SetWindowTextW(m_statusLabel, m_statusText.c_str());
    InvalidateRect(m_statusLabel, nullptr, TRUE);
}

void LoginPanel::Show() {
    ShowWindow(m_hwnd, SW_SHOW);
    m_isVisible = true;
    
    // Clear previous status and token
    SetWindowTextW(m_statusLabel, L"");
    SetWindowTextW(m_tokenEdit, L"");
    
    // Set focus to token input
    SetFocus(m_tokenEdit);
}

void LoginPanel::Hide() {
    ShowWindow(m_hwnd, SW_HIDE);
    m_isVisible = false;
}

bool LoginPanel::IsVisible() const {
    return m_isVisible;
}