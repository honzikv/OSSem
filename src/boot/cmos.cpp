#include "cmos.h"

#include <fstream>

CCMOS cmos{};

extern "C" char __ImageBase;


std::filesystem::path Get_Application_Dir() {
	const size_t bufsize = 1024;
	wchar_t ModuleFileName[bufsize];
	

	GetModuleFileNameW(((HINSTANCE)&__ImageBase), ModuleFileName, bufsize);

	std::filesystem::path exePath{ ModuleFileName };
	return exePath.parent_path();
}

CCMOS::CCMOS() noexcept {
	auto file_path = Get_Application_Dir() / rsConfig_Name;

	std::vector<char> buf;
	std::ifstream config_file;

	try {
		config_file.open(file_path);

		if (config_file.is_open()) {
			buf.assign(std::istreambuf_iterator<char>(config_file), std::istreambuf_iterator<char>());
			mIni.LoadData(buf.data(), buf.size());
		}	
	}
	catch (...) {
	}
}

TCMOS_Drive_Parameters CCMOS::Drive_Parameters(uint8_t drive_index) {
	const wchar_t dec_2_hex[16] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

	std::wstring section_full_name{ isDrive_Prefix };
	section_full_name += dec_2_hex[drive_index >> 4];
	section_full_name += dec_2_hex[drive_index & 0xf];
			

	TCMOS_Drive_Parameters result;
	
	result.is_present = mIni.GetSection(section_full_name.c_str());

	if (result.is_present) {
		result.is_ram_disk = mIni.GetBoolValue(section_full_name.c_str(), iiRAM_Disk);
		result.read_only = mIni.GetBoolValue(section_full_name.c_str(), iiRead_Only);
		result.disk_image = mIni.GetValue(section_full_name.c_str(), iiDisk_Image, L"");
		result.RAM_Disk_Size = mIni.GetLongValue(section_full_name.c_str(), iiRAM_Disk_Size, result.bytes_per_sector);			//alespon jeden sektor
	}

	return result;
}