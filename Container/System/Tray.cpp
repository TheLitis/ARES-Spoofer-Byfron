#include "Tray.h"
#include <thread>

namespace TITAN::Tray {

    TrayIconManager::TrayIconManager(HINSTANCE hInstance) : hInst_(hInstance) {
        self_ = this;

        // Register hidden window class
        wc_ = {};
        wc_.cbSize = sizeof(WNDCLASSEX);
        wc_.lpfnWndProc = TrayProc;
        wc_.hInstance = hInst_;
        wc_.lpszClassName = L"TITAN_TRAY_CLASS";
        RegisterClassEx(&wc_);

        // Create message-only window
        hwnd_ = CreateWindowEx(0, wc_.lpszClassName, L"TrayHiddenWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);

        // Init notifications (AUMID/shortcut/protocol + WinRT)
        notifier_.Initialize();

        // Add tray icon + menu
        InitTrayIcon(hInstance);

        // Pump messages on a background thread
        std::thread([] {
            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            }).detach();
    }

    TrayIconManager::~TrayIconManager() {
        CleanupTrayIcon();
        UnregisterClass(wc_.lpszClassName, hInst_);
        self_ = nullptr;
    }

    void TrayIconManager::SetRunAction(std::function<bool()> fn) {
        runAction_ = std::move(fn);
    }

    void TrayIconManager::InitTrayIcon(HINSTANCE) {
        nid_ = {};
        nid_.cbSize = sizeof(NOTIFYICONDATA);
        nid_.hWnd = hwnd_;
        nid_.uID = 1;
        nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid_.uCallbackMessage = WM_TRAYICON;
        nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcscpy_s(nid_.szTip, L"TITAN Spoofer");

        Shell_NotifyIcon(NIM_ADD, &nid_);

        trayMenu_ = CreatePopupMenu();
        AppendMenu(trayMenu_, MF_STRING, ID_TRAY_RUN_SPOOF, L"Run Spoof");
        AppendMenu(trayMenu_, MF_STRING, ID_TRAY_EXIT, L"Exit");
    }

    void TrayIconManager::CleanupTrayIcon() {
        Shell_NotifyIcon(NIM_DELETE, &nid_);
        if (trayMenu_) DestroyMenu(trayMenu_);
        if (hwnd_)     DestroyWindow(hwnd_);
    }

    void TrayIconManager::SetMenuEnabled_(UINT id, bool enabled) {
        if (!trayMenu_) return;
        EnableMenuItem(trayMenu_, id, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
    }

    void TrayIconManager::UpdateTip_(const wchar_t* tip) {
        wcsncpy_s(nid_.szTip, tip, _TRUNCATE);
        Shell_NotifyIcon(NIM_MODIFY, &nid_);
    }

    void TrayIconManager::ShowToast_(const std::wstring& title, const std::wstring& body) {
        // fire-and-forget toast (no actions)
        notifier_.NotifyDesktop(title, body, {});
    }

    void TrayIconManager::RunAction_() {
        // Prevent re-entrancy
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true)) {
            // already running
            return;
        }

        SetMenuEnabled_(ID_TRAY_RUN_SPOOF, false);
        UpdateTip_(L"TITAN Spoofer (working…)");

        std::thread([this] {
            bool ok = false;
            try {
                if (runAction_) {
                    ok = runAction_();    // your pipeline; return true/false
                }
                else {
                    // Nothing wired: treat as success no-op
                    ok = true;
                }
            }
            catch (...) {
                ok = false;
            }

            ShowToast_(ok ? L"Spoof complete" : L"Task finished",
                ok ? L"All operations successful."
                : L"One or more operations failed.");

            UpdateTip_(L"TITAN Spoofer");
            SetMenuEnabled_(ID_TRAY_RUN_SPOOF, true);
            running_.store(false);
            }).detach();
    }

    LRESULT CALLBACK TrayIconManager::TrayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (!self_) return DefWindowProc(hwnd, msg, wParam, lParam);

        switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                POINT pt; GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(self_->trayMenu_, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                    pt.x, pt.y, 0, hwnd, nullptr);
                // Close menu reliably
                PostMessage(hwnd, WM_NULL, 0, 0);
                return 0;
            }
            if (lParam == WM_LBUTTONUP) {
                // Optional: left-click could trigger the menu as well
                POINT pt; GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(self_->trayMenu_, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                    pt.x, pt.y, 0, hwnd, nullptr);
                PostMessage(hwnd, WM_NULL, 0, 0);
                return 0;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case ID_TRAY_RUN_SPOOF:
                self_->RunAction_();  // no message boxes, runs async, shows toast on completion
                return 0;
            case ID_TRAY_EXIT:
                PostQuitMessage(0);
                return 0;
            }
            break;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

}