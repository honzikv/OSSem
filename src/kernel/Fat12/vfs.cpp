#include "vfs.h"

VFS::VFS(const FsType file_system_type): file_system_type(file_system_type) {

}

kiv_os::NOS_Error VFS::Open(Path& path, kiv_os::NOpen_File flags, File& file, uint8_t attributes) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Close(File file) {
	return kiv_os::NOS_Error::Unknown_Error;
}

std::shared_ptr<IFile> VFS::Open_IFile(Path& path, kiv_os::NOpen_File flags, uint8_t attributes,
	kiv_os::NOS_Error& err) {
	err = kiv_os::NOS_Error::Unknown_Error;
	return {};
}

kiv_os::NOS_Error VFS::Read_Dir(Path& path, std::vector<kiv_os::TDir_Entry>& entries) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Mk_Dir(Path& path, uint8_t attributes) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Rm_Dir(Path& path) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Create_File(Path& path, uint8_t attributes) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Read(File file, size_t size, size_t offset, std::vector<char>& out) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Write(File &file, size_t offset, std::vector<char> buffer, size_t& written) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Set_Attributes(Path path, uint8_t attributes) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error VFS::Get_Attributes(Path path, uint8_t& attributes) {
	return kiv_os::NOS_Error::Unknown_Error;
}

bool VFS::Check_If_File_Exists(Path path) { return false; }
uint32_t VFS::Get_Root_Fd() {
	return -1;
}

kiv_os::NOS_Error VFS::Set_Size(File file, size_t new_size) {
	return kiv_os::NOS_Error::Unknown_Error;
}
