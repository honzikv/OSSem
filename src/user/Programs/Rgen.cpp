#include "Rgen.h"

#include "Utils/Logging.h"

bool terminated = false;
bool eof = false;

size_t Terminated_Checker(const kiv_hal::TRegisters &regs) {
	terminated = true;
	return 0;
}

size_t Eof_Checker(const kiv_hal::TRegisters &regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	
	constexpr int buffer_size = 2;
	std::vector<char> buffer(buffer_size);
	size_t read;
	Log_Debug("eof checker started");

	kiv_os_rtl::Read_File(std_in, buffer.data(), 2, read);
	while (read && !terminated) {
		if (buffer[0] == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
			break;
		}
		kiv_os_rtl::Read_File(std_in, buffer.data(), 1, read);
	}
	eof = true;
	
	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}

size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto handler = reinterpret_cast<kiv_os::TThread_Proc>(Terminated_Checker);

	kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id::Terminate, handler);


	const auto args = std::string(reinterpret_cast<const char *>(regs.rdi.r));

	std::string output;
	size_t written;

	/*if (args.empty()) {
		output = "Not enough arguments.\n";
		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument));
		return 0;
	}*/
	kiv_os::THandle handle;
	eof = false;
	kiv_os_rtl::Create_Thread("Eof_Checker", "false", std_in, std_out, handle);

	srand(static_cast <unsigned> (time(0)));


	while (!eof && !terminated) {
		const auto random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);;
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