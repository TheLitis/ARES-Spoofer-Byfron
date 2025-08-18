#pragma once
#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <functional>
#include <string>

#include "Notify.h"   // TITAN::Notification

#define WM_TRAYICON         (WM_USER + 1)
#define ID_TRAY_RUN_SPOOF   1001
#define ID_TRAY_EXIT        1002

namespace TITAN::Tray {

    class TrayIconManager {
    public:
        explicit TrayIconManager(HINSTANCE hInstance);
        ~TrayIconManager();

        // Provide the action to run when the user clicks "Run Spoof".
        // Return true for success, false for failure (controls toast text).
        void SetRunAction(std::function<bool()> fn);

    private:
        // Window proc for the hidden tray window
        static LRESULT CALLBACK TrayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // Init/teardown
        void InitTrayIcon(HINSTANCE hInstance);
        void CleanupTrayIcon();

        // Helpers
        void RunAction_();                           // kicks off a worker thread once
        void ShowToast_(const std::wstring& title, const std::wstring& body);
        void SetMenuEnabled_(UINT id, bool enabled);
        void UpdateTip_(const wchar_t* tip);

        // State
        HWND hwnd_{};
        HMENU trayMenu_{};
        NOTIFYICONDATA nid_{};
        HINSTANCE hInst_{};
        WNDCLASSEX wc_{};
        std::atomic_bool running_{ false };
        std::function<bool()> runAction_{};

        // Notifications (WinRT/COM init handled internally)
        TITAN::Notification notifier_{ L"TITAN.Spoofer", L"TITAN Spoofer" };

        // global self for window proc
        static inline TrayIconManager* self_ = nullptr;
    };

} // namespace TITAN::Tray