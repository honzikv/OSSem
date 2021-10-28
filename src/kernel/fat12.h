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

public:

    Fat12();

    //TODO doimplementovat
    kiv_os::NOS_Error Open(Path &path, kiv_os::NOpen_File flags, File &file, uint8_t attributes) override;

    kiv_os::NOS_Error Close(File file) override;

    kiv_os::NOS_Error ReadDir(Path &path, std::vector<kiv_os::TDir_Entry> &entries) override;

    kiv_os::NOS_Error MkDir(Path &path, uint8_t attributes) override;

    kiv_os::NOS_Error RmDir(Path &path) override;

    kiv_os::NOS_Error CreateFile(Path &path, uint8_t attributes) override;

    kiv_os::NOS_Error Read(File file, size_t bytes_to_read, size_t offset, std::vector<char> &buffer) override;

    kiv_os::NOS_Error Write(File file, size_t offset, std::vector<char> buffer, size_t &written) override;

    kiv_os::NOS_Error SetAttributes(Path path, uint8_t attributes) override;

    kiv_os::NOS_Error GetAttributes(Path path, uint8_t &attributes) override;
};


#endif //OS_FAT12_H
