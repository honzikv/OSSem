#pragma once

#include <cstdint>
#include <limits>

#include "hal.h"

namespace kiv_os {
	const kiv_hal::NInterrupt System_Int_Number = kiv_hal::NInterrupt(0x21);
			//je to libovolne arbitrarni cislo, ktere neni uz obsazene v kiv_hal::NInterrupt

#ifndef KERNEL
	bool Sys_Call(kiv_hal::TRegisters &context);
#endif	
	using TThread_Proc = size_t(__stdcall *)(const kiv_hal::TRegisters &context);				//vstupni bod programu vlakna


	using THandle = uint16_t;  //16-bitu, aby se zabranilo predavani instanci trid mezi jadrem a procesem 
							   //namisto integerovych handlu jako v realnem OS
	constexpr THandle Invalid_Handle = static_cast<kiv_os::THandle>(-1);		//neplatny handle 

	struct TDir_Entry {
		uint16_t file_attributes;			//viz konstanty NFile_Attributes
		char file_name[8 + 1 + 3];	//8.3 FAT
	};

	//Cisla hlavnich skupin sluzeb, ktere poskytuje OS
	//zadava se do AH
	//je-li po volani nastavena vlajka carry, tj. TRegisters.flags.carry != 0, pak Rax je kod chyby - NOS_Error
	//Co nemuzete zjistit z API, to zkuste nejprve vyresit pomoci souboroveho systemu, napr.adresarem 0:\procfs

	enum class NOS_Service_Major : std::uint8_t {
		File_System = 1,	//souborove operace, std IO konzole jsou take jenom soubory
		Process
	};

	//vedlejsi cisla sluzbe pro IO operace, ktere poskytuje OS
	//zadava se do AL
	enum class NOS_File_System : std::uint8_t {
		Open_File = 1,					//IN: rdx je pointer na null - terminated ANSI char string udavajici file_name;
										//IN: rcx jsou flags k otevreni souboru - viz NOpen_File konstanty
										//IN: rdi jsou atributy souboru - viz NFile_Attributes
										//OUT : ax je handle nove otevreneho souboru

		Write_File,						//IN : dx je handle souboru, rdi je pointer na buffer, rcx je pocet bytu v bufferu k zapsani
										//OUT : rax je pocet zapsanych bytu

		Read_File,						//IN : dx je handle souboru, rdi je pointer na buffer, kam zapsat, rcx je velikost bufferu v bytech
										//OUT : rax je pocet prectenych bytu
		
		Seek,							//IN : dx je handle souboru, rdi je nova pozice v souboru
										//cl konstatna je typ pozice - viz NFile_Seek,
										//		Beginning : od zacatku souboru
										//		Current : od aktualni pozice v souboru
										//		End : od konce souboru

										//ch == Set_Position jenom nastavi pozici
										//ch == Set_Size nastav pozici a nastav velikost souboru na tuto pozici 
										//ch == Get_Position => OUT: rax je pozice v souboru od jeho zacatku

		Close_Handle,					//IN : dx  je handle libovolneho typu k zavreni

		Delete_File,					//IN : rdx je pointer na null - terminated ANSI char string udavajici file_name

		

		Set_Working_Dir,				//IN : rdx je pointer na null - terminated ANSI char string udavajici novy adresar(muze byt relativni cesta)
		Get_Working_Dir,				//IN : rdx je pointer na ANSI char buffer, rcx je velikost buffer
										//OUT : rax pocet zapsanych znaku

		Set_File_Attribute,				//IN: rdx je pointer na null - terminated ANSI char string udavajici file_name;
										//IN: rdi jsou atributy souboru - viz NFile_Attributes

		Get_File_Attribute,				//IN: rdx je pointer na null - terminated ANSI char string udavajici file_name;
										//OUT: rdi jsou atributy souboru - viz NFile_Attributes


		
		//vytvoreni adresare - vytvori se soubor s atributem adresar
		//smazani adresare - smaze se soubor
		//vypis adresare - otevre se adresar jako read - only soubor a cte se jako binarni soubor obsahujici jenom polozky TDir_Entry


		Create_Pipe,					//IN : rdx je pointer na pole dvou Thandle - prvni zapis a druhy pro cteni z pipy

		
	};


