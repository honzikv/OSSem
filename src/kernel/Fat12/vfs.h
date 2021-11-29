//
// Created by Kuba on 09.10.2021.
//

#pragma once

#include <memory>

#include "../../api/api.h"
#include "path.h"
#include "vector"
#include "../IO/IFile.h"

struct File {
	char* name;
	size_t size;
	size_t offset;
	uint16_t attributes;
	kiv_os::THandle handle;
};

enum class FsType : uint8_t {
	Fat12,
	Procfs
};

class VFS {

public:

	/// <summary>
	/// Typ filesystemu
	/// </summary>
	FsType file_system_type;

	/// <summary>
	/// Konstruktor - pri vytvoreni potrebujeme definovat typ filesystemu
	/// </summary>
	/// <param name="file_system_type">typ filesystemu</param>
	explicit VFS(FsType file_system_type);

	virtual kiv_os::NOS_Error Open(Path& path, kiv_os::NOpen_File flags, File& file, uint8_t attributes);

	virtual kiv_os::NOS_Error Close(File file);

	virtual std::shared_ptr<IFile> Open_IFile(Path& path, kiv_os::NOpen_File flags, uint8_t attributes, kiv_os::NOS_Error& err);

	virtual kiv_os::NOS_Error Read_Dir(Path& path, std::vector<kiv_os::TDir_Entry>& entries);

	virtual kiv_os::NOS_Error Mk_Dir(Path& path, uint8_t attributes);

	virtual kiv_os::NOS_Error Rm_Dir(Path& path);

	virtual kiv_os::NOS_Error Create_File(Path& path, uint8_t attributes);

	virtual kiv_os::NOS_Error Read(File file, size_t size, size_t offset, std::vector<char>& out);

	virtual kiv_os::NOS_Error Write(File file, size_t offset, std::vector<char> buffer, size_t& written);

	virtual kiv_os::NOS_Error Set_Attributes(Path path, uint8_t attributes);

	virtual kiv_os::NOS_Error Get_Attributes(Path path, uint8_t& attributes);

	virtual bool Check_If_File_Exists(Path path);

	virtual uint32_t Get_Root_Fd();

	virtual kiv_os::NOS_Error Set_Size(File file, size_t new_size);

	virtual ~VFS() = default;

};
