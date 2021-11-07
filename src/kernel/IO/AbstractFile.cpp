#include "AbstractFile.h"


kiv_os::NOS_Error AbstractFile::Read(char* target_buffer, size_t bytes, size_t& bytes_read) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error AbstractFile::Write(const char* source_buffer, size_t bytes, size_t& bytes_written) {
	return kiv_os::NOS_Error::Unknown_Error;
}
