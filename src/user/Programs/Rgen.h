#pragma once

#include "..\api\api.h"
#include "rtl.h"
#include "time.h"

#include <string>

extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs);
