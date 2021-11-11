#pragma once
#include "HandleService.h"
#include "IFile.h"
#include "Pipe.h"

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
	/// Otevrene soubory
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::shared_ptr<IFile>> open_files;

public:
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
	/// Namapuje stdio na nove hodnoty pro novy proces
	/// </summary>
	/// <param name="std_in">stary stdin</param>
	/// <param name="std_out">stary stdout</param>
	/// <returns>Par novych handlu pro novy proces, vraci nevalidni hodnoty pokud nastala chyba</returns>
	auto MapProcessStdio(kiv_os::THandle std_in, kiv_os::THandle std_out) -> std::pair<kiv_os::THandle, kiv_os::THandle>;

	/// <summary>
	/// Zavre oba handlery
	/// </summary>
	/// <param name="std_in">stdin, ktery se odmapuje</param>
	/// <param name="std_out">stdout, ktery se odmapuje</param>
	/// <returns></returns>
	void CloseProcessStdio(kiv_os::THandle std_in, kiv_os::THandle std_out);

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
	kiv_os::NOS_Error CreatePipe(const kiv_hal::TRegisters& regs);

	/// <summary>
	/// Zavre handle - odstrani ho z tabulky otevrenych souboru
	/// </summary>
	/// <param name="regs">Kontext</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error CloseHandle(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error SetWorkingDir(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error GetWorkingDir(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error GetFileAttribute(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error OpenFsFile(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
};
