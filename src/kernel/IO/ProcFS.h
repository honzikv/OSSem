#pragma once
#include "IOManager.h"
#include "vfs.h"
#include "Process/ProcessManager.h"

/// <summary>
/// </summary>
class ProcFS final : public VFS {

public:
	ProcFS();

	std::shared_ptr<IFile>
	Open_IFile(Path& path, kiv_os::NOpen_File flags, uint8_t attributes, kiv_os::NOS_Error& err) override;
};
