#pragma once
#include "../api/hal.h"
#include "IO/HandleService.h"

void Set_Err(const bool failed, kiv_hal::TRegisters& regs);
void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context);

/// <summary>
/// Funkce pro systemove volani
/// </summary>
/// <param name="regs">registry s kontextem</param>
void Syscall(kiv_hal::TRegisters& regs);

inline HMODULE User_Programs;
