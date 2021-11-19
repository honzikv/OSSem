#include "../api/hal.h"
#include "keyboard.h"
#include "vga.h"
#include "disk.h"

#include <memory>
#include <Windows.h>

 
//Pouzijeme RAII, abychom si bezpecne uvolnit pamet, kterou budeme chtit, aby byla alokovana na fixni adresu
 class CIDT {
 protected:
	 kiv_hal::TInterrupt_Handler* mIDT = nullptr;
	 const size_t mIDT_Entries = 256;

	 void Release() {
		 if (mIDT != nullptr) {
			 VirtualFree(mIDT, 0, MEM_RELEASE);
			 mIDT = nullptr;
		 }
	 }

 public:
	 ~CIDT() {
		 Release();
	 }


	 bool Init() {
		 mIDT = static_cast<kiv_hal::TInterrupt_Handler*>(VirtualAlloc(reinterpret_cast<LPVOID>(kiv_hal::interrupt_descriptor_table), sizeof(kiv_hal::TInterrupt_Handler)*mIDT_Entries, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		 
		 if ((mIDT == kiv_hal::interrupt_descriptor_table) &&( mIDT != nullptr)) {
			 memset(mIDT, 0, sizeof(kiv_hal::TInterrupt_Handler) * mIDT_Entries);


			 kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, VGA_Handler);
			 kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, Disk_Handler);
			 kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, Keyboard_Handler);

			 return true;
		 }
		 else {
			 Release();
			 return false;
		 }

	 }

	 kiv_hal::TInterrupt_Handler* entries() const {
		 return mIDT;
	 }
 };



 CIDT IDT;


bool Init_IDT() {
	return IDT.Init();	
}