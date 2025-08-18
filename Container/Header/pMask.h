#pragma once

#include <windows.h>

#include "../Services/Services.hpp"

namespace TITAN {

    // TODO: replace with NtCreateUserProcessEx
    // FUNCTION UNUSED (FUTURE)
    bool LaunchDaemon(bool debugMode);

    class TsBlockHandle {
    public:
        TsBlockHandle();
        ~TsBlockHandle();

        bool ok() const;

    private:
        bool Harden();
        bool success_{ false };
    };
}