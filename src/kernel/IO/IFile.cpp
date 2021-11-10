#include "IFile.h"


kiv_os::NOS_Error IFile::Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error IFile::Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) {
	return kiv_os::NOS_Error::Unknown_Error;
}

void IFile::CloseWriting() {}
void IFile::CloseReading() {}

void IFile::Close() {
	CloseWriting();
	CloseWriting();
}
