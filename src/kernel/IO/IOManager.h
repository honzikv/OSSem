#pragma once
#include <unordered_set>

#include "HandleService.h"
#include "IFile.h"
#include "Pipe.h"
#include "../vfs.h"
#include "../fat12.h"
#include "../../api/hal.h"

/// <summary>
/// Singleton pro spravu IO
/// </summary>
class IOManager {

public:
	static IOManager& Get() {
		static IOManager instance;
		return instance;
	}

private:
	IOManager() = default;
	~IOManager() = default;
	IOManager(const IOManager&) = delete;
	IOManager& operator=(const IOManager&) = delete;

	/// <summary>
	/// Mutex pro pristup
	/// </summary>
	std::mutex mutex;

	/// <summary>
	/// Otevrene soubory - obsahuje dvojici pocet alokaci a referenci na soubor
	///	Pokud klesne pocet alokaci pod 1 je soubor automaticky odstranen
	///
	///	Jeden soubor muze byt sdilen mezi vice procesy
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::pair<int64_t, std::shared_ptr<IFile>>> open_files;

	/// <summary>
	/// PID -> mnozina file descriptoru, ktere muze proces pouzivat
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::unordered_set<kiv_os::THandle>> process_to_file_mapping;

	/**
	* Mapa souborovych systemu - dvojice nazev a souborovy system
	*/
	std::unordered_map<std::string, std::unique_ptr<VFS>> file_systems;

public:
	/**
	* Inicializuje souborove systemy
	*/
	void Init_Filesystems();

	/// <summary>
	/// Vrati zda-li ma proces pristup k file descriptoru
	/// </summary>
	/// <param name="pid"></param>
	/// <param name="file_descriptor">file descriptor</param>
	/// <returns>True pokud ano, jinak false</returns>
	bool Is_File_Descriptor_Accessible(kiv_os::THandle pid, kiv_os::THandle file_descriptor);

	/// <summary>
	/// Zvysi pocet vyskytu file descriptoru. Vrati File_Not_Found pokud soubor neexistuje
	/// </summary>
	/// <param name="file_descriptor"></param>
	/// <returns></returns>
	kiv_os::NOS_Error Increment_File_Descriptor_Count(kiv_os::THandle file_descriptor);

	/// <summary>
	/// Registruje procesu soubor - tzn. proces bude mit pravo tento handle pouzivat
	/// </summary>
	/// <param name="pid">pid procesu</param>
	/// <param name="file_descriptor">file descriptor, ktery registrujeme</param>
	/// <returns></returns>
	kiv_os::NOS_Error Register_File_To_Process(kiv_os::THandle pid, kiv_os::THandle file_descriptor);

	/// <summary>
	/// Snizi pocet vyskytu file descriptoru. Vrati File_Not_Found  pokud soubor neexistuje.
	///	Pokud ma soubor pocet vyskytu 0 nebo mensi je z open_files odstranen
	/// </summary>
	/// <param name="file_descriptor">file descriptor souboru</param>
	kiv_os::NOS_Error Decrement_File_Descriptor_Count(kiv_os::THandle file_descriptor);

	/// <summary>
	/// Odregistruje procesu soubor - tzn. proces ho nemuze nadale pouzivat
	/// </summary>
	/// <param name="pid">pid procesu</param>
	/// <param name="file_descriptor">file descriptor, ktery odebirame</param>
	/// <returns></returns>
	kiv_os::NOS_Error Unregister_File_From_Process(kiv_os::THandle pid, kiv_os::THandle file_descriptor);


	/// <summary>
	/// Funkce pro obsluhu IO pozadavku
	/// </summary>
	/// <param name="regs">Kontext</param>
	void Handle_IO(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vytvori psani z konzole a do konzole
	/// </summary>
	/// <returns>standardni vstup a vystup</returns>
	auto Create_Stdio() -> std::pair<kiv_os::THandle, kiv_os::THandle>;


	/// <summary>
	/// Registruje pro proces stdio
	/// </summary>
	/// <param name="pid">pid procesu</param>
	/// <param name="std_in">standardni vstup</param>
	/// <param name="std_out">standardni vystup</param>
	/// <returns>success, pokud vse probehlo v poradku jinak chybu</returns>
	kiv_os::NOS_Error Register_Process_Stdio(kiv_os::THandle pid, kiv_os::THandle std_in, kiv_os::THandle std_out);

	/// <summary>
	/// Odstrani stdio pro dany proces
	/// </summary>
	/// <param name="pid">pid procesu</param>
	/// <param name="std_in">standardni vstup</param>
	/// <param name="std_out">standardni vystup</param>
	/// <returns>success, pokud vse probehlo v poradku jinak chybu</returns>
	kiv_os::NOS_Error Unregister_Process_Stdio(kiv_os::THandle pid, kiv_os::THandle std_in, kiv_os::THandle std_out);

	/// <summary>
	/// Zavre vsechny file descriptory, ktere patri procesu s danym pidem
	/// </summary>
	/// <param name="pid">pid procesu</param>
	void Close_Process_File_Descriptors(kiv_os::THandle pid);

private:
	/// <summary>
	/// Provede cteni ze souboru - pro soubor zavola metodu read()
	///
	///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
	/// </summary>
	/// <param name="regs">Kontext</param>
	///	<returns>Vysledek operace</returns>
	kiv_os::NOS_Error Syscall_Read(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Provede zapis do souboru - pro soubor zavola metodu write().
	///
	///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
	/// </summary>
	/// <param name="regs">Kontext</param>
	///	<returns>Vysledek operace</returns>
	kiv_os::NOS_Error Syscall_Write(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vytvori novou pipe
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Syscall_Create_Pipe(const kiv_hal::TRegisters& regs);

	/// <summary>
	/// Zavre handle - odstrani ho z tabulky otevrenych souboru
	/// </summary>
	/// <param name="regs">Kontext</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Syscall_Close_Handle(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Set_Working_Dir(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Get_Working_Dir(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Get_File_Attribute(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Open_File(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Delete_File(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Seek(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Set_File_Attribute(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Open_File(Path path, kiv_os::NOpen_File flags, uint8_t attributes, kiv_os::THandle& handle);

	VFS* Get_File_System(const std::string& disk);
};
