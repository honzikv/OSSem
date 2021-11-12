#pragma once

#include "../../api/api.h"
#include "../rtl.h"

extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters & _) {
	kiv_os_rtl::Shutdown();
	return 0;
}