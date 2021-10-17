#include "Rgen.h"
#include "shell.h"

extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters & regs) {

	// StdOut pro vystup
	const auto stdOut = static_cast<kiv_os::THandle>(regs.rax.x);
}
