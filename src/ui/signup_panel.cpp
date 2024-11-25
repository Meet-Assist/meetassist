#include "signup_panel.h"
#include <commctrl.h>
#include <windowsx.h>
#include <string>
#include <sstream>


// Timer ID for location updates
const UINT_PTR LOCATION_TIMER_ID = 1001;
const UINT LOCATION_CHECK_INTERVAL = 1000; // 1 second

void SignupPanel::InitializeLocation() {
    // Get initial location info (might be default values)
    m_locationInfo = LocationService::getInstance().getCachedLocationInfo();
    UpdateLocationDisplay();

    // Set up timer to check for location updates
    SetTimer(m_hwnd, LOCATION_TIMER_ID, LOCATION_CHECK_INTERVAL, 
        [](HWND hwnd, UINT, UINT_PTR timerId, DWORD) {
            SignupPanel* panel = reinterpret_cast<SignupPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (panel && LocationService::getInstance().isLocationAvailable()) {
                panel->m_locationInfo = LocationService::getInstance().getCachedLocationInfo();
                panel->UpdateLocationDisplay();
                KillTimer(hwnd, timerId); // Stop checking once we have the location
            }
    });
}

void SignupPanel::UpdateLocationDisplay() {
    if (!IsWindow(m_locationLabel) || !IsWindow(m_currencyLabel)) {
        return;
    }

    // Create location text
    std::wstring locationText;
    LocationInfo info = m_locationInfo;

    // Add IP Address
    locationText = L"IP Address: " + std::wstring(info.ip.begin(), info.ip.end()) + L"\n";

    // Add Location
    locationText += L"Location: ";
    if (info.city != "Detecting..." && info.city != "Unknown") {
        locationText += std::wstring(info.city.begin(), info.city.end()) + L", ";
    }
    if (info.region != "Detecting..." && info.region != "Unknown") {
        locationText += std::wstring(info.region.begin(), info.region.end()) + L" ";
        if (!info.region_code.empty()) {
            locationText += L"(" + std::wstring(info.region_code.begin(), info.region_code.end()) + L"), ";
        }
    }
    locationText += std::wstring(info.country.begin(), info.country.end());
    if (!info.country_code.empty()) {
        locationText += L" (" + std::wstring(info.country_code.begin(), info.country_code.end()) + L")";
    }

    // Add Timezone
    if (info.timezone != "Detecting..." && info.timezone != "Unknown") {
        locationText += L"\nTimezone: " + std::wstring(info.timezone.begin(), info.timezone.end());
    }

    // Add Coordinates
    if (info.latitude != 0.0 || info.longitude != 0.0) {
        std::wstringstream coords;
        coords.precision(6);
        coords << L"\nCoordinates: " << info.latitude << L", " << info.longitude;
        locationText += coords.str();
    }
    
    // Create currency text
    std::wstring currencyText = L"Currency: ";
    currencyText += std::wstring(info.currency.begin(), info.currency.end());
    if (!info.currency_symbol.empty()) {
        currencyText += L" (" + std::wstring(info.currency_symbol.begin(), info.currency_symbol.end()) + L")";
    }
    
    SetWindowTextW(m_locationLabel, locationText.c_str());
    SetWindowTextW(m_currencyLabel, currencyText.c_str());
}

void SignupPanel::ValidateAndSubmit() {
    // Get email text
    int textLength = GetWindowTextLengthW(m_emailEdit) + 1;
    std::vector<wchar_t> buffer(textLength);
    GetWindowTextW(m_emailEdit, buffer.data(), textLength);
    std::wstring email(buffer.data());

    // Convert wstring to string for the auth manager
    std::string emailStr(email.begin(), email.end());

    // Register user
    if (AuthenticationManager::getInstance().registerUser(emailStr)) {
        // Get the generated token
        std::string token = AuthenticationManager::getInstance().getCurrentToken();
        
        ShowSuccess(L"Registration successful! Please check your email for the activation token.");
    } else {
        ShowError(L"Invalid email format. Please try again.");
    }
}

SignupPanel::SignupPanel() 
    : m_hwnd(nullptr)
    , m_parentHwnd(nullptr)
    , m_emailEdit(nullptr)
    , m_submitButton(nullptr)
    , m_statusLabel(nullptr)
    , m_locationLabel(nullptr)
    , m_currencyLabel(nullptr)
    , m_isVisible(false)
{
}

SignupPanel::~SignupPanel() {
    if (m_hwnd) {
        KillTimer(m_hwnd, LOCATION_TIMER_ID);
        DestroyWindow(m_hwnd);
    }
}

