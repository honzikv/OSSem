#pragma once
#include "IFile.h"
#include "Pipe.h"
#include <memory>

/// <summary>
/// Pipe pro zapis - po zavreni se zavre zapis
/// </summary>
class WritablePipe final : public IFile {

	const std::shared_ptr<Pipe> pipe;

public:

	explicit WritablePipe(std::shared_ptr<Pipe> pipe);

	kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) override;

	kiv_os::NOS_Error Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) override;

	kiv_os::NOS_Error Close() override;

	kiv_os::NOS_Error Seek(size_t position, kiv_os::NFile_Seek seek_type) override;
};
