#include "IFile.h"

#include "Utils/Logging.h"


IFile::~IFile() = default;

kiv_os::NOS_Error IFile::Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) {
	LogWarning("IFile: accessing unimplemented method read");
	return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error IFile::Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) {
	LogWarning("IFile: accessing unimplemented method write");
	return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error IFile::Close() {
	LogWarning("IFile: accessing unimplemented method close");
	return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error IFile::Seek(size_t position, kiv_os::NFile_Seek seek_type, kiv_os::NFile_Seek seek_operation, size_t& res_pos) {
	LogWarning("IFile: accessing unimplemented method seek");
	return kiv_os::NOS_Error::IO_Error;
}
