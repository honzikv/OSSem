#include "AbstractFile.h"


kiv_os::NOS_Error AbstractFile::read(char* targetBuffer, size_t bytes, size_t& bytesRead) {
	return kiv_os::NOS_Error::Unknown_Error;
}

kiv_os::NOS_Error AbstractFile::write(const char* sourceBuffer, size_t bytes, size_t& bytesWritten) {
	return kiv_os::NOS_Error::Unknown_Error;
}