	enum class NOS_Process : std::uint8_t {
		Clone = 1,			//IN : rcx je NClone hodnota
							//	Create_Process: rdx je je pointer na null - terminated string udavajici jmeno souboru ke spusteni(tj.retezec pro GetProcAddress v kernelu)
							//		rdi je pointer na null-termined ANSI char string udavajici argumenty programu
							//		bx obsahuje 2x THandle na stdin a stdout, tj. bx.e = (stdin << 16) | stdout
							//OUT - v programu, ktery zavolal Clone: ax je handle noveho procesu 
							//		ve spustenem programu:	ax a bx jsou hodnoty stdin a stdout, stderr pro jednoduchost nepodporujeme
							//
							//anebo
							//	Create_Thread a pak rdx je TThread_Proc a rdi jsou *data
							//OUT : rax je handle noveho procesu / threadu
							//
							//	Funkce procesu i vlakna maji prototyp TThread_Proc, protoze proces na zacatku bezi jako jedno vlakno,
							//		context.rdi v TThread_Proc pak pro proces ukazuji na retezec udavajiciho jeho argumenty, tj. co bylo dano do rdi
							//		a u vlakna je to pointer na jeho data

		Wait_For,			//IN : rdx pointer na pole THandle, na ktere se ma cekat, rcx je pocet handlu
							//funkce se vraci jakmile je signalizovan prvni handle
							//OUT : rax je index handle, ktery byl signalizovan
		Read_Exit_Code,		//IN:  dx je handle procesu/thread jehoz exit code se ma cist
							//OUT: cx je exitcode

		Exit,				//ukonci proces/vlakno
							//IN: cx je exit code

		Shutdown,			//nema parametry, nejprve korektne ukonci vsechny bezici procesy a pak kernel, cimz se preda rizeni do boot.exe, ktery provede simulaci vypnuti pocitace pres ACPI
		Register_Signal_Handler		//IN: rcx NSignal_Id, rdx 
						//	a) pointer na TThread_Proc, kde pri jeho volani context.rcx bude id signalu
						//	b) 0 a pak si OS dosadi defualtni obsluhu signalu
	};


	
	//Navratove kody OS
	enum class NOS_Error : uint16_t {
		Success = 0,				//vse v poradku
		Unknown_Filesystem,
		Invalid_Argument,			//neplatna kombinace vstupnich argumentu
		File_Not_Found,				//soubor  nenalezen
		Directory_Not_Empty,		//adresar neni prazdny (a napr. proto nesel smazat)
		Not_Enough_Disk_Space,
		Out_Of_Memory,	
		Permission_Denied,
		IO_Error,

		Unknown_Error = static_cast<uint16_t>(-1)		//doposud neznama chyba		
	};

	enum class NSignal_Id : uint8_t {
		Terminate = 15		//SIGTERM
	};

	//atributy souboru
	enum class NFile_Attributes : std::uint8_t {
		Read_Only = 0x01,
		Hidden = 0x02,
		System_File = 0x04,
		Volume_ID = 0x08,
		Directory = 0x10,
		Archive = 0x20
	};

	//hodnoty typu nastaveni pozice
	enum class NFile_Seek : std::uint8_t {
		Beginning = 1,
		Current,
		End,

		Get_Position,
		Set_Position,
		Set_Size
	};


	//konstanty pro volani clone
	enum class NClone : std::uint8_t {
		Create_Process = 1,
		Create_Thread
	};

	//rezim otevreni noveho souboru
	enum class NOpen_File : std::uint8_t {
		fmOpen_Always = 1	//pokud je nastavena, pak soubor musi existovat, aby byl otevren
								//není-li fmOpen_Always nastaveno, pak je soubor vždy vytvoøen - tj. i pøepsán starý soubor
	};


	//Pokud byste potrebovali dalsi cisla sluzeb, chyb, apod. musite je zduvodnit prednasejicimu, aby pripadne rozsiril tento referencni api.h.
	//Jinak se pozadovane zmeny nebudou propagovat do referencniho api.h, se kterym se bude Vase semestralka prekladat a tudiz nepujde prelozit a nebude akceptovana.

}