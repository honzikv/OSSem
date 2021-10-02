#pragma once

#include <cstdint>

//HAL aka hardware abstraction layer for the kiv_os namespace

namespace kiv_hal {

	struct TGeneral_Register {
		union {
			uint64_t r;			//e.g., rax
			uint32_t e;			//e.g., eax
			uint16_t x;			//e.g.; ax
			struct {
				std::uint8_t l;		//e.g., al
				std::uint8_t h;		//e.g., ah
			};
		};
	};

	struct TIndex_Register {
		union {
			uint64_t r;			//e.g., rdi
			uint32_t e;			//e.g., edi
			uint16_t i;			//e.g.; di
		};
	};

	struct TFlags {
		std::uint8_t carry : 1;
		std::uint8_t non_zero : 1;
	};

	struct TRegisters;

	using TInterrupt_Handler = void(__stdcall *)(TRegisters &context);			//prototyp funkce, ktera realizuje syscall
	using TInterrupt_Descriptor_Table = TInterrupt_Handler*;					//ziskani ukazatele na tabulku se deje v dllmain.cpp, ktery nemate povoleno menit
	

	const uint32_t Expected_Tls_IDT_Index = 1;					//k ziskani ukazatele na tabulku vektoru preruseni pouzijeme TLS s ocekavanou hodnotu TLS indexu
	
	struct TRegisters {
		TGeneral_Register rax, rbx, rcx, rdx;
		TIndex_Register rdi;
		TFlags flags;
	};	//vice registru neni treba



	//definovano pri prekladu boot.exe (aka hw abstraction layer - HAL) nebo kernel.dll
	enum class NInterrupt : std::uint8_t {
		VGA_BIOS = 0x10,						//viz NVGA_BIOS
		Disk_IO = 0x13,							//viz NDisk_IO
		Keyboard = 0x16,						//viz NKeyboard		
		Bootstrap_Loader = 0x19					//vektor na rutinnu inicializace jadra OS
	};

#ifdef KERNEL
	void Set_Interrupt_Handler(NInterrupt intterupt, TInterrupt_Handler handler);
	void Call_Interrupt_Handler(NInterrupt intterupt, TRegisters &context);
			//programy v uzivatelskem adresovem prostoru maji svoji fci v api.h - Sys_Call
#endif


	//sluzby VGA BIOSu, cislo sluzby patri do ah
	enum class NVGA_BIOS {
		Write_Control_Char,							//zapise kontrolni znak, napr. umi backspace (NControl_Codes::BS) namisto zobrazeni piktogramu
													//IN: kontrolni znak je v dl

		Write_String								//zapise retezec znaku od aktualni pozice kurzoru
													//IN: rdx ukazuje na buffer znaku, char, k zapsani
													//	  rcx je pocet znaku k zapsani
	};


	//diskovy sluzby BIOSu, cislo sluzby patri do ah
	enum class NDisk_IO {
		Read_Sectors = 0x42,			//precti sektory
										//IN: dl je cislo disku
										//	  rdi je adresa TDisk_Address_Packet
										//OUT: Carry pokud je chyba
										//		ax je NDisk_Status

		Write_Sectors = 0x43,			//zapis sektory
										//IN: dl je cislo disku
										//	  rdi je adresa TDisk_Address_Packet
										//OUT: Carry pokud je chyba
										//		ax je NDisk_Status

		Drive_Parameters = 0x48			//ziskej informaci o disku
										//IN:  dl cislo disku (0=A:, 1=druha mechanika, 0x80 prvni disk, 0x81 druhy disk, 0eh cd/dvd, etc.)
										//		rdi je ukazatel na TDrive_Parameters
										//OUT: carry pokud je chyba
										//		ax je NDisk_Status
	};
	
	struct TDisk_Address_Packet {
		uint64_t count;			//pocet sektoru k preceteni/zapisu
		void* sectors;			//ukazatel na buffer sektoru k precteni/zapisu
		uint64_t lba_index;		//adresa prvniho sektoru na disku
	};

	struct TDrive_Parameters {
		uint32_t cylinders, heads, sectors_per_track;
		uint64_t absolute_number_of_sectors;
		uint16_t bytes_per_sector;
	};

	//sluzby klavesnice, cislo sluzby patri do ah
	enum class NKeyboard : std::uint8_t {
		Peek_Char = 1,			//otestuj, zda je na vstupu znak; neblokujici volani
								//OUT: je-li  na vstupu je znak, pak je nastavena vlajka TRegisters::Flags::non_zero
								//		ad nasledujici Read_Char, NControl_Codes::EOT spolu se shozenou TRegisters::Flags::non_zero je povazovan za platnou kombinaci, tj. znak na vstupu
		Read_Char,				//OUT: ax obsahuje kod znaku; blokujici volani
								//     vlajka TRegisters::Flags::non_zero je nastavena, pokud byl znak skutecne precten
								//		pokud nastavena neni, pak
								//			a) je-li ax==EOT, pak byl vstup korektne odpojen
								//			b) doslo k chybe cteni z klavesnice

		Write_Char,				//IN: al kod znaku ke vlozeni do buffer klavesnice, napr. ke korektnimu ukonceni programu cekajiciho na EOT
								//OUT: je-li  nastavena vlajka TRegisters::Flags::non_zero, pak byl znak uspesne vlozen, jinak chyba "buffer full"
	};

	enum class NControl_Codes : std::uint8_t {
		NUL = 0,		//null, aneb end of string
		ETX = 3,		//end of text, aneb Ctrl+C
		EOT = 4,		//end of transmission, aneb Ctrl+D
		BS = 8,			//backspace
		TAB = 9,		//tabulator
		LF = 10,		//0x0a aka Line Feed
		CR = 13,		//0x0d aka Carriage Return
		SUB = 26		//substitue, aneb Ctrl+Z aneb EOF DOSu
	};

	enum class NDisk_Status : std::uint8_t {
		No_Error = 0,
		Bad_Command,
		Address_Mark_Not_Found_Or_Bad_Sector,
		Sector_Not_Found = 4,
		Bad_Fixed_Disk_Parameter_Table = 7,
		Fixed_Disk_Write_Fault_On_Selected_Drive = 0xCC,
		Drive_Not_Ready = 0xAA
	};

};