#pragma once
#include <vector>

#include "IFile.h"
#include "../Utils/Semaphore.h"

/// <summary>
/// Reprezentuje Pipe pro IPC
/// </summary>
class Pipe final {

public:
	/// <summary>
	/// Defaultni velikost pro buffer
	/// </summary>
	static constexpr size_t DEFAULT_PIPE_BUFFER_SIZE = 1024; // 1 K
private:
	/// <summary>
	/// Semafor pro pristup k polozkam na zapis
	/// </summary>
	std::shared_ptr<Semaphore> write;

	/// <summary>
	/// Semafor pro pristup k polozkam na cteni
	/// </summary>
	std::shared_ptr<Semaphore> read;

	/// <summary>
	/// Index pro cteni
	/// </summary>
	size_t reading_idx = 0;

	/// <summary>
	/// Index pro zapis
	/// </summary>
	size_t writing_idx = 0;

	/// <summary>
	/// Pocet polozek v bufferu. Tento field se musi editovat/cist pomoci buffer_access mutexu
	/// </summary>
	size_t items = 0;

	/// <summary>
	/// Mutex pro pristup k bufferu a flagum
	/// </summary>
	std::mutex pipe_access;

	/// <summary>
	/// Buffer pro data
	/// </summary>
	std::vector<char> buffer;

	/// <summary>
	/// Zapis je zavreny
	/// </summary>
	bool writing_closed = false;

	/// <summary>
	/// Ctnei je zavrene
	/// </summary>
	bool reading_closed = false;
	

	/// <summary>
	/// Vrati, zda-li je pipe prazdna
	/// </summary>
	/// <returns>True pokud ano, jinak false</returns>
	[[nodiscard]] bool Empty() const;

	/// <summary>
	/// Vrati, zda-li je pipe plna. Musi se synchronizovat mutexem
	/// </summary>
	/// <returns>True pokud ano, jinak false</returns>
	[[nodiscard]] bool Full() const;

	/// <summary>
	/// Posune read index o 1 dopredu. Pokud prekroci max index vrati se na zacatek
	/// </summary>
	void Advance_Reading_Idx();

	/// <summary>
	/// Posune write index o 1 dopredu. Pokud prekroci max index vrati se na zacatek
	/// </summary>
	void Advance_Writing_Idx();

public:
	/// <summary>
	/// Konstruktor pro vytvoreni pipe
	/// </summary>
	/// <param name="buffer_size">velikost bufferu</param>
	explicit Pipe(size_t buffer_size = DEFAULT_PIPE_BUFFER_SIZE);

	/// <summary>
	/// Precte z pipe data do ciloveho bufferu. Pokud je vse z pipe prectene, v bufferu bude zapsano EOT
	/// </summary>
	/// <param name="target_buffer">Buffer, kam se data maji zapsat</param>
	/// <param name="buffer_size">Velikost bufferu</param>
	/// <param name="bytes_read">Pocet prectenych bytu</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read);

	/// <summary>
	/// Zapise do bufferu data z bufferu
	/// </summary>
	/// <param name="source_buffer">Zdrojovy buffer, ze ktereho se data zapisuji</param>
	/// <param name="buffer_size">Velikost zdrojoveho bufferu - kolik bytu se ma zapsat</param>
	/// <param name="bytes_written">Pocet zapsanych bytu</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written);

	void Close_For_Reading();
	void Close_For_Writing();

};
