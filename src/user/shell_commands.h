#pragma once

#include "../api/api.h"
#include <thread>

#include "rtl.h"

//nasledujici funkce si dejte do vlastnich souboru
//cd nemuze byt externi program, ale vestavny prikaz shellu!

// TODO zmenit - tohle je pouze pro debug
extern "C" size_t __stdcall type(const kiv_hal::TRegisters& regs) { 

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::cout << "Hello Thread" << std::endl;
	}

	return 0;

};

// TODO zmenit
extern "C" size_t __stdcall md(const kiv_hal::TRegisters& regs) { 
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	// Spustime type
	const auto program = "type";
	const auto params = "";
	kiv_os_rtl::CreateThread(program, params, std_in, std_out);

	std::this_thread::sleep_for(std::chrono::seconds(8));

	return 0;
}
extern "C" size_t __stdcall rd(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall dir(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall echo(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall find(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall sort(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters& regs) { return 0; }

extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters& regs) { return 0; }

