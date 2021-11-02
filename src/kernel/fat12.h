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

    kiv_os::NOS_Error Read_Dir(Path &path, std::vector<kiv_os::TDir_Entry> &entries) override;

    kiv_os::NOS_Error Mk_Dir(Path &path, uint8_t attributes) override;

    kiv_os::NOS_Error Rm_Dir(Path &path) override;

    kiv_os::NOS_Error Create_File(Path &path, uint8_t attributes) override;

    kiv_os::NOS_Error Read(File file, size_t bytes_to_read, size_t offset, std::vector<char> &buffer) override;

    kiv_os::NOS_Error Write(File file, size_t offset, std::vector<char> buffer, size_t &written) override;

    kiv_os::NOS_Error Set_Attributes(Path path, uint8_t attributes) override;

    kiv_os::NOS_Error Get_Attributes(Path path, uint8_t &attributes) override;

    bool File_Exists(Path path, int32_t current_fd, int32_t &target_fd, bool root) override;

    uint32_t Get_Root_Fd() override;

};


#endif //OS_FAT12_H
