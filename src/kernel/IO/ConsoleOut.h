#pragma once
#include <mutex>

#include "IFile.h"

/// <summary>
/// Zapis do konzole
/// </summary>
class ConsoleOut final : public IFile {


public:
	/// <summary>
	/// Zapis do konzole
	/// </summary>
	/// <param name="source_buffer"></param>
	/// <param name="bytes"></param>
	/// <param name="bytes_written"></param>
	/// <returns></returns>
	kiv_os::NOS_Error Write(const char* source_buffer, size_t bytes, size_t& bytes_written) override;

	/// <summary>
	/// Close konzole moc smysl nedava, takze se pouze vrati success
	/// </summary>
	/// <returns></returns>
	kiv_os::NOS_Error Close() override {
		return kiv_os::NOS_Error::Success;
	}
};
