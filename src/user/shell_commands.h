#pragma once

#include "../api/api.h"
#include <thread>

#include "rtl.h"

//nasledujici funkce si dejte do vlastnich souboru
//cd nemuze byt externi program, ale vestavny prikaz shellu!

// TODO zmenit - tohle je pouze pro debug
extern "C" size_t __stdcall type(const kiv_hal::TRegisters& regs) {
	// Debug zavirani vlakna
	//
	// const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	// while (true) {
	// 	std::this_thread::sleep_for(std::chrono::seconds(1));
	// 	std::cout << "Hello Thread" << std::endl;
	// }
	//
	// return 0;

	// Debug pipy
	LogDebug("Type std_out is : " + std::to_string(std_out));
	auto string_stream = std::stringstream();
	for (size_t i = 0; i < 1024; i += 1) {
		string_stream << "Pipe Test";
	}

	auto buffer = string_stream.str();
	auto written = size_t{0};
	auto success = kiv_os_rtl::WriteFile(std_out, buffer.data(), buffer.size(), written);
	if (!success) {
		return 1;
	}

	return 0;
};

// TODO zmenit
extern "C" size_t __stdcall md(const kiv_hal::TRegisters& regs) {
	// Debug zavirani vlakna
	// const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	// const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	//
	// // Spustime type
	// const auto program = "type";
	// const auto params = "";
	// kiv_os_rtl::CreateThread(program, params, std_in, std_out);
	//
	// std::this_thread::sleep_for(std::chrono::seconds(8));
	//
	// return 0;
	// Debug pipy
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	auto written = size_t{0};
	const auto message_md = std::string{"MD start"};

	kiv_os_rtl::WriteFile(std_out, message_md.data(), message_md.size(), written);

	LogDebug("md std_in is : " + std::to_string(std_in) + " std_out is " + std::to_string(std_out));
	auto buffer = std::vector<char>();
	kiv_os_rtl::ReadIntoBuffer(std_in, buffer);

	auto success = kiv_os_rtl::WriteFile(std_out, buffer.data(), buffer.size(), written);
	if (!success) {
		return 1;
	}

	return 0;
}

extern "C" size_t __stdcall rd(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall dir(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall echo(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall find(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall sort(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters& regs) { return 0; }

extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters& regs) { return 0; }
