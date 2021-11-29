#include "Find.h"

#include <string>
#include <vector>
#include <locale>
#include "../api/hal.h"
#include "rtl.h"
#include "Utils/StringUtils.h"

extern "C" size_t __stdcall find(const kiv_hal::TRegisters & regs) {
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    auto args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	size_t read;
    kiv_os::THandle file_handle;

    constexpr size_t buffer_size = 256;
    char buffer[buffer_size] = {};
    
    bool is_handler_closable = true;
    std::vector<std::string> lines;
    std::string curr_string;

    int lines_count = 0;

    // parsing dat
    constexpr int start_position = 0;
    const std::string format = "/v \"\" /c";

    if (const int index = static_cast<int>(args.find(format, start_position)); index != 0) {
        size_t written;
        const auto err_message = std::string("Wrong arguments.. Run command with /v "" /c [optional path]\n");
        kiv_os_rtl::Write_File(std_out, err_message.data(), err_message.size(), written);
        return 0;
    }
    // syntax OK
    args = args.substr(format.size(), args.size() - 1);
    args = StringUtils::Trim_Whitespaces(args);

    if (args.c_str() && !args.empty()) {
        if (kiv_os_rtl::Open_File(file_handle, args, kiv_os::NOpen_File::fmOpen_Always, static_cast<kiv_os::NFile_Attributes>(0))) {
        	auto err_message = std::string("Can not open file ");
            err_message.append(args);
            err_message.append(".\n");

            size_t written = 0;
            kiv_os_rtl::Write_File(std_out, err_message.c_str(), err_message.size(), written);
            return 1;
        }
    }
    else {
        // bude se cist ze std_inu
        file_handle = std_in;
        is_handler_closable = false;
    }
    
    do {
        if (kiv_os_rtl::Read_File(file_handle, buffer, buffer_size, read)) {
            for (int i = 0; i < read; i++) {
                if (static_cast<kiv_hal::NControl_Codes>(buffer[i]) == kiv_hal::NControl_Codes::EOT) {
                    read = 0;
                    break; // EOT
                }

                if (buffer[i] == '\n') {
                    lines_count++;
                }
            }
        }
    } while (read);


    memset(buffer, 0, buffer_size);
    size_t written = 0;
    const auto size = sprintf_s(buffer, "%d", lines_count);
    kiv_os_rtl::Write_File(std_out, buffer, size, written);
    kiv_os_rtl::Write_File(std_out, "\n", 1, written);

    if (is_handler_closable) {
        kiv_os_rtl::Close_File_Descriptor(file_handle);
    }

    return 0;
}