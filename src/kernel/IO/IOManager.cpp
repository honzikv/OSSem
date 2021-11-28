#include "IOManager.h"

#include "ConsoleIn.h"
#include "ConsoleOut.h"
#include "ReadablePipe.h"
#include "WritablePipe.h"
#include "Utils/Logging.h"
#include "fs_file.h"
#include "ProcFS.h"
#include "Process/ProcessManager.h"

void IOManager::Handle_IO(kiv_hal::TRegisters& regs) {
	const auto operation = regs.rax.l;
	auto operation_result = kiv_os::NOS_Error::Success;
	switch (static_cast<kiv_os::NOS_File_System>(operation)) {
		case kiv_os::NOS_File_System::Read_File: {
			operation_result = Syscall_Read(regs);
			break;
		}

		case kiv_os::NOS_File_System::Write_File: {
			operation_result = Syscall_Write(regs);
			break;
		}

		case kiv_os::NOS_File_System::Create_Pipe: {
			operation_result = Syscall_Create_Pipe(regs);
			break;
		}

		case kiv_os::NOS_File_System::Close_Handle: {
			operation_result = Syscall_Close_Handle(regs);
			break;
		}

		case kiv_os::NOS_File_System::Set_Working_Dir: {
			operation_result = Syscall_Set_Working_Dir(regs);
			break;
		}

		case kiv_os::NOS_File_System::Get_Working_Dir: {
			operation_result = Syscall_Get_Working_Dir(regs);
			break;
		}

		case kiv_os::NOS_File_System::Get_File_Attribute: {
			operation_result = Syscall_Get_File_Attribute(regs);
			break;
		}

		case kiv_os::NOS_File_System::Open_File: {
			operation_result = Syscall_Open_File(regs);
			break;
		}
		case kiv_os::NOS_File_System::Delete_File: {
			operation_result = Syscall_Delete_File(regs);
			break;
		}
		case kiv_os::NOS_File_System::Seek: {
			operation_result = Syscall_Seek(regs);
			break;
		}
		case kiv_os::NOS_File_System::Set_File_Attribute: {
			operation_result = Syscall_Set_File_Attribute(regs);
			break;
		}

	}

	if (operation_result != kiv_os::NOS_Error::Success) {
		regs.flags.carry = 1;
		regs.rax.x = static_cast<decltype(regs.rax.x)>(operation_result);
	}

}

auto IOManager::Create_Stdio() -> std::pair<kiv_os::THandle, kiv_os::THandle> {
	const auto std_in = std::make_shared<ConsoleIn>();
	const auto std_out = std::make_shared<ConsoleOut>();

	const auto std_in_handle = HandleService::Get().Get_Empty_Handle();
	const auto std_out_handle = HandleService::Get().Get_Empty_Handle();

	// Pro stdio chceme vyskyt na nule, protoze stdio se musi predat procesu
	open_files[std_in_handle] = {0, std_in};
	open_files[std_out_handle] = {0, std_out};

	return {std_in_handle, std_out_handle};
}


kiv_os::NOS_Error IOManager::Syscall_Read(kiv_hal::TRegisters& regs) {
	std::shared_ptr<IFile> file;
	const auto process = ProcessManager::Get().Get_Current_Process();
	if (process == nullptr) {
		return kiv_os::NOS_Error::Permission_Denied;
	}
	{
		const auto file_descriptor = regs.rdx.x;
		auto lock = std::scoped_lock(mutex);
		if (!Is_File_Descriptor_Accessible(process->Get_Pid(), file_descriptor)) {
			regs.flags.carry = 1;
			return kiv_os::NOS_Error::File_Not_Found;
		}

		file = open_files.at(file_descriptor).second; // file pointer je druhy prvek v dvojici
	}

	const auto buffer = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)
	const auto bytes = regs.rcx.r; // kolik bytu se ma precist
	auto bytes_read = size_t{0}; // pocet prectenych bytu

	const auto op_result = file->Read(buffer, bytes, bytes_read);

	// Pokud je vysledek success vratime pocet bytu v rax
	if (op_result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytes_read;
		return kiv_os::NOS_Error::Success;
	}

	// Jinak nastavime flag a do rax zapiseme chybu
	regs.flags.carry = 1;
	return op_result;
}

