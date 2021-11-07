#pragma once
#include "..\api\hal.h"
#include "..\api\api.h"
#include "Process/Process.h"
#include "handles.h"

void Set_Error(const bool failed, kiv_hal::TRegisters& regs);
void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context);

inline HMODULE User_Programs;
