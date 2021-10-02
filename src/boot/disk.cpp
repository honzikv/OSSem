//Tento kod nebyl odladen. Mozna, ze tu budou chyby k odhaleni... a mozna i tak k ziskani nejakeho toho bonusoveho bodu.

#include "disk.h"

#include <array>
#include <filesystem>

std::array<std::unique_ptr<CDisk_Drive>, 256> disk_drives;

#undef max

void __stdcall Disk_Handler(kiv_hal::TRegisters &context) {
	auto& disk_drive = disk_drives[context.rdx.l];

	bool drive_seems_ready = disk_drive.operator bool();
	if (!drive_seems_ready) {
		auto cmos_params = cmos.Drive_Parameters(context.rdx.l);
		if (cmos_params.is_present) {
			if (cmos_params.is_ram_disk) {
				disk_drive.reset(new CRAM_Disk{ cmos_params });
				drive_seems_ready = true;
			}
			else {
				std::error_code ec;
				drive_seems_ready = std::filesystem::exists(cmos_params.disk_image, ec);
				if (drive_seems_ready) disk_drive.reset(new CDisk_Image{ cmos_params });
			}
		}
	}

	 if (drive_seems_ready) {
		 switch (static_cast<kiv_hal::NDisk_IO>(context.rax.h)) {
			 case kiv_hal::NDisk_IO::Read_Sectors:		return disk_drive->Read_Sectors(context);
			 case kiv_hal::NDisk_IO::Write_Sectors:		return disk_drive->Write_Sectors(context);
			 case kiv_hal::NDisk_IO::Drive_Parameters:	return disk_drive->Drive_Parameters(context);

			 default: CDisk_Drive::Set_Status(context, kiv_hal::NDisk_Status::Bad_Command);
		 }
	 }
	 else {
		 CDisk_Drive::Set_Status(context, kiv_hal::NDisk_Status::Drive_Not_Ready);
	 }
}

CDisk_Drive::CDisk_Drive(const TCMOS_Drive_Parameters &cmos_parameters) : mBytes_Per_Sector(cmos_parameters.bytes_per_sector), mDisk_Size(0) {

}

void CDisk_Drive::Set_Status(kiv_hal::TRegisters &context, const kiv_hal::NDisk_Status status) {
	context.flags.carry = status == kiv_hal::NDisk_Status::No_Error ? 0 : 1;
	if (context.flags.carry) context.rax.x = static_cast<uint16_t>(status);
}

void CDisk_Drive::Drive_Parameters(kiv_hal::TRegisters &context) {
	if (mDisk_Size == 0) {
		Set_Status(context, kiv_hal::NDisk_Status::Drive_Not_Ready);
		return;
	}

	kiv_hal::TDrive_Parameters &params = *reinterpret_cast<kiv_hal::TDrive_Parameters*>(context.rdi.r);

	const size_t MB = 1024 * 1024; //1MiB
	bool assisted_translation = true;
	
	if (mDisk_Size < 504 * MB) params.heads = 16;
	 else  if (mDisk_Size < 1008 * MB) params.heads = 32;
		else if (mDisk_Size < 2016 * MB) params.heads = 64;
			else if (mDisk_Size < 4032 * MB) params.heads = 128;
				else if (mDisk_Size < 8032 * MB) params.heads = 255;
				else
					//pro tyhle cisla nemame standardni prepocet, tak nastavime vse na max
					{
						params.sectors_per_track = std::numeric_limits<decltype(params.sectors_per_track)>::max();
						params.heads = std::numeric_limits<decltype(params.heads)>::max();
						params.cylinders = std::numeric_limits<decltype(params.cylinders)>::max();
						assisted_translation = false;
					};

	if (assisted_translation) {
		params.sectors_per_track = 63;
//		size_t tmp1 = static_cast<size_t>(params.sectors_per_track * params.heads);
		//size_t tmp2 = tmp1 * mBytes_Per_Sector;
		//tmp2 /= mDisk_Size;
		params.cylinders = static_cast<decltype(params.cylinders) > (mDisk_Size / (static_cast<size_t>(params.sectors_per_track) * static_cast<size_t>(params.heads) * mBytes_Per_Sector));
		//params.cylinders = static_cast<decltype(params.cylinders)>(tmp2);
	}

	params.bytes_per_sector = static_cast<decltype(params.bytes_per_sector)>(mBytes_Per_Sector);
	params.absolute_number_of_sectors = mDisk_Size / mBytes_Per_Sector;
	Set_Status(context, kiv_hal::NDisk_Status::No_Error);
}


