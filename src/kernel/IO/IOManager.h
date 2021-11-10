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
	/// Otevrene soubory
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::shared_ptr<IFile>> open_files;

public:
	/// <summary>
	/// Funkce pro obsluhu IO pozadavku
	/// </summary>
	/// <param name="regs">Kontext</param>
	void HandleIO(kiv_hal::TRegisters& regs) {
		const auto operation = regs.rax.l;
		switch (static_cast<kiv_os::NOS_File_System>(operation)) {
			case kiv_os::NOS_File_System::Read_File: {
				PerformRead(regs);
				break;
			}

			case kiv_os::NOS_File_System::Write_File: {
				PerformWrite(regs);
				break;
			}

			case kiv_os::NOS_File_System::Create_Pipe: {
				CreatePipe(regs);
				break;
			}

			case kiv_os::NOS_File_System::Close_Handle: {
				CloseHandle(regs);
			}
		}
	}

	/// <summary>
	/// Vytvori psani z konzole a do konzole
	/// </summary>
	/// <returns>standardni vstup a vystup</returns>
	auto CreateStdIO() -> std::pair<kiv_os::THandle, kiv_os::THandle>;

private:
	/// <summary>
	/// Provede cteni ze souboru - pro soubor zavola metodu read()
	///
	///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
	/// </summary>
	/// <param name="regs">Kontext</param>
	///	<returns>Vysledek operace</returns>
	static kiv_os::NOS_Error PerformRead(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Provede zapis do souboru - pro soubor zavola metodu write().
	///
	///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
	/// </summary>
	/// <param name="regs">Kontext</param>
	///	<returns>Vysledek operace</returns>
	static kiv_os::NOS_Error PerformWrite(kiv_hal::TRegisters& regs);

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
};
