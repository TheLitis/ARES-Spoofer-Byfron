#include "Container/Services/Services.hpp"

#include "Container/Header/TraceCleaner.h"
#include "Container/Header/Installer.h"
#include "Container/Header/Mac.h"
#include "Container/Header/Registry.h"
#include "Container/Header/WMI.h"
#include "Container/Header/Watchdog.h"
#include "Container/Header/pMask.h"

#include "Container/System/Notify.h"
#include "Container/System/Tray.h"

#include <iostream>
#include <thread>

int TsRun(bool quiet) {

    TITAN::TsBlockHandle guard;

    if (TITAN::Notification::HandleProtocolIfPresentAndExitEarly()) {
        return 0;
    }

    TsService::TITAN();

    TITAN::Notification notif;
    notif.Initialize();

    TITAN::Watchdog wd(L"RobloxPlayerBeta.exe");

    wd.setOnAllExited([&]() {
        wd.pause();

        bool agreed = false;
        if (notif.PromptSpoofConsentAndWait(agreed) && agreed) {
            bool success = true;
            try {
                TsService::__TerminateRoblox();
                TraceCleaner::run();
                MAC::MacSpoofer::run();
                Registry::RegSpoofer::run();
                WMI::WmiSpoofer::run();
                Installer::Install(wd);

                std::this_thread::sleep_for(std::chrono::seconds(3));
                TsService::__TerminateRoblox();
            }
            catch (...) {
                success = false;
            }

            notif.NotifyDesktop(
                success ? L"Spoof complete" : L"Spoof failed",
                success ? L"Done."
                : L"One or more operations failed.");
        }

        wd.resume();
     });

    if (!wd.start()) {
        if (!quiet)
            std::cerr << "[!] Failed to start watchdog\n";
        return 1;
    }

    if (!quiet)
        std::wcout << L"[*] Watchdog running... press Ctrl+C to exit.\n";

    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    wd.stop();

    return 0;
}

int SafeRun(bool quiet) {
    for (;;) {
        try {
            int code = TsRun(quiet);
            if (code == 0) {
                return 0;
            }
            else {
                std::cerr << "[!] TsRun exited with code " << code << ", restarting...\n";
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "[!] Unhandled exception: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[!] Unknown fatal exception, restarting...\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

// Release (no-console)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    TsService::TsAdjustAccess();

    return TsRun(true);
}

// Debug (console)
int main() {
    TsService::TsAdjustAccess();

    return SafeRun(false);
}