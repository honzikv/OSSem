#pragma once
#include "vfs.h"

/// <summary>
/// ProcFS je konstruovany tak, ze root adresar obsahuje slozku pro kazdy proces
///	Kazda slozka obsahuje jako soubory atributy procesu
/// </summary>
class ProcFS final : public VFS {

public:
	kiv_os::NOS_Error Get_All_Processes(std::vector<kiv_os::TDir_Entry>& vector) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error Open(Path& path, kiv_os::NOpen_File flags, File& file, uint8_t attributes) override;

	/// <summary>
	/// Precte adresar - bud pro 
	/// </summary>
	/// <param name="path"></param>
	/// <param name="entries"></param>
	/// <returns></returns>
	kiv_os::NOS_Error Read_Dir(Path& path, std::vector<kiv_os::TDir_Entry>& entries) override {
		const auto uri = path.path_vector;

		// Pro procfs nam muze prijit jeden ze 3 stavu:
		// 1. - /procfs - chceme vratit vsechny procesy
		// 2. - /procfs/pid - chceme vratit vsechny fieldy pro pid - tato funkce je staticka a vraci vzdy to same
		switch (uri.size()) {
			case 1: {
				return Get_All_Processes(entries);
			}
			case 2: {
				// return Get_Process_Fields(path, entries);
			}
			default: {
				return kiv_os::NOS_Error::File_Not_Found;
			}
		}
	}
};
