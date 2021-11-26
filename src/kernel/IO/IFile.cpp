#include "IFile.h"

IFile::~IFile() = default;

kiv_os::NOS_Error IFile::Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error IFile::Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error IFile::Close() {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error IFile::Seek(size_t position, kiv_os::NFile_Seek seek_type, kiv_os::NFile_Seek seek_operation,
                              size_t& res_size) {
	return kiv_os::NOS_Error::Unknown_Error;
}
