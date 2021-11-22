#pragma once
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
	std::recursive_mutex mutex;

	/// <summary>
	/// Otevrene soubory - obsahuje dvojici pocet alokaci a referenci na soubor
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::pair<int64_t, std::shared_ptr<IFile>>> open_files;

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
	/// Funkce pro obsluhu IO pozadavku
	/// </summary>
	/// <param name="regs">Kontext</param>
	void HandleIO(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vytvori psani z konzole a do konzole
	/// </summary>
	/// <returns>standardni vstup a vystup</returns>
	auto CreateStdIO() -> std::pair<kiv_os::THandle, kiv_os::THandle>;

	/// <summary>
	/// Registruje std_in a std_out handly pro dany proces. Pokud neexistuji vyhodi NOS_Error::IO_Error
	/// </summary>
	/// <param name="std_in">Handle pro stdin</param>
	/// <param name="std_out">Handle pro stdout</param>
	/// <returns>Success nebo IO_Error v pripade chyby</returns>
	kiv_os::NOS_Error RegisterProcessStdIO(kiv_os::THandle std_in, kiv_os::THandle std_out);

	kiv_os::NOS_Error UnregisterProcessStdIO(kiv_os::THandle std_in, kiv_os::THandle std_out);

	/// <summary>
	/// Odregistruje referenci na soubor o 1, pokud na soubor nic neukazuje smaze ho z mapy a nastavi handle removed na true
	/// </summary>
	/// <param name="handle"></param>
	/// <param name="handle_removed">Zprava o tom, zda-li se handle z mapy odstranilo</param>
	/// <returns></returns>
	void DecrementFileReference(kiv_os::THandle handle, bool& handle_removed);

private:
	/// <summary>
	/// Provede cteni ze souboru - pro soubor zavola metodu read()
	///
	///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
	/// </summary>
	/// <param name="regs">Kontext</param>
	///	<returns>Vysledek operace</returns>
	kiv_os::NOS_Error PerformRead(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Provede zapis do souboru - pro soubor zavola metodu write().
	///
	///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
	/// </summary>
	/// <param name="regs">Kontext</param>
	///	<returns>Vysledek operace</returns>
	kiv_os::NOS_Error PerformWrite(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vytvori novou pipe
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error PerformCreatePipe(const kiv_hal::TRegisters& regs);

	/// <summary>
	/// Zavre handle - odstrani ho z tabulky otevrenych souboru
	/// </summary>
	/// <param name="regs">Kontext</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error PerformCloseHandle(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformSetWorkingDir(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformGetWorkingDir(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformGetFileAttribute(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformOpenFile(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformDeleteFile(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformSeek(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformSetFileAttribute(const kiv_hal::TRegisters& regs);

    kiv_os::NOS_Error OpenFile(Path path, kiv_os::NOpen_File flags, uint8_t attributes, kiv_os::THandle &handle);

    VFS *GetFileSystem(const std::string& disk);
};