kiv_os::NOS_Error IOManager::Syscall_Write(kiv_hal::TRegisters& regs) {
	std::shared_ptr<IFile> file;
	const auto process = ProcessManager::Get().Get_Current_Process();
	if (process == nullptr) {
		return kiv_os::NOS_Error::Permission_Denied;
	}
	{
		const auto file_descriptor = regs.rdx.x;
		auto lock = std::scoped_lock(mutex);
		if (!Is_File_Descriptor_Accessible(process->Get_Pid(), file_descriptor)) {
			regs.flags.carry = 1;
			return kiv_os::NOS_Error::File_Not_Found;
		}

		file = open_files.at(file_descriptor).second; // file pointer je druhy prvek v dvojici
	}

	const auto buffer = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)
	const auto bytes = regs.rcx.r;

	auto bytes_written = size_t{0};

	const auto op_result = file->Write(buffer, bytes, bytes_written);

	// Pokud je vysledek Success vratime pocet bytu v rax
	if (op_result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytes_written;
		return kiv_os::NOS_Error::Success;
	}

	// Jinak nastavime flag a vratime chybu
	regs.flags.carry = 1;
	return op_result;
}

kiv_os::NOS_Error IOManager::Syscall_Create_Pipe(const kiv_hal::TRegisters& regs) {
	const auto pipe_read_write_pair = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	// NOLINT(performance-no-int-to-ptr)
	auto pipe_write = pipe_read_write_pair[0];
	auto input_preinitialized = false;
	auto pipe_read = pipe_read_write_pair[1];

	// Pipe musime vytvorit jako shared pointer, jinak dojde ke kopirovani a my potrebujeme
	// alokaci na jednom miste
	const auto pipe = std::make_shared<Pipe>();

	// Aby se zavolali spravne metody pro zavreni, musi byt pipe ve svem danem wrapperu, ktery zavola
	// bud close pro cteni nebo pro zapis
	const auto readable_pipe = std::make_shared<ReadablePipe>(pipe);
	const auto writable_pipe = std::make_shared<WritablePipe>(pipe);

	// Pokud je zapis do pipy invalid, musime ziskat novy handle
	if (pipe_write == kiv_os::Invalid_Handle) {
		input_preinitialized = true;
		const auto file_descriptor = HandleService::Get().Get_Empty_Handle();
		if (file_descriptor == kiv_os::Invalid_Handle) {
			// dosly handly, vyhodime vyjimku
			return kiv_os::NOS_Error::Out_Of_Memory;
		}

		// Jinak nastavime file descriptor
		pipe_write = file_descriptor;
	}

	// To same pro cteni z pipy
	if (pipe_read == kiv_os::Invalid_Handle) {
		const auto file_descriptor = HandleService::Get().Get_Empty_Handle();
		if (file_descriptor == kiv_os::Invalid_Handle) {
			// dosly handly, vyhodime vyjimku
			if (input_preinitialized) {
				// Navic musime jeste provest check, zda-li je potreba odstranit i input handle
				HandleService::Get().Remove_Handle(pipe_write);
			}

			return kiv_os::NOS_Error::Out_Of_Memory; // Dosla pamet takze nic delat nemusime
		}

		pipe_read = file_descriptor;
	}

	{
		const auto current_process_pid = ProcessManager::Get().Get_Current_Pid();
		// Ulozime pipy do otevrenych souboru a k procesu, ktery tento syscall zavolal
		auto lock = std::scoped_lock(mutex);

		// Zde chceme -1, protoze pipe bude zaregistrovana jak u shellu, tak i pro proces, ktery ji bude vyuzivat
		// protoze chceme aby oba mohli zavolat shutdown
		open_files[pipe_write] = {-1, std::static_pointer_cast<IFile>(writable_pipe)};
		open_files[pipe_read] = {-1, std::static_pointer_cast<IFile>(readable_pipe)};
		Register_File_Descriptor_To_Process(current_process_pid, pipe_write);
		Register_File_Descriptor_To_Process(current_process_pid, pipe_read);
	}

	// Zapiseme vysledky zpet do pole
	pipe_read_write_pair[0] = pipe_write;
	pipe_read_write_pair[1] = pipe_read;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::Syscall_Close_Handle(const kiv_hal::TRegisters& regs) {
	const auto file_descriptor = static_cast<kiv_os::THandle>(regs.rdx.x);

	// Ziskame aktualni proces
	const auto current_process = ProcessManager::Get().Get_Current_Process();
	if (current_process == nullptr) { // toto se stane pri shutdownu
		return kiv_os::NOS_Error::Success;
	}
	auto lock = std::scoped_lock(mutex);

	// Pokud proces nema pristup k handlu vratime chybu
	if (!Is_File_Descriptor_Accessible(current_process->Get_Pid(), file_descriptor)) {
		return kiv_os::NOS_Error::Permission_Denied;
	}

	// Pokud file neni vratime file not found - pravdepodobne se uz zavrel nekdy jindy
	if (open_files.count(file_descriptor) == 0) {
		return kiv_os::NOS_Error::File_Not_Found;
	}
	Decrement_File_Descriptor_Count(file_descriptor);

	return kiv_os::NOS_Error::Success;
}

void IOManager::Init_Filesystems() {
	//TODO procs asi
	for (int i = 0; i < 256; ++i) {
		const auto disk_num = static_cast<uint8_t>(i);
		kiv_hal::TRegisters registers{};
		kiv_hal::TDrive_Parameters parameters{};
		registers.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);
		registers.rdi.r = reinterpret_cast<uint64_t>(&parameters);
		registers.rdx.l = disk_num;
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers);
		if (!registers.flags.carry) {
			// je disk
			file_systems["C"] = std::make_unique<Fat12>();
			break;
		}
	}
	Log_Debug("(Init Filesystems) FAT12 file system loaded");

	// Vytvoreni procfs
	file_systems["P"] = std::make_unique<ProcFS>();
	Log_Debug("(Init Filesystems) Procfs file system loaded");
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool IOManager::Is_File_Descriptor_Accessible(const kiv_os::THandle pid, const kiv_os::THandle file_descriptor) {
	return open_files.count(file_descriptor) != 0 &&
		process_to_file_mapping.count(pid) != 0 && process_to_file_mapping[pid].count(file_descriptor) != 0;
}

