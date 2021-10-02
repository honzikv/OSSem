#include "../api/hal.h"
#include "keyboard.h"
#include "vga.h"
#include "disk.h"

#include <memory>
#include <Windows.h>

extern kiv_hal::TInterrupt_Handler* interrupt_descriptor_table;

kiv_hal::TInterrupt_Handler interrupt_descriptor_table_storage[256];

bool Init_IDT() {
	memset(interrupt_descriptor_table_storage, 0, sizeof(interrupt_descriptor_table_storage));
	interrupt_descriptor_table = interrupt_descriptor_table_storage;

	kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, VGA_Handler);
	kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, Disk_Handler);
	kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, Keyboard_Handler);

	auto index = TlsAlloc();
	if ((kiv_hal::Expected_Tls_IDT_Index != index) || !TlsSetValue(index, interrupt_descriptor_table)) {
		TlsFree(index);
		return false;
	}

	return true;
}