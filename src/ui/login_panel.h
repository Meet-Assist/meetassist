#pragma once
#include <windows.h>
#include <string>
#include "../auth/auth.h"

class LoginPanel {
public:
    LoginPanel();
    ~LoginPanel();

    void Create(HWND parentHwnd, int x, int y, int width, int height);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool IsVisible() const;
    void Show();
    void Hide();
    HWND GetHandle() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void ValidateAndLogin();
    void ShowError(const std::wstring& message);
    void ShowSuccess(const std::wstring& message);

    HWND m_hwnd;
    HWND m_parentHwnd;
    HWND m_tokenEdit;
    HWND m_loginButton;
    HWND m_statusLabel;
    
    std::wstring m_tokenText;
    std::wstring m_statusText;
    bool m_isVisible;

    // UI Constants
    static const int EDIT_HEIGHT = 25;
    static const int BUTTON_HEIGHT = 30;
    static const int PADDING = 20;
    static const int LABEL_HEIGHT = 20;

    // Control IDs
    static const int ID_TOKEN_EDIT = 201;
    static const int ID_LOGIN_BUTTON = 202;
    static const int ID_STATUS_LABEL = 203;
};
