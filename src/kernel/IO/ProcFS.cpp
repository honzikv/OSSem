#include "ProcFS.h"
ProcFS::ProcFS(): VFS(FsType::Procfs) {}

std::shared_ptr<IFile> ProcFS::Open_IFile(Path& path, kiv_os::NOpen_File flags, uint8_t attributes,
	kiv_os::NOS_Error& err) {
	// Zde jde otevrit pouze soubor procfs
	if (path.is_relative || path.path_vector.size() != 1) {
		err = kiv_os::NOS_Error::File_Not_Found;
		return {};
	}

	const auto file = path.path_vector[0];
	if (file != "PROCLST") {
		err = kiv_os::NOS_Error::File_Not_Found;
		return {};
	}

	auto& process_manager = ProcessManager::Get();

	// Ziskame snapshot
	const auto snapshot = process_manager.Get_Process_Table_Snapshot();
	return std::static_pointer_cast<IFile>(snapshot); // vratime snapshot
}
