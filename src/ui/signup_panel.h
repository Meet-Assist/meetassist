#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "../auth/auth.h"
#include "../services/location_service.h"

class SignupPanel {
public:
    SignupPanel();
    ~SignupPanel();

    // Core window functionality
    void Create(HWND parentHwnd, int x, int y, int width, int height);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Window visibility management
    bool IsVisible() const;
    void Show();
    void Hide();
    HWND GetHandle() const { return m_hwnd; }

    // Location handling
    void InitializeLocation();
    void UpdateLocationDisplay();

private:
    // Window message handlers
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void ValidateAndSubmit();
    void ShowError(const std::wstring& message);
    void ShowSuccess(const std::wstring& message);

    // Window handles
    HWND m_hwnd;
    HWND m_parentHwnd;
    HWND m_emailEdit;
    HWND m_submitButton;
    HWND m_statusLabel;
    HWND m_locationLabel;
    HWND m_currencyLabel;
    
    // State variables
    std::wstring m_emailText;
    std::wstring m_statusText;
    bool m_isVisible;
    LocationInfo m_locationInfo;

    // UI Constants
    static const int EDIT_HEIGHT = 25;
    static const int BUTTON_HEIGHT = 30;
    static const int PADDING = 20;
    static const int LABEL_HEIGHT = 20;

    // Control IDs
    static const int ID_EMAIL_EDIT = 101;
    static const int ID_SUBMIT_BUTTON = 102;
    static const int ID_STATUS_LABEL = 103;
};