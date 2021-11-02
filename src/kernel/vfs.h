//
// Created by Kuba on 09.10.2021.
//

#pragma once

#include "../api/api.h"
#include "path.h"
#include "vector"

struct File {
    char *name;
    size_t size;
    size_t offset;
    uint16_t attributes;
    kiv_os::THandle handle;
};

class VFS {
public:
    virtual kiv_os::NOS_Error Open(Path &path, kiv_os::NOpen_File flags, File &file, uint8_t attributes) = 0;

    virtual kiv_os::NOS_Error Close(File file) = 0;

    virtual kiv_os::NOS_Error Read_Dir(Path &path, std::vector<kiv_os::TDir_Entry> &entries) = 0;

    virtual kiv_os::NOS_Error Mk_Dir(Path &path, uint8_t attributes) = 0;

    virtual kiv_os::NOS_Error Rm_Dir(Path &path) = 0;

    virtual kiv_os::NOS_Error Create_File(Path &path, uint8_t attributes) = 0;

    virtual kiv_os::NOS_Error Read(File file, size_t size, size_t offset, std::vector<char> &out) = 0;

    virtual kiv_os::NOS_Error Write(File file, size_t offset, std::vector<char> buffer, size_t &written) = 0;

    virtual kiv_os::NOS_Error Set_Attributes(Path path, uint8_t attributes) = 0;

    virtual kiv_os::NOS_Error Get_Attributes(Path path, uint8_t &attributes) = 0;

    virtual bool File_Exists(Path path, int32_t current_fd, int32_t &target_fd, bool root) = 0;

    virtual uint32_t Get_Root_Fd() = 0;


    virtual ~VFS() = default;

};