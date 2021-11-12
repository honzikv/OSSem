#include "rtl.h"

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters PrepareSyscallContext(kiv_os::NOS_Service_Major major, uint8_t minor) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<uint8_t>(major);
	regs.rax.l = minor;
	return regs;
}


kiv_os::NOS_Error kiv_os_rtl::GetLastError() {
	return Last_Error.load();
}

bool kiv_os_rtl::ReadFile(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size,
                          size_t& read) {
	kiv_hal::TRegisters regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::File_System,
	                                                 static_cast<uint8_t>(kiv_os::NOS_File_System::Read_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}

bool kiv_os_rtl::WriteFile(const kiv_os::THandle file_handle, const char* buffer, const size_t buffer_size,
                           size_t& written) {
	kiv_hal::TRegisters regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::File_System,
	                                                 static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	written = regs.rax.r;
	return result;
}

bool kiv_os_rtl::CreatePipe(kiv_os::THandle& input, kiv_os::THandle& output) {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::File_System,
	                                  static_cast<uint8_t>(kiv_os::NOS_File_System::Create_Pipe));

	kiv_os::THandle pipes[] = {input, output};
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(std::addressof(pipes));

	input = pipes[0];
	output = pipes[1];
	return kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::OpenFsFile(kiv_os::THandle& file_descriptor, const std::string& file_uri, kiv_os::NOpen_File mode) {
	// TODO impl
	return false;
}

bool kiv_os_rtl::CloseHandle(const kiv_os::THandle file_descriptor) {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::File_System,
	                                  static_cast<uint8_t>(kiv_os::NOS_File_System::Close_Handle));
	regs.rdx.x = static_cast<uint16_t>(file_descriptor);

	return kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::SetWorkingDir(const std::string& params) {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::File_System,
	                                  static_cast<uint8_t>(kiv_os::NOS_File_System::Set_Working_Dir));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(params.c_str());
	regs.rcx.e = static_cast<decltype(regs.rcx.e)>(params.size());

	return kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::GetWorkingDir(char* buffer, const uint32_t new_dir_buffer_size, uint32_t& new_directory_str_size) {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::File_System,
	                                  static_cast<uint8_t>(kiv_os::NOS_File_System::Get_Working_Dir));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.e = new_dir_buffer_size;

	if (!kiv_os::Sys_Call(regs)) {
		// pokud doslo k chybe rovnou se vratime
		return false;
	}

	// Jinak nastavime velikost stringu
	new_directory_str_size = regs.rax.e;
	return true;
}

bool kiv_os_rtl::CreateProcess(const std::string& program_name, const std::string& params, const kiv_os::THandle std_in,
                               const kiv_os::THandle std_out, kiv_os::THandle& pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Clone);
	regs.rcx.l = static_cast<decltype(regs.rcx.l)>(kiv_os::NClone::Create_Process);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(program_name.c_str());
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(params.c_str());
	regs.rbx.r = static_cast<decltype(regs.rbx.r)>(std_in << 16 | std_out);

	if (!kiv_os::Sys_Call(regs)) {
		return false;
	}

	pid = regs.rax.x;
	return true;
}

bool kiv_os_rtl::CreateThread(const std::string& program_name, const std::string& params, kiv_os::THandle std_in,
	kiv_os::THandle std_out) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Clone);
	regs.rcx.l = static_cast<decltype(regs.rcx.l)>(kiv_os::NClone::Create_Thread);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(program_name.c_str());
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(params.c_str());
	regs.rbx.r = static_cast<decltype(regs.rbx.r)>(std_in << 16 | std_out);

	if (!kiv_os::Sys_Call(regs)) {
		return false;
	}
	
	return true;
}

bool kiv_os_rtl::WaitFor(const std::vector<kiv_os::THandle>& handles) {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::Process,
		static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles.data());
	regs.rcx.e = static_cast<decltype(regs.rcx.e)>(handles.size());

	return kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::ReadExitCode(kiv_os::THandle pid, kiv_os::NOS_Error& exit_code) {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::Process,
		static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));

	regs.rdx.x = pid;

	if (!kiv_os::Sys_Call(regs)) {
		return false;
	}

	exit_code = static_cast<kiv_os::NOS_Error>(regs.rcx.x);
	return true;
}

void kiv_os_rtl::Shutdown() {
	auto regs = PrepareSyscallContext(kiv_os::NOS_Service_Major::Process,
		static_cast<uint8_t>(kiv_os::NOS_Process::Shutdown));

	kiv_os::Sys_Call(regs);
}