void SignupPanel::Create(HWND parentHwnd, int x, int y, int width, int height) {
    m_parentHwnd = parentHwnd;

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = SignupPanel::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MeetAssistSignupPanel";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    // Create panel window
    m_hwnd = CreateWindowExW(
        0, L"MeetAssistSignupPanel", L"",
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parentHwnd, nullptr,
        GetModuleHandleW(nullptr), this
    );

    // Calculate control positions
    int currentY = PADDING;
    int editWidth = width - (2 * PADDING);
    int buttonWidth = 120;

    // Create location section first (at the top)
    CreateWindowExW(
        0, L"BUTTON", L"Location Information",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        PADDING, currentY,
        editWidth, LABEL_HEIGHT * 5 + PADDING * 2,  // Increased height to accommodate IP
        m_hwnd, nullptr,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += 20;

    // Create location label
    m_locationLabel = CreateWindowExW(
        0, L"STATIC", L"Detecting location...",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        PADDING * 2, currentY,
        editWidth - PADDING * 2, LABEL_HEIGHT,
        m_hwnd, nullptr,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += LABEL_HEIGHT + 5;

    // Create currency label
    m_currencyLabel = CreateWindowExW(
        0, L"STATIC", L"Detecting currency...",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        PADDING * 2, currentY,
        editWidth - PADDING * 2, LABEL_HEIGHT,
        m_hwnd, nullptr,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += LABEL_HEIGHT * 2 + PADDING;

    // Create email section
    CreateWindowExW(
        0, L"STATIC", L"Enter your email address:",
        WS_CHILD | WS_VISIBLE,
        PADDING, currentY,
        editWidth, LABEL_HEIGHT,
        m_hwnd, nullptr,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += LABEL_HEIGHT + 5;

    // Create email edit control
    m_emailEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        PADDING, currentY,
        editWidth, EDIT_HEIGHT,
        m_hwnd, (HMENU)(INT_PTR)ID_EMAIL_EDIT,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += EDIT_HEIGHT + PADDING;

    // Create submit button
    m_submitButton = CreateWindowExW(
        0, L"BUTTON", L"Sign Up",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        (width - buttonWidth) / 2, currentY,
        buttonWidth, BUTTON_HEIGHT,
        m_hwnd, (HMENU)(INT_PTR)ID_SUBMIT_BUTTON,
        GetModuleHandleW(nullptr), nullptr
    );
    currentY += BUTTON_HEIGHT + PADDING;

    // Create status label
    m_statusLabel = CreateWindowExW(
        0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE,
        PADDING, currentY,
        editWidth, LABEL_HEIGHT * 2,
        m_hwnd, (HMENU)(INT_PTR)ID_STATUS_LABEL,
        GetModuleHandleW(nullptr), nullptr
    );

    // Set default font for all controls
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    EnumChildWindows(m_hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
    }, (LPARAM)hFont);

    m_isVisible = true;

    // Initialize location immediately
    InitializeLocation();
}

LRESULT CALLBACK SignupPanel::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    SignupPanel* panel = nullptr;

    if (message == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        panel = reinterpret_cast<SignupPanel*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
    } else {
        panel = reinterpret_cast<SignupPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (panel) {
        return panel->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT SignupPanel::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;
            if (hwndStatic == m_statusLabel) {
                SetTextColor(hdcStatic, m_statusText.find(L"Error") != std::wstring::npos ? 
                           RGB(255, 0, 0) : RGB(0, 128, 0));
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            break;
        }

        case WM_DESTROY:
            KillTimer(hwnd, LOCATION_TIMER_ID);
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void SignupPanel::OnCommand(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
        case ID_SUBMIT_BUTTON:
            ValidateAndSubmit();
            break;
    }
}

void SignupPanel::ShowError(const std::wstring& message) {
    m_statusText = L"Error: " + message;
    SetWindowTextW(m_statusLabel, m_statusText.c_str());
    InvalidateRect(m_statusLabel, nullptr, TRUE);
}

void SignupPanel::ShowSuccess(const std::wstring& message) {
    m_statusText = message;
    SetWindowTextW(m_statusLabel, m_statusText.c_str());
    InvalidateRect(m_statusLabel, nullptr, TRUE);
}

void SignupPanel::Show() {
    ShowWindow(m_hwnd, SW_SHOW);
    m_isVisible = true;
    
    // Reset fields
    SetWindowTextW(m_emailEdit, L"");
    SetWindowTextW(m_statusLabel, L"");
    
    // Update location display (will show cached info or "detecting...")
    UpdateLocationDisplay();
}

void SignupPanel::Hide() {
    ShowWindow(m_hwnd, SW_HIDE);
    m_isVisible = false;
}

bool SignupPanel::IsVisible() const {
    return m_isVisible;
}