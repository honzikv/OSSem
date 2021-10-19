//
// Created by Kuba on 09.10.2021.
//

#ifndef OS_FAT12_H
#define OS_FAT12_H


#include <string>
#include "vfs.h"
#include "../api/hal.h"
#include "fat_helper.h"


class Fat12 : public VFS {

    Fat12();

    //TODO doimplementovat
    kiv_os::NOS_Error open(Path &path, kiv_os::NOpen_File flags, File &file, uint8_t attributes) override;

    kiv_os::NOS_Error close(File file) override;

    kiv_os::NOS_Error readDir(const Path &path, std::vector<kiv_os::TDir_Entry> &entries) override;

    kiv_os::NOS_Error mkDir(Path &path, uint16_t attributes) override;

    kiv_os::NOS_Error rmDir(const Path &path) override;

    kiv_os::NOS_Error read(File file, size_t size, size_t offset, std::vector<char> &out) override;

    kiv_os::NOS_Error
    write(File file, size_t size, size_t offset, std::vector<char> buffer, size_t &written) override;

    static DirItem get_cluster(const int startSector, const Path &path);
};


#endif //OS_FAT12_H
