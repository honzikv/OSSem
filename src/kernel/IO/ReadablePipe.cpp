#include "ReadablePipe.h"

ReadablePipe::ReadablePipe(std::shared_ptr<Pipe> pipe): pipe(std::move(pipe)) {  }

kiv_os::NOS_Error ReadablePipe::Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) {
	return pipe->Read(target_buffer, buffer_size, bytes_read);
}

kiv_os::NOS_Error ReadablePipe::Close() {
	pipe->CloseForReading();
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ReadablePipe::Seek(size_t position, kiv_os::NFile_Seek seek) {
	return kiv_os::NOS_Error::IO_Error;
}
