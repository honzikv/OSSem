#pragma once

#include "..\api\api.h"
#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace kiv_os_rtl {

	/// <summary>
	/// Reference pro ziskani posledni chyby
	/// </summary>
	extern std::atomic<kiv_os::NOS_Error> Last_Error;

	/// <summary>
	/// Vrati posledni chybu po syscallu
	/// </summary>
	/// <returns>Posledni chybu</returns>
	kiv_os::NOS_Error GetLastError();

	bool ReadFile(kiv_os::THandle file_handle, char* buffer, size_t buffer_size, size_t& read);
	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti BUFFER_SIZE a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK

	/// <summary>
	/// Cte data ze vstupu, dokud nenarazi na EOF
	/// </summary>
	/// <param name="std_in">Vstup</param>
	/// <param name="buffer">Buffer, kam se data zapisuji</param>
	void ReadIntoBuffer(kiv_os::THandle std_in, std::vector<char>& buffer);

	bool WriteFile(kiv_os::THandle file_handle, const char* buffer, size_t buffer_size, size_t& written);
	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti BUFFER_SIZE a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK

	/// <summary>
	/// Vytvori pipe pro vstup a vystup. Pokud je vstup/vystup jako kiv_os::Invalid_Value vygeneruje novy file descriptor
	/// </summary>
	/// <param name="writing_process_output">Reference na promennou se vstupem - sem se zapise zaroven vysledek po vytvoreni</param>
	/// <param name="reading_process_input">Reference na promennou s vystupem - sem se zapise zaroven vysledek po vytvoreni</param>
	/// <returns>Vysledek operace</returns>
	bool CreatePipe(kiv_os::THandle& writing_process_output, kiv_os::THandle& reading_process_input);

	/// <summary>
	/// Otevre file z filesystemu
	/// </summary>
	/// <param name="file_descriptor">Reference na promennou, kam se zapise vytvoreny file descriptor</param>
	/// <param name="file_uri">Uri souboru</param>
	/// <param name="mode">Rezim otevreni</param>
	/// <returns>Vysledek operace</returns>
	bool OpenFsFile(kiv_os::THandle& file_descriptor, const std::string& file_uri, kiv_os::NOpen_File mode);

	/// <summary>
	/// Zavre handle daneho file descriptoru
	/// </summary>
	/// <param name="file_descriptor">Handle (fd), ktere chceme zavrit</param>
	/// <returns>Vysledek operace</returns>
	bool CloseHandle(kiv_os::THandle file_descriptor);

	/// <summary>
	/// Nastavi procesu pracovni adresar
	/// </summary>
	/// <param name="params">parametry</param>
	/// <returns>Vysledek operace</returns>
	bool SetWorkingDir(const std::string& params);

	/// <summary>
	/// Ziska aktualni pracovni adresar procesu
	/// </summary>
	/// <param name="buffer">Buffer, kam se maji data zapsat</param>
	/// <param name="new_dir_buffer_size">Velikost bufferu</param>
	/// <param name="new_directory_str_size">velikost stringu po zapsani</param>
	bool GetWorkingDir(char* buffer, const uint32_t new_dir_buffer_size, uint32_t& new_directory_str_size);

	bool CreateProcess(const std::string& program_name, const std::string& params, kiv_os::THandle std_in,
	                   kiv_os::THandle std_out, kiv_os::THandle& pid);

	bool CreateThread(const std::string& program_name, const std::string& params, kiv_os::THandle std_in, kiv_os::THandle std_out);

	/// <summary>
	/// Aktualni vlakno vycka, dokud dany handle nedobehne
	/// </summary>
	/// <param name="handles">Handly, na ktere se ceka</param>
	/// <returns>Vzdy true</returns>
	bool WaitFor(const std::vector<kiv_os::THandle>& handles);

	/// <summary>
	/// Precte exit code pro dany pid. Proces musi byt ukonceny - tzn. musi se na nej pockat pomoci WaitFor, jinak
	///	se exit code precte spatne
	/// </summary>
	/// <param name="pid">process id</param>
	/// <param name="exit_code">Promenna pro zapsani exit codu</param>
	bool ReadExitCode(kiv_os::THandle pid, kiv_os::NOS_Error& exit_code);

	/// <summary>
	/// Vypnuti OS
	/// </summary>
	/// <returns></returns>
	void Shutdown();
}
