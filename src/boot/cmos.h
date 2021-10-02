#pragma once

#include "../api/hal.h"
#include "SimpleIni.h"

#include <filesystem>

struct TCMOS_Drive_Parameters {
	bool is_present = false;
	bool is_ram_disk = true;												//bud vytvorime neformatovany RAM disk
	bool read_only = true;
	std::filesystem::path disk_image = "";									//anebo pouzijeme soubor z disku
	const static size_t bytes_per_sector = 512;
	size_t RAM_Disk_Size = 0;
};

class CCMOS {
protected:
	CSimpleIniW mIni;
protected:
	const wchar_t* rsConfig_Name = L"boot.ini";
	const wchar_t* isDrive_Prefix = L"Drive_0x";
	const wchar_t* iiRAM_Disk = L"RAM_Disk";
	const wchar_t* iiRead_Only = L"Ready_Only";
	const wchar_t* iiDisk_Image = L"Disk_Image";
	const wchar_t* iiRAM_Disk_Size = L"RAM_Disk_Size";
public:
	CCMOS() noexcept;

	TCMOS_Drive_Parameters Drive_Parameters(uint8_t drive_index);
};

extern CCMOS cmos;