#pragma once
#include "../../api/api.h"

extern "C" size_t __stdcall sort(const kiv_hal::TRegisters & regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
}
