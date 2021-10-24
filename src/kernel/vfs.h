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

    virtual kiv_os::NOS_Error ReadDir(const Path &path, std::vector<kiv_os::TDir_Entry> &entries) = 0;

    virtual kiv_os::NOS_Error MkDir(Path &path, uint8_t attributes) = 0;

    virtual kiv_os::NOS_Error RmDir(Path &path) = 0;

    virtual kiv_os::NOS_Error CreateFile(Path &path, uint8_t attributes) = 0;

    virtual kiv_os::NOS_Error Read(File file, size_t size, size_t offset, std::vector<char> &out) = 0;

    virtual kiv_os::NOS_Error Write(File file, size_t size, size_t offset, std::vector<char> buffer, size_t &written) = 0;

};