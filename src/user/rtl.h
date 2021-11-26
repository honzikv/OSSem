#pragma once

#include "../api/api.h"
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
	kiv_os::NOS_Error Get_Last_Err();

	/// <summary>
	/// Precte ze souboru
	/// </summary>
	/// <param name="file_descriptor">File descriptor souboru</param>
	/// <param name="buffer">Reference na buffer</param>
	/// <param name="buffer_size">Velikost bufferu</param>
	/// <param name="read">Pocet prectenych bytu</param>
	/// <returns>True, pokud operace dobehla uspesne, jinak false</returns>
	bool Read_File(kiv_os::THandle file_descriptor, char* buffer, size_t buffer_size, size_t& read);

	/// <summary>
	/// Cte data ze vstupu, dokud nenarazi na EOF
	/// </summary>
	/// <param name="std_in">Vstup</param>
	/// <param name="buffer">Buffer, kam se data zapisuji</param>
	void Read_Into_Buffer(kiv_os::THandle std_in, std::vector<char>& buffer);

	/// <summary>
	/// Zapise do souboru
	/// </summary>
	/// <param name="file_descriptor">File descriptor souboru</param>
	/// <param name="buffer">Buffer, ze ktereho se data ctou</param>
	/// <param name="buffer_size">Velikost bufferu - resp. pocet bytu, ktere se maji precist</param>
	/// <param name="written">Pocet zapsanych bytu</param>
	/// <returns>True, pokud operace dobehla uspesne, jinak false</returns>
	bool Write_File(kiv_os::THandle file_descriptor, const char* buffer, size_t buffer_size, size_t& written);


	/// <summary>
	/// Vytvori pipe pro vstup a vystup. Pokud je vstup/vystup jako kiv_os::Invalid_Value vygeneruje novy file descriptor
	/// </summary>
	/// <param name="writing_process_output">Reference na promennou se vstupem - sem se zapise zaroven vysledek po vytvoreni</param>
	/// <param name="reading_process_input">Reference na promennou s vystupem - sem se zapise zaroven vysledek po vytvoreni</param>
	/// <returns>Vysledek operace</returns>
	bool Create_Pipe(kiv_os::THandle& writing_process_output, kiv_os::THandle& reading_process_input);

	/// <summary>
	/// Otevre file z filesystemu
	/// </summary>
	/// <param name="file_descriptor">Reference na promennou, kam se zapise vytvoreny file descriptor</param>
	/// <param name="file_uri">Uri souboru</param>
	/// <param name="mode">Rezim otevreni</param>
	/// <param name="attributes">Atributy souboru</param>
	/// <returns>Vysledek operace</returns>
	bool Open_File(kiv_os::THandle& file_descriptor, const std::string& file_uri, kiv_os::NOpen_File mode, const kiv_os::NFile_Attributes
	               attributes);

	/// <summary>
	/// Provede seek v souboru
	/// </summary>
	/// <param name="file_descriptor">File descriptor souboru</param>
	/// <param name="new_pos">Nova pozice - posun vuci seek_type</param>
	/// <param name="seek_type">Typ seeku</param>
	/// <returns></returns>
	bool Seek(kiv_os::THandle file_descriptor, uint64_t new_pos, kiv_os::NFile_Seek seek_type);

	/// <summary>
	/// Smaze soubor
	/// </summary>
	/// <param name="file_name">jmeno souboru</param>
	/// <returns></returns>
	bool Delete_File(const std::string& file_name);

	/// <summary>
	/// Nastavi atribut danemu souboru
	/// </summary>
	/// <param name="file_name">jmeno souboru</param>
	/// <param name="attributes">Atribut, ktery se ma nastvit</param>
	/// <param name="new_attributes">Zde se zapise vysledek po nastaveni atributu</param>
	/// <returns></returns>
	bool Set_File_Attribute(const std::string& file_name, kiv_os::NFile_Attributes attributes, kiv_os::NFile_Attributes& new_attributes);

	/// <summary>
	/// Ziska atributy souboru
	/// </summary>
	/// <param name="file_name">jmeno souboru</param>
	/// <param name="attributes">Sem se prectou atributy</param>
	/// <returns></returns>
	bool Get_File_Attributes(const std::string& file_name, kiv_os::NFile_Attributes& attributes);

	/// <summary>
	/// Zavre handle daneho file descriptoru
	/// </summary>
	/// <param name="file_descriptor">Handle (fd), ktere chceme zavrit</param>
	/// <returns>Vysledek operace</returns>
	bool Close_File_Descriptor(kiv_os::THandle file_descriptor);

	/// <summary>
	/// Nastavi procesu pracovni adresar
	/// </summary>
	/// <param name="params">parametry</param>
	/// <returns>Vysledek operace</returns>
	bool Set_Working_Dir(const std::string& params);

	/// <summary>
	/// Ziska aktualni pracovni adresar procesu
	/// </summary>
	/// <param name="buffer">Buffer, kam se maji data zapsat</param>
	/// <param name="new_dir_buffer_size">Velikost bufferu</param>
	/// <param name="new_directory_str_size">velikost stringu po zapsani</param>
	bool Get_Working_Dir(char* buffer, const uint32_t new_dir_buffer_size, uint32_t& new_directory_str_size);

	/// <summary>
	/// Vytvori novy proces pomoci clone
	/// </summary>
	/// <param name="program_name">Jmeno programu</param>
	/// <param name="params">Parametry</param>
	/// <param name="std_in">standardni vstup</param>
	/// <param name="std_out">standardni vystup</param>
	/// <param name="pid">pid vytvoreneho programu</param>
	/// <returns></returns>
	bool Create_Process(const std::string& program_name, const std::string& params, kiv_os::THandle std_in,
	                   kiv_os::THandle std_out, kiv_os::THandle& pid);

	/// <summary>
	/// Vytvori vlakno
	/// </summary>
	/// <param name="program_name">Jmeno programu</param>
	/// <param name="params">Paramtery programu</param>
	/// <param name="std_in">standardni vstup</param>
	/// <param name="std_out">standardni vystup</param>
	/// <returns></returns>
	bool Create_Thread(const std::string& program_name, const std::string& params, kiv_os::THandle std_in, kiv_os::THandle std_out);

	/// <summary>
	/// Aktualni vlakno vycka, dokud dany handle nedobehne
	/// </summary>
	/// <param name="handles">Handly, na ktere se ceka</param>
	/// <returns>Vzdy true</returns>
	bool Wait_For(const std::vector<kiv_os::THandle>& handles);

	/// <summary>
	/// Precte exit code pro dany pid. Proces musi byt ukonceny - tzn. musi se na nej pockat pomoci WaitFor, jinak
	///	se exit code precte spatne
	/// </summary>
	/// <param name="pid">process id</param>
	/// <param name="exit_code">Promenna pro zapsani exit codu</param>
	bool Read_Exit_Code(kiv_os::THandle pid, kiv_os::NOS_Error& exit_code);

	bool Register_Signal_Handler(kiv_os::NSignal_Id signal, kiv_os::TThread_Proc callback);

	/// <summary>
	/// Vypnuti OS
	/// </summary>
	/// <returns></returns>
	void Shutdown();
}
