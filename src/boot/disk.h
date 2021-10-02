#pragma once

#include "cmos.h"
#include "../api/hal.h"

#include <fstream>
#include <vector>

class CDisk_Drive {
protected:
	size_t mBytes_Per_Sector;
	size_t mDisk_Size;	
	bool Check_DAP(kiv_hal::TRegisters &context);	
		//vrati true, pokud by cteni/zapis nezpusobilo pristup za velikost disku
		//v takovem pripade vraci false a nastavi chybu
public:
	CDisk_Drive(const TCMOS_Drive_Parameters &cmos_parameters);
	void Drive_Parameters(kiv_hal::TRegisters &context);

	virtual void Read_Sectors(kiv_hal::TRegisters &context) = 0;
	virtual void Write_Sectors(kiv_hal::TRegisters &context) = 0;	

	static void Set_Status(kiv_hal::TRegisters& context, const kiv_hal::NDisk_Status status);	
};

class CDisk_Image : public CDisk_Drive {
protected:
	std::fstream mDisk_Image;
public:
	CDisk_Image(const TCMOS_Drive_Parameters &cmos_parameters);
	
	virtual void Read_Sectors(kiv_hal::TRegisters &context )final;
	virtual void Write_Sectors(kiv_hal::TRegisters &context) final;
};


class CRAM_Disk : public CDisk_Drive {
protected:
	std::vector<char> mDisk_Image;
public:
	CRAM_Disk(const TCMOS_Drive_Parameters &cmos_parameters);

	virtual void Read_Sectors(kiv_hal::TRegisters &context) final;
	virtual void Write_Sectors(kiv_hal::TRegisters &context) final;
};


void __stdcall Disk_Handler(kiv_hal::TRegisters &context);