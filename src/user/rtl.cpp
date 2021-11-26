#include "rtl.h"
#include <array>

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major major, uint8_t minor) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<uint8_t>(major);
	regs.rax.l = minor;
	return regs;
}


kiv_os::NOS_Error kiv_os_rtl::Get_Last_Err() {
	return Last_Error.load();
}


bool kiv_os_rtl::Read_File(const kiv_os::THandle file_descriptor, char* const buffer, const size_t buffer_size,
                           size_t& read) {
	kiv_hal::TRegisters regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                               static_cast<uint8_t>(kiv_os::NOS_File_System::Read_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_descriptor);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}


void kiv_os_rtl::Read_Into_Buffer(const kiv_os::THandle std_in, std::vector<char>& buffer) {
	static constexpr auto internal_buffer_size = 2048;
	auto internal_buffer = std::array<char, internal_buffer_size>();
	auto bytes_read = size_t{0};

	while (Read_File(std_in, internal_buffer.data(), internal_buffer.size(), bytes_read)) {
		// Jinak cteme dokud nedostaneme EOF
		for (size_t i = 0; i < bytes_read; i += 1) {
			if (internal_buffer[i] == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
				return;
			}
			buffer.push_back(internal_buffer[i]);
		}

		if (bytes_read == 0) {
			return;
		}
	}
}


bool kiv_os_rtl::Write_File(const kiv_os::THandle file_descriptor, const char* buffer, const size_t buffer_size,
                            size_t& written) {
	kiv_hal::TRegisters regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                               static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_descriptor);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	written = regs.rax.r;
	return result;
}


bool kiv_os_rtl::Create_Pipe(kiv_os::THandle& writing_process_output, kiv_os::THandle& reading_process_input) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Create_Pipe));

	kiv_os::THandle pipes[] = {writing_process_output, reading_process_input};
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(std::addressof(pipes));

	const auto success = kiv_os::Sys_Call(regs);
	writing_process_output = pipes[0];
	reading_process_input = pipes[1];

	return success;
}


bool kiv_os_rtl::Open_File(kiv_os::THandle& file_descriptor, const std::string& file_uri, kiv_os::NOpen_File mode,
                           const kiv_os::NFile_Attributes attributes) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Open_File));

	// Nastavime parametry
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_uri.c_str());
	regs.rcx.r = static_cast<decltype(regs.rcx.r)>(mode);
	regs.rdi.r = static_cast<decltype(regs.rdi.r)>(attributes);

	// Vytvorime syscall a ziskame file descriptor pokud success
	const auto success = kiv_os::Sys_Call(regs);
	file_descriptor = regs.rax.x;

	return success;
}


bool kiv_os_rtl::Seek(kiv_os::THandle file_descriptor, const uint64_t new_pos, kiv_os::NFile_Seek seek_type) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Seek));

	regs.rdx.x = file_descriptor;
	regs.rdi.r = new_pos;
	regs.rcx.l = static_cast<decltype(regs.rcx.l)>(seek_type);

	return kiv_os::Sys_Call(regs);
}


bool kiv_os_rtl::Delete_File(const std::string& file_name) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Delete_File));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name.c_str());

	return kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::Set_File_Attribute(const std::string& file_name, kiv_os::NFile_Attributes attributes,
                                    kiv_os::NFile_Attributes& new_attributes) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Set_File_Attribute));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name.c_str());

	const auto result = kiv_os::Sys_Call(regs);
	new_attributes = static_cast<kiv_os::NFile_Attributes>(regs.rdi.r);

	return result;
}

bool kiv_os_rtl::Get_File_Attributes(const std::string& file_name, kiv_os::NFile_Attributes& attributes) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Set_File_Attribute));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name.c_str());

	const auto result = kiv_os::Sys_Call(regs);
	attributes = static_cast<kiv_os::NFile_Attributes>(regs.rdi.r);

	return result;
}


bool kiv_os_rtl::Close_File_Descriptor(const kiv_os::THandle file_descriptor) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Close_Handle));
	regs.rdx.x = static_cast<uint16_t>(file_descriptor);

	return kiv_os::Sys_Call(regs);
}


bool kiv_os_rtl::Set_Working_Dir(const std::string& params) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
	                                static_cast<uint8_t>(kiv_os::NOS_File_System::Set_Working_Dir));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(params.c_str());
	regs.rcx.e = static_cast<decltype(regs.rcx.e)>(params.size());

	return kiv_os::Sys_Call(regs);
}


bool kiv_os_rtl::Get_Working_Dir(char* buffer, const uint32_t new_dir_buffer_size, uint32_t& new_directory_str_size) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::File_System,
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


bool kiv_os_rtl::Create_Process(const std::string& program_name, const std::string& params,
                                const kiv_os::THandle std_in,
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


bool kiv_os_rtl::Create_Thread(const std::string& program_name, const std::string& params, const kiv_os::THandle std_in,
                               const kiv_os::THandle std_out) {
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


bool kiv_os_rtl::Wait_For(const std::vector<kiv_os::THandle>& handles) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::Process,
	                                static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles.data());
	regs.rcx.e = static_cast<decltype(regs.rcx.e)>(handles.size());

	return kiv_os::Sys_Call(regs);
}


bool kiv_os_rtl::Read_Exit_Code(kiv_os::THandle pid, kiv_os::NOS_Error& exit_code) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::Process,
	                                static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));

	regs.rdx.x = pid;

	if (!kiv_os::Sys_Call(regs)) {
		return false;
	}

	exit_code = static_cast<kiv_os::NOS_Error>(regs.rcx.x);
	return true;
}

bool kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id signal, kiv_os::TThread_Proc callback) {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::Process,
		static_cast<uint8_t>(kiv_os::NOS_Process::Register_Signal_Handler));

	regs.rcx.r = static_cast<decltype(regs.rcx.r)>(signal);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(std::addressof(callback));

	return kiv_os::Sys_Call(regs);
}


void kiv_os_rtl::Shutdown() {
	auto regs = Prepare_Syscall_Ctx(kiv_os::NOS_Service_Major::Process,
	                                static_cast<uint8_t>(kiv_os::NOS_Process::Shutdown));

	kiv_os::Sys_Call(regs);
}
