//
// Created by Kuba on 18.11.2021.
//

#pragma once


#include "../vfs.h"
#include "IFile.h"

class Fs_File final : public IFile {
public:
    explicit Fs_File(VFS *vfs, File file);

    kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) override;

    kiv_os::NOS_Error Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) override;

    kiv_os::NOS_Error Close() override;

    kiv_os::NOS_Error Seek(size_t position, kiv_os::NFile_Seek seek_type, kiv_os::NFile_Seek seek_operation, size_t &res_pos) override;

    ~Fs_File() override;

private:
    /**
     * VFS
     */
    VFS *fs;

    /**
     * Struktura file
     */
    File file;

    bool Is_Read_Only();

    bool Is_Dir();

    bool Has_Attr(kiv_os::NFile_Attributes attr);
};

