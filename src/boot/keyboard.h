#pragma once

#include "../api/hal.h"

bool Init_Keyboard();

void __stdcall Keyboard_Handler(kiv_hal::TRegisters &context);