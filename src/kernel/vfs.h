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
    virtual kiv_os::NOS_Error open(const Path &path, const char*name, kiv_os::NOpen_File flags, File &file);

    virtual kiv_os::NOS_Error close(File file);

    virtual kiv_os::NOS_Error readDir(const Path &path, std::vector<kiv_os::TDir_Entry> &entries);

    virtual kiv_os::NOS_Error mkDir(const Path &path, uint16_t attributes);

    virtual kiv_os::NOS_Error rmDir(const Path &path);

    virtual kiv_os::NOS_Error read(File file, size_t size, size_t offset, std::vector<char> &out);

    virtual kiv_os::NOS_Error write(File file, size_t size, size_t offset, std::vector<char> buffer, size_t &written);

};