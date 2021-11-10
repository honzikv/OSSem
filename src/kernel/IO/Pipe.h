#pragma once
#include <vector>

#include "IFile.h"
#include "Utils/Semaphore.h"

/// <summary>
/// Reprezentuje Pipe pro IPC
/// </summary>
class Pipe final : public IFile {

public:
	/// <summary>
	/// Defaultni velikost pro buffer
	/// </summary>
	static constexpr size_t DEFAULT_PIPE_BUFFER_SIZE = 1024; // 1 K
private:
	/// <summary>
	/// Semafor pro synchronizaci prazdnych polozek
	/// </summary>
	std::shared_ptr<Semaphore> write;

	/// <summary>
	/// Semafor pro synchronizaci plnych polozek
	/// </summary>
	std::shared_ptr<Semaphore> read;

	/// <summary>
	/// Index pro cteni
	/// </summary>
	size_t read_idx = {};

	/// <summary>
	/// Index pro psani
	/// </summary>
	size_t write_idx = {};

	/// <summary>
	/// Pocet polozek v bufferu. Tento field se musi editovat/cist pomoci buffer_access mutexu
	/// </summary>
	size_t items = {};

	/// <summary>
	/// Mutex pro pristup k bufferu
	/// </summary>
	std::mutex buffer_access;

	/// <summary>
	/// Mutex pro pristup k flagum pro cteni/zapis
	/// </summary>
	std::recursive_mutex flag_access;

	/// <summary>
	/// Buffer pro data
	/// </summary>
	std::vector<char> buffer;

	/// <summary>
	/// Zda-li se do pipe zapsalo vsechno potrebne
	/// </summary>
	bool write_finished = false;

	/// <summary>
	/// Zda-li se z pipe vsechno potrebne precetlo
	/// </summary>
	bool read_finished = false;

	/// <summary>
	/// Vrati, zda-li je pipe prazdna
	/// </summary>
	/// <returns>True pokud ano, jinak false</returns>
	[[nodiscard]] bool Empty() const;

	/// <summary>
	/// Posune read index o 1 dopredu. Pokud prekroci max index vrati se na zacatek
	/// </summary>
	void AdvanceReadIdx() {
		read_idx = (read_idx + 1) % buffer.size();
	}

	/// <summary>
	/// Posune write index o 1 dopredu. Pokud prekroci max index vrati se na zacatek
	/// </summary>
	void AdvanceWriteIdx() {
		write_idx = (write_idx = 1) % buffer.size();
	}

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
	kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) override;

	/// <summary>
	/// Zapise do bufferu data z bufferu
	/// </summary>
	/// <param name="source_buffer">Zdrojovy buffer, ze ktereho se data zapisuji</param>
	/// <param name="buffer_size">Velikost zdrojoveho bufferu - kolik bytu se ma zapsat</param>
	/// <param name="bytes_written">Pocet zapsanych bytu</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) override;
	
	/// <summary>
	/// Zavre pipu - odesle EOT do bufferu
	/// </summary>
	void Close();
};