kiv_os::NOS_Error IOManager::Increment_File_Descriptor_Count(const kiv_os::THandle file_descriptor) {
	if (open_files.count(file_descriptor) == 0) {
		return kiv_os::NOS_Error::File_Not_Found;
	}

	auto [count, file] = open_files[file_descriptor];
	open_files[file_descriptor] = {count + 1, file}; // zvysime pocet o 1
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error
IOManager::Register_File_Descriptor_To_Process(const kiv_os::THandle pid, const kiv_os::THandle file_descriptor) {

	// Nejprve chceme zjistit jestli dava operace vubec smysl - file uz musi byt otevreny
	// Pokud increment file descriptor selze nebudeme nic delat a vratime chybu
	if (const auto op_result = Increment_File_Descriptor_Count(file_descriptor);
		op_result != kiv_os::NOS_Error::Success) {
		return op_result;
	}

	// Pokud neni pid v mape, musime vytvorit novy set, do ktereho rovnou umistime file descriptor
	if (process_to_file_mapping.count(pid) == 0) {
		// NOLINT(bugprone-branch-clone)
		process_to_file_mapping[pid] = {file_descriptor};
	}
	else {
		// Jinak pouze emplace
		process_to_file_mapping[pid].emplace(file_descriptor);
	}

	// Jinak success
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::Register_Process_Stdio(const kiv_os::THandle pid, const kiv_os::THandle std_in,
                                                    const kiv_os::THandle std_out) {
	auto lock = std::scoped_lock(mutex);
	if (const auto std_in_register_result = Register_File_Descriptor_To_Process(pid, std_in);
		std_in_register_result != kiv_os::NOS_Error::Success) {
		return std_in_register_result;
	}

	if (const auto std_out_register_result = Register_File_Descriptor_To_Process(pid, std_out);
		std_out_register_result != kiv_os::NOS_Error::Success) {
		return std_out_register_result;
	}

	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::Unregister_Process_Stdio(const kiv_os::THandle pid, const kiv_os::THandle std_in,
                                                      const kiv_os::THandle std_out) {
	auto lock = std::scoped_lock(mutex);
	if (const auto std_in_register_result = Unregister_File_From_Process(pid, std_in);
		std_in_register_result != kiv_os::NOS_Error::Success) {
		return std_in_register_result;
	}

	if (const auto std_out_register_result = Unregister_File_From_Process(pid, std_out);
		std_out_register_result != kiv_os::NOS_Error::Success) {
		return std_out_register_result;
	}

	return kiv_os::NOS_Error::Success;
}

void IOManager::Close_Process_File_Descriptors(const kiv_os::THandle pid) {
	auto lock = std::scoped_lock(mutex);
	if (process_to_file_mapping.count(pid) == 0) {
		return;
	}

	// Snizime pocet file descriptoru pro soubor
	for (const auto file_descriptor : process_to_file_mapping[pid]) {
		Decrement_File_Descriptor_Count(file_descriptor);
	}

	// Smazeme proces z mappingu
	process_to_file_mapping.erase(pid);
}

kiv_os::NOS_Error IOManager::Decrement_File_Descriptor_Count(const kiv_os::THandle file_descriptor) {
	if (open_files.count(file_descriptor) == 0) {
		return kiv_os::NOS_Error::File_Not_Found;
	}

	auto [count, file] = open_files[file_descriptor];
	count -= 1;

	// Pokud je count <= 0 zavreme soubor, smazeme ho a odstranime file handle
	if (count <= 0) {
		// NOLINT(bugprone-branch-clone)
		open_files.erase(file_descriptor);
		file->Close();
		HandleService::Get().Remove_Handle(file_descriptor);
	}
	else {
		open_files[file_descriptor] = {count, file};
	}

	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::Unregister_File_From_Process(const kiv_os::THandle pid,
                                                          const kiv_os::THandle file_descriptor) {

	// Pokud file neexistuje vyhodime chybu
	if (process_to_file_mapping[pid].count(file_descriptor) == 0) {
		return kiv_os::NOS_Error::Permission_Denied; // nemuzeme zavrit soubor, ktery nevlastnime
	}

	// Smazeme file descriptor
	process_to_file_mapping[pid].erase(file_descriptor);

	// Snizime pocet file descriptoru o 1
	if (const auto op_result = Decrement_File_Descriptor_Count(file_descriptor);
		op_result != kiv_os::NOS_Error::Success) {
		return op_result;
	}

	return kiv_os::NOS_Error::Success;
}

/**
 * Provede operaci seek
 * @param regs registry
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error IOManager::Syscall_Seek(kiv_hal::TRegisters& regs) {
	std::shared_ptr<IFile> file;
	{
		const auto file_handle = regs.rdx.x;
		auto lock = std::scoped_lock(mutex);
		if (open_files.count(file_handle) == 0) {
			return kiv_os::NOS_Error::File_Not_Found;
		}
		file = open_files.at(file_handle).second; // file pointer je druhy prvek v dvojici
	}
	kiv_os::THandle handle = regs.rdx.x; // handle
	const auto new_pos = static_cast<size_t>(regs.rdi.r); // nova pozice
	const auto pos_type = static_cast<kiv_os::NFile_Seek>(regs.rcx.l); // typ pozice
	const auto operation = static_cast<kiv_os::NFile_Seek>(regs.rcx.h); // operace - (get/set position / set size)
	size_t file_pos;
	if (operation == kiv_os::NFile_Seek::Set_Size) {
		auto res = file->Seek(new_pos, pos_type, operation, file_pos);
	}
	const auto res = file->Seek(new_pos, pos_type, operation, file_pos);
	if (res == kiv_os::NOS_Error::Success && operation == kiv_os::NFile_Seek::Set_Position) {
		regs.rax.r = file_pos;
	}
	return res;
}

kiv_os::NOS_Error IOManager::Syscall_Get_Working_Dir(kiv_hal::TRegisters& regs) {
	const std::shared_ptr<Process> process = ProcessManager::Get().Get_Current_Process();
	auto lock = std::scoped_lock(mutex);
	const auto buffer = reinterpret_cast<char*>(regs.rdx.r);
	const auto buffer_size = static_cast<size_t>(regs.rcx.r);
	//TODO najit proces vlakna, kopirovat buffer do working dir, nastavit written
	strcpy_s(buffer, buffer_size, process->Get_Working_Dir().To_String().c_str());
	const size_t written = min(process->Get_Working_Dir().To_String().length(), buffer_size);
	regs.rax.r = written;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::Syscall_Set_Working_Dir(const kiv_hal::TRegisters& regs) {
	auto lock = std::scoped_lock(mutex);
	const auto path_char = reinterpret_cast<char*>(regs.rdx.r);
	if (path_char == nullptr) {
		return kiv_os::NOS_Error::IO_Error;
	}
	const Path path(path_char);
	if (path.is_relative) {
		// TODO ziskat proces a jeho cestu a append pred nasi cestu
		//Path process_path = process->GetWorkingDir();
	}
	const auto fs = Get_File_System(path.disk_letter);
	if (fs == nullptr) {
		return kiv_os::NOS_Error::Unknown_Filesystem;
	}
	if (fs->Check_If_File_Exists(path)) {
		//process->SetWorkingDir(path);
		return kiv_os::NOS_Error::Success;
	}

	return kiv_os::NOS_Error::File_Not_Found;
}

kiv_os::NOS_Error IOManager::Syscall_Open_File(kiv_hal::TRegisters& regs) {
	char* file_name = reinterpret_cast<char*>(regs.rdx.r);
	const auto flags = static_cast<kiv_os::NOpen_File>(regs.rcx.l); // TODO check
	const auto attributes = static_cast<uint8_t>(regs.rdi.i);
	kiv_os::THandle handle; //TODO vytvorit handle
	std::shared_ptr<Process> current_process = ProcessManager::Get().Get_Current_Process();

	if (current_process == nullptr) {
		return kiv_os::NOS_Error::File_Not_Found;
	}

	Path path(file_name);
	if (path.is_relative) {

		Path working_dir = current_process->Get_Working_Dir();
		working_dir.Append_Path(path);
		path = working_dir;
	}

	const auto res = Open_File(path, flags, attributes, handle, current_process->Get_Pid());

	if (res == kiv_os::NOS_Error::Success) {
		regs.rax.x = handle;
	}

	return res;
}


auto IOManager::Open_Procfs_File(VFS* fs, Path& path, const kiv_os::NOpen_File flags, uint8_t attributes,
                                 kiv_os::THandle& handle,
                                 const kiv_os::THandle current_pid) -> kiv_os::NOS_Error {

	auto err = kiv_os::NOS_Error::Success;
	auto file = fs->Open_IFile(path, flags, attributes, err);
	if (err != kiv_os::NOS_Error::Success) {
		return err;
	}

	auto lock = std::scoped_lock(mutex);
	const auto file_descriptor = HandleService::Get().Get_Empty_Handle();

	// Do otevrenych souboru ulozime handle s referenci na soubor
	open_files[file_descriptor] = {1, file};

	// Soubor pridame k procesu aby k nemu mel pristup
	Register_File_Descriptor_To_Process(current_pid, file_descriptor);
	handle = file_descriptor;
	return err;
}

//TODO jestli prepsat soubor, kdyz je 0 fmalways a existuje
kiv_os::NOS_Error IOManager::Open_File(Path path, const kiv_os::NOpen_File flags, uint8_t attributes,
                                       kiv_os::THandle& handle, const kiv_os::THandle current_pid) {


	//TODO mozna vytvoreni ruznych typu filu
	const auto fs = Get_File_System(path.disk_letter);
	if (fs == nullptr) {
		return kiv_os::NOS_Error::Unknown_Filesystem;
	}

	if (fs->file_system_type == FsType::Procfs) {
		return Open_Procfs_File(fs, path, flags, attributes, handle, current_pid);
	}

	auto lock = std::scoped_lock(mutex);
	const bool file_exists = fs->Check_If_File_Exists(path);

	if (flags == kiv_os::NOpen_File::fmOpen_Always) {
		if (!file_exists) {
			// neexistuje, ale mel by
			handle = kiv_os::Invalid_Handle;
			return kiv_os::NOS_Error::Invalid_Argument;
		}
	}

	path.Delete_Name_From_Path(); // nemusi existovat, kontroluje se i bez jmena
	const bool parent_exists = fs->Check_If_File_Exists(path);

	if (!parent_exists) {
		handle = kiv_os::Invalid_Handle;
		return kiv_os::NOS_Error::File_Not_Found;
	}

	path.Return_Name_to_Path();

	// existuje a mel by byt vytvoren adresar - chyba
	if (file_exists && (attributes & static_cast<decltype(attributes)>(kiv_os::NFile_Attributes::Directory))) {
		handle = kiv_os::Invalid_Handle;
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	File f{};
	const auto res = fs->Open(path, flags, f, attributes);

	if (res != kiv_os::NOS_Error::Success) {
		handle = kiv_os::Invalid_Handle;
		return res;
	}

	auto file = std::make_shared<Fs_File>(fs, f);

	handle = HandleService::Get().Get_Empty_Handle();

	// Do otevrenych souboru ulozime handle s referenci na soubor
	open_files[handle] = {1, file};

	// Soubor pridame k procesu aby k nemu mel pristup
	Register_File_Descriptor_To_Process(current_pid, handle);


	return kiv_os::NOS_Error::Success;
}


kiv_os::NOS_Error IOManager::Syscall_Get_File_Attribute(kiv_hal::TRegisters& regs) {
	char* file_name = reinterpret_cast<char*>(regs.rdx.r);

	Path path(file_name);
	if (path.is_relative) {
		std::shared_ptr<Process> process = ProcessManager::Get().Get_Current_Process();
		Path working_dir = process->Get_Working_Dir();
		working_dir.Append_Path(path);
		path = working_dir;
	}
	const auto fs = Get_File_System(path.disk_letter);
	if (fs == nullptr) {
		return kiv_os::NOS_Error::Unknown_Filesystem;
	}
	if (fs->Check_If_File_Exists(path)) {
		uint8_t attributes;
		const auto res = fs->Get_Attributes(path, attributes);
		if (res == kiv_os::NOS_Error::Success) {
			regs.rdi.i = attributes;
		}
		return res;
	}
	return kiv_os::NOS_Error::File_Not_Found;
}

kiv_os::NOS_Error IOManager::Syscall_Set_File_Attribute(const kiv_hal::TRegisters& regs) {
	char* file_name = reinterpret_cast<char*>(regs.rdx.r);
	const auto attributes = static_cast<uint8_t>(regs.rdi.i);

	Path path(file_name);
	if (path.is_relative) {
		std::shared_ptr<Process> process = ProcessManager::Get().Get_Current_Process();
		Path working_dir = process->Get_Working_Dir();
		working_dir.Append_Path(path);
		path = working_dir;
	}
	auto fs = Get_File_System(path.disk_letter);
	if (fs == nullptr) {
		return kiv_os::NOS_Error::Unknown_Filesystem;
	}
	if (fs->Check_If_File_Exists(path)) {
		auto res = fs->Set_Attributes(path, attributes);
		return res;
	}
	return kiv_os::NOS_Error::File_Not_Found;
}


kiv_os::NOS_Error IOManager::Syscall_Delete_File(const kiv_hal::TRegisters& regs) {
	char* file_name = reinterpret_cast<char*>(regs.rdx.r);

	Path path(file_name);
	if (path.is_relative) {
		std::shared_ptr<Process> process = ProcessManager::Get().Get_Current_Process();
		Path working_dir = process->Get_Working_Dir();
		working_dir.Append_Path(path);
		path = working_dir;
	}
	auto fs = Get_File_System(path.disk_letter);
	if (fs == nullptr) {
		return kiv_os::NOS_Error::Unknown_Filesystem;
	}
	if (fs->Check_If_File_Exists(path)) {
		auto res = fs->Rm_Dir(path);
		return res;
	}
	return kiv_os::NOS_Error::File_Not_Found;
}

VFS* IOManager::Get_File_System(const std::string& disk) {
	const auto res = file_systems.find(disk);
	if (res != file_systems.end()) {
		return res->second.get();
	}

	return nullptr;
}
