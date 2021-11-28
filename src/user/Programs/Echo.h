#pragma once

#include "rtl.h"
#include "..\api\api.h"

#include <string>

#define ECHO_ON "on"
#define ECHO_OFF "off"
#define ECHO_HELP "/?"

inline extern bool echo_on = true;

extern "C" size_t __stdcall echo(const kiv_hal::TRegisters & regs);