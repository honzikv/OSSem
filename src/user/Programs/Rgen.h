#pragma once

#include "../../api/api.h"

struct eof_checker_params {
    kiv_os::THandle std_in;
    bool* is_eof;
};

extern "C" size_t __stdcall checker_for_eof(const kiv_hal::TRegisters & regs);
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs);
