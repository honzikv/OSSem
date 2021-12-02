#pragma once

#include <Windows.h>

#include "../api/hal.h"
#include "Process/InitProcess.h"

void Set_Err(const bool failed, kiv_hal::TRegisters& regs);
void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context);

/// <summary>
/// Funkce pro systemove volani
/// </summary>
/// <param name="regs">registry s kontextem</param>
void __stdcall Syscall(kiv_hal::TRegisters& regs);

inline HMODULE User_Programs;
