#include "Rgen.h"

#include <random>
#include <atomic>
#include "../Utils/Logging.h"

std::atomic<bool> terminated = false;
auto eof = false;

size_t __stdcall Terminated_Checker(const kiv_hal::TRegisters& regs) {
	terminated = true;
	return 0;
}

// Parametry pro EOF checker
struct EofParams {
	bool* is_eof;
	kiv_os::THandle std_in;
	kiv_os::THandle std_out;
};

extern "C" size_t __stdcall checker_for_eof(const kiv_hal::TRegisters& regs) {
	// Parametry ziskame z rdi
	const auto eof_params = *reinterpret_cast<EofParams*>(regs.rdi.r);
	const auto std_in = eof_params.std_in;
	const auto std_out = eof_params.std_out;
	auto is_eof = eof_params.is_eof;


	constexpr int buffer_size = 256;
	std::vector<char> buffer(buffer_size);
	size_t read = 1;
	
	while (read && !terminated) {
		if (!kiv_os_rtl::Read_File(std_in, buffer.data(), buffer_size, read)) {
			terminated = true;
			break;
		}
		for (auto c : buffer) {
			if (c == static_cast<char>(kiv_hal::NControl_Codes::EOT)
				|| c == static_cast<char>(kiv_hal::NControl_Codes::ETX)
				|| c == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
				terminated = true;
				break;
			}
		}
	}
	*is_eof = true;
	terminated = true;
	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}

size_t __stdcall rgen(const kiv_hal::TRegisters& regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto handler = reinterpret_cast<kiv_os::TThread_Proc>(Terminated_Checker);

	kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id::Terminate, handler);

	const auto args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	std::string output;
	size_t written;

	kiv_os::THandle handle;
	eof = false;
	terminated = false;

	auto params = EofParams{ &eof, std_in, std_out };

	if (!kiv_os_rtl::Create_Thread(checker_for_eof, reinterpret_cast<char*>(&params), handle)) {
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Out_Of_Memory));
		return 1;
	}

	// ziska random generator od hardwaru
	std::random_device rand;
	// nastavi seed
	std::mt19937 gen(rand());
	std::uniform_real_distribution distr(-1000.0, 1000.0);

	while (!eof && !terminated) {
		const auto random = static_cast<float>(distr(gen));
		output = std::to_string(random);
		output.append("\n");
		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
	}

	std::vector<kiv_os::THandle> handles;
	handles.push_back(handle);

	if (terminated) {
		kiv_os::NOS_Error checker_exit_code;
		kiv_os_rtl::Wait_For(handles);
		kiv_os_rtl::Read_Exit_Code(handle, checker_exit_code);
	}

	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}