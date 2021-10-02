#pragma once

#include "..\api\api.h"
#include <atomic>

namespace kiv_os_rtl {

	extern std::atomic<kiv_os::NOS_Error> Last_Error;

	bool Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read);
		//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
		//vraci true, kdyz vse OK
		//vraci true, kdyz vse OK

	bool Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written);
	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK

}