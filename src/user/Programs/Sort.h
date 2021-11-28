#pragma once

#include "..\api\api.h"
#include "rtl.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <list>

extern "C" size_t __stdcall sort(const kiv_hal::TRegisters & regs);