bool CDisk_Drive::Check_DAP(kiv_hal::TRegisters &context) {
	kiv_hal::TDisk_Address_Packet &dap = *reinterpret_cast<kiv_hal::TDisk_Address_Packet*>(context.rdi.r);

	if (mBytes_Per_Sector*(dap.lba_index + dap.count) > mDisk_Size) {
		//nemuzeme dovolit, vysledkem by byl pristup za velikost disku
		Set_Status(context, kiv_hal::NDisk_Status::Sector_Not_Found);
		return false;
	}

	return true;
}

CDisk_Image::CDisk_Image(const TCMOS_Drive_Parameters &cmos_parameters) : CDisk_Drive(cmos_parameters) {
	auto open_mode = std::ios::binary | std::ios::in;
	if (!cmos_parameters.read_only) open_mode |= std::ios::out;
	mDisk_Image.open(cmos_parameters.disk_image, open_mode);	
	mDisk_Image.seekg(0, std::ios::end);      //rovnou nastavime pozici na konce souboru, protoze pri zapisu ji budeme stejne prestavovat
	mDisk_Size = mDisk_Image.tellg();
	if (mDisk_Image.fail()) mDisk_Size = 0;
}


void CDisk_Image::Read_Sectors(kiv_hal::TRegisters &context) {
	if (Check_DAP(context)) {
		kiv_hal::TDisk_Address_Packet &dap = *reinterpret_cast<kiv_hal::TDisk_Address_Packet*>(context.rdi.r);
		mDisk_Image.seekg(mBytes_Per_Sector*dap.lba_index, std::ios::beg);
		const auto bytes_to_read = dap.count*mBytes_Per_Sector;
		mDisk_Image.read(static_cast<char*>(dap.sectors), bytes_to_read);
		if (mDisk_Image.gcount() == bytes_to_read) Set_Status(context, kiv_hal::NDisk_Status::No_Error);
			else Set_Status(context, kiv_hal::NDisk_Status::Address_Mark_Not_Found_Or_Bad_Sector);
	}
}

void CDisk_Image::Write_Sectors(kiv_hal::TRegisters &context) {
	if (Check_DAP(context)) {
		kiv_hal::TDisk_Address_Packet &dap = *reinterpret_cast<kiv_hal::TDisk_Address_Packet*>(context.rdi.r);
		mDisk_Image.seekg(mBytes_Per_Sector*dap.lba_index, std::ios::beg);
		const auto bytes_to_write = dap.count*mBytes_Per_Sector;

		const auto before = mDisk_Image.tellp();
		mDisk_Image.write(static_cast<char*>(dap.sectors), bytes_to_write);
		const auto number_of_bytes_written = mDisk_Image.tellp() - before;

		if (number_of_bytes_written == bytes_to_write) Set_Status(context, kiv_hal::NDisk_Status::No_Error);
			else Set_Status(context, kiv_hal::NDisk_Status::Fixed_Disk_Write_Fault_On_Selected_Drive);
	}
}

CRAM_Disk::CRAM_Disk(const TCMOS_Drive_Parameters &cmos_parameters) : CDisk_Drive(cmos_parameters) {
	mDisk_Size = cmos_parameters.RAM_Disk_Size;
	mDisk_Image.resize(mDisk_Size);
}


void CRAM_Disk::Read_Sectors(kiv_hal::TRegisters &context) {
	if (Check_DAP(context)) {
		kiv_hal::TDisk_Address_Packet &dap = *reinterpret_cast<kiv_hal::TDisk_Address_Packet*>(context.rdi.r);

		memcpy(dap.sectors, &mDisk_Image[dap.lba_index*mBytes_Per_Sector], mBytes_Per_Sector*dap.count);
		Set_Status(context, kiv_hal::NDisk_Status::No_Error);
	}
}

void CRAM_Disk::Write_Sectors(kiv_hal::TRegisters &context) {
	if (Check_DAP(context)) {
		kiv_hal::TDisk_Address_Packet &dap = *reinterpret_cast<kiv_hal::TDisk_Address_Packet*>(context.rdi.r);

		memcpy(&mDisk_Image[dap.lba_index*mBytes_Per_Sector], dap.sectors, mBytes_Per_Sector*dap.count);
		Set_Status(context, kiv_hal::NDisk_Status::No_Error);
	}
}