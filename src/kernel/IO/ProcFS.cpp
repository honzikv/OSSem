#include "ProcFS.h"

kiv_os::NOS_Error ProcFS::Open(Path& path, kiv_os::NOpen_File flags, File& file, uint8_t attributes) {
	return VFS::Open(path, flags, file, attributes);
}
