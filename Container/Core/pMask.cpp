#include "../Header/pMask.h"

#include <shlwapi.h>
#include <sddl.h>
#include <tchar.h>
#include <aclapi.h>

#include <iostream>
#include <algorithm>

#include <cwctype>

#pragma comment(lib, "Shlwapi.lib")

namespace TITAN {

    bool LaunchDaemon(bool debugMode) {
        wchar_t selfPath[MAX_PATH];
        if (!GetModuleFileNameW(nullptr, selfPath, MAX_PATH)) return false;

        std::wstring dir = selfPath;
        PathRemoveFileSpecW(&dir[0]);
        dir.resize(wcslen(dir.c_str()));

        std::wstring randName = TsService::genRand(5) + L".exe";
        std::transform(randName.begin(), randName.end(), randName.begin(), [](wchar_t c) {
            return std::towlower(c);
            });

        std::wstring daemonPath = dir + L"\\" + randName;

        if (!CopyFileW(selfPath, daemonPath.c_str(), FALSE)) {
            std::wcerr << L"[!] Failed to copy self to " << daemonPath << L" (" << GetLastError() << L")\n";
            return false;
        }

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi{};
        if (!CreateProcessW(daemonPath.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            std::wcerr << L"[!] Failed to launch daemon (" << GetLastError() << L")\n";
            DeleteFileW(daemonPath.c_str());
            return false;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        DeleteFileW(daemonPath.c_str());

        return true;
    }

    TsBlockHandle::TsBlockHandle() {
        success_ = Harden();
    }

    TsBlockHandle::~TsBlockHandle() {
        // Cleanup handled by RAII
    }

    bool TsBlockHandle::ok() const {
        return success_;
    }

    bool TsBlockHandle::Harden() {
        HANDLE hProcess = (HANDLE)-1;
        PACL pNewDACL = nullptr;
        PSECURITY_DESCRIPTOR pSD = nullptr;

        LPCWSTR sddl = L"D:P(A;;GA;;;SY)(A;;GA;;;OW)";

        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &pSD, NULL)) {
            std::wcerr << L"[TsBlockHandle] ConvertStringSecurityDescriptor failed: "
                << GetLastError() << std::endl;
            return false;
        }

        BOOL daclPresent, daclDefaulted;
        if (!GetSecurityDescriptorDacl(pSD, &daclPresent, &pNewDACL, &daclDefaulted)) {
            std::wcerr << L"[TsBlockHandle] GetSecurityDescriptorDacl failed: "
                << GetLastError() << std::endl;
            LocalFree(pSD);
            return false;
        }

        DWORD dwRes = SetSecurityInfo(
            hProcess,
            SE_KERNEL_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
            NULL, NULL, pNewDACL, NULL);

        if (dwRes != ERROR_SUCCESS) {
            std::wcerr << L"[TsBlockHandle] SetSecurityInfo failed: " << dwRes << std::endl;
            LocalFree(pSD);
            return false;
        }

        LocalFree(pSD);
        return true;
    }
}