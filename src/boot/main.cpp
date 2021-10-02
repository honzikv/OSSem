#include <Windows.h>
#include <iostream>

#include "../api/hal.h"
#include "idt.h"
#include "keyboard.h"

bool Setup_HW() {
	if (!Init_Keyboard()) {
		std::wcout << L"Nepodarilo se vypnout konzolove echo, mohly by byt chyby na vystupu => koncime." << std::endl;
		return false;
	}


	//pripraveme tabulku vektoru preruseni
	if (!Init_IDT()) {
		std::wcout << L"Nepodarilo se spravne inicializovat IDT!" << std::endl;
		return false;
	}

	//disky tady inicializovat nebudeme, ty at si klidne selzou treba behem chodu systemu

	return true;
}

int __cdecl main() {
	if (!Setup_HW()) return 1;

	//HW je nastaven, zavedeme simulovany operacni system
	HMODULE kernel = LoadLibraryW(L"kernel.dll");
	if (!kernel) {
		std::wcout << L"Nelze nacist kernel.dll!" << std::endl;
		return 1;
	}

	//v tuto chvili DLLMain v kernel.dll mela nahrat na NInterrupt::Bootstrap_Loader adresu pro inicializaci jadra
	//takze ji spustime
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Bootstrap_Loader, kiv_hal::TRegisters{ 0 });	

	
	//a az simulovany OS skonci, uvolnime zdroje z pameti
	FreeLibrary(kernel);
	TlsFree(kiv_hal::Expected_Tls_IDT_Index);
	return 0;
}