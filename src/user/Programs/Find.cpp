#include "Find.h"

#include <string>
#include <vector>
#include <locale>
#include "../../api/hal.h"
#include "../rtl.h"
#include "../Utils/StringUtils.h"

extern "C" size_t __stdcall find(const kiv_hal::TRegisters & regs) {
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    auto args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	size_t read = 1;
    kiv_os::THandle file_handle;

    constexpr size_t buffer_size = 256;
    char buffer[buffer_size] = {};
    
    bool is_handler_closable = true;
    std::vector<std::string> lines;

    int lines_count = 0;

    // parsing dat
    constexpr int start_position = 0;
    const std::string format = "/c /v\"\"";

    if (const int index = static_cast<int>(args.find(format, start_position)); index != 0) {
        size_t written;
        const auto err_message = std::string("Wrong arguments.. Run command with /v "" /c [optional path]\n");
        kiv_os_rtl::Write_File(std_out, err_message.data(), err_message.size(), written);
        return 0;
    }
    // syntax OK
    args = args.substr(format.size());
    args = StringUtils::Trim_Whitespaces(args);

    if (!args.empty()) {
        if (!kiv_os_rtl::Open_File(file_handle, args, kiv_os::NOpen_File::fmOpen_Always, static_cast<kiv_os::NFile_Attributes>(0))) {
        	auto err_message = std::string("Can not open file ");
            err_message.append(args);
            err_message.append(".\n");

            size_t written = 0;
            kiv_os_rtl::Write_File(std_out, err_message.c_str(), err_message.size(), written);
            kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found));
            return 1;
        }
    }
    else {
        // bude se cist ze std_inu
        file_handle = std_in;
        is_handler_closable = false;
    }
    
    while (read) {
        if (!kiv_os_rtl::Read_File(file_handle, buffer, buffer_size, read)) {
            // neni co cist
        	break;	
        }
        for (int i = 0; i < read; i++) {
            if (static_cast<kiv_hal::NControl_Codes>(buffer[i]) == kiv_hal::NControl_Codes::EOT) {
                read = 0;
                break;
            }

            if (buffer[i] == '\n') {
                lines_count++;
            }
        }
    }

    size_t written = 0;
    std::string message = std::to_string(lines_count);
    message.append("\n");
    kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);

    if (is_handler_closable) {
        kiv_os_rtl::Close_File_Descriptor(file_handle);
    }

    return 0;
}