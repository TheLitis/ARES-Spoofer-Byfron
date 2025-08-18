#include "../Header/TraceCleaner.h"
#include "../Services/Services.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <optional>
#include <vector>
#include <iostream>

namespace fs = std::filesystem;

namespace {

    struct PathHelper {
        static std::wstring user() { return TsService::GetUser(); }
        static std::wstring sys() { return TsService::GetSysDrive(); }

        static std::optional<fs::path> resolveShortcut(const std::wstring& name) {
            std::vector<fs::path> shortcuts = {
                fs::path(sys()) / L"ProgramData" / L"Microsoft" / L"Windows" / L"Start Menu" / L"Programs" / name,
                fs::path(user()) / L"AppData" / L"Roaming" / L"Microsoft" / L"Windows" / L"Start Menu" / L"Programs" / name
            };

            for (const auto& p : shortcuts) {
                if (fs::exists(p)) {
                    auto target = TsService::ResolveTarget(p);
                    if (!target.empty() && fs::exists(target))
                        return fs::path(target);
                }
            }
            return std::nullopt;
        }
    };

    void cleanVers(const fs::path& baseDir) {
        if (!fs::exists(baseDir)) return;
        for (const auto& d : fs::directory_iterator(baseDir)) {
            const auto name = d.path().filename().wstring();
            if (d.is_directory() && name.rfind(L"version-", 0) == 0) {
                TsService::BulkDelete(d.path(), {
                    L"RobloxPlayerBeta.exe",
                    L"RobloxPlayerBeta.dll",
                    L"RobloxCrashHandler.exe",
                    L"RobloxPlayerLauncher.exe"
                    });
            }
        }
    }

    void RmvReferents(const fs::path& filePath, const std::wstring& itemClass) {
        if (!fs::exists(filePath))
            throw std::runtime_error("File does not exist: " + filePath.string());

        std::wifstream in(filePath);
        if (!in.is_open())
            throw std::runtime_error("Failed to open file: " + filePath.string());

        std::wstringstream buf;
        buf << in.rdbuf();
        in.close();

        std::wstring content = buf.str();
        std::wregex rx(LR"(<Item class=\")" + itemClass + LR"(" referent=\"[^\"]+\">)");
        std::wstring rep = L"<Item class=\"" + itemClass + L"\" referent=\"" + TsService::genRand() + L"\">";

        content = std::regex_replace(content, rx, rep);

        std::wofstream out(filePath);
        if (!out.is_open())
            throw std::runtime_error("Failed to write to file -> " + filePath.string());
        out << content;
    }

    void CleanRbx() {
        fs::path userBase = fs::path(PathHelper::user()) / L"AppData/Local/Roblox";

        if (auto robLnk = PathHelper::resolveShortcut(L"Roblox Player.lnk")) {
            cleanVers(robLnk->parent_path().parent_path());
        }

        cleanVers(fs::path(PathHelper::sys()) / L"Program Files (x86)/Roblox/Versions");

        if (auto bxLnk = PathHelper::resolveShortcut(L"Bloxstrap.lnk")) {
            cleanVers(bxLnk->parent_path() / L"Versions");
        }

        cleanVers(fs::path(PathHelper::user()) / L"AppData/Local/Bloxstrap/Versions");

        if (auto fsLnk = PathHelper::resolveShortcut(L"Fishstrap.lnk")) {
            cleanVers(fsLnk->parent_path() / L"Versions");
        }

        cleanVers(fs::path(PathHelper::user()) / L"AppData/Local/Fishstrap/Versions");

        std::vector<std::wstring> dirsToDelete = {
            L"Temp/Roblox",
            L"Roblox/logs",
            L"Roblox/LocalStorage",
            L"Roblox/Downloads",
            L"Roblox/ClientSettings",
            L"Roblox/rbx-storage",
            L"Roblox/Versions"
        };

        for (const auto& sub : dirsToDelete) {
            fs::remove_all(fs::path(PathHelper::user()) / L"AppData/Local" / sub);
            std::wcout << L"Deleted -> " << fs::path(PathHelper::user()) / L"AppData/Local" / sub << std::endl;
        }

        std::vector<std::wstring> filesToDelete = {
            L"rbx-storage.db",
            L"rbx-storage.db-shm",
            L"rbx-storage.db-wal",
            L"rbx-storage.id",
            L"frm.cfg"
        };

        for (const auto& file : filesToDelete) {
            fs::remove(fs::path(userBase) / file);
            std::wcout << L"Deleted -> " << (userBase / file) << std::endl;
        }

        RmvReferents(userBase / L"GlobalBasicSettings_13.xml", L"UserGameSettings");
        RmvReferents(userBase / L"GlobalSettings_13.xml", L"UserGameSettings");
        RmvReferents(userBase / L"AnalysticsSettings.xml", L"GoogleAnalyticsConfiguration");
    }
}

namespace TraceCleaner {
    void run() {
        TsService::SectHeader("File System Cleaning", 202);
        try {
            CleanRbx();
        }
        catch (const std::exception& ex) {
            std::cerr << "TraceCleaner::Run() error: " << ex.what() << '\n';
        }
    }
}