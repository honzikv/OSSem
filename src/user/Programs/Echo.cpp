#include "Echo.h"

size_t __stdcall echo(const kiv_hal::TRegisters& regs) {
	auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	auto* args = reinterpret_cast<const char*>(regs.rdi.r);

	std::string output;

	if (strlen(args) > 0) {		
	
		if (strcmp(args, ECHO_HELP) == 0) {
			output.append("ECHO command displays messages or turns echoing of the command off.\n");
			output.append("   ECHO [ON | OFF]\n");
			output.append("   ECHO [message]\n");
			output.append("To display current ECHO settings, type ECHO without any parameters.\n");

			size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
			kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		} else if (strcmp(args, ECHO_ON) == 0) {
			echo_on = true;
		} else if (strcmp(args, ECHO_OFF) == 0) {
			echo_on = false;
		} else { // je argument, ale neni vyznamovy, tak vse vypisu
			output.append(args);
			output.append("\n");
			size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
			kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		}
	} else {
		if (echo_on) {
			output.append("Echo ON.\n");

			size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
			kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		} else {
			output.append("Echo OFF.\n");

			size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
			kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		}
	}
	uint16_t exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Success);
	kiv_os_rtl::Exit(exit_code);
	return 0;
}