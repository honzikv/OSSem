#pragma once
#include "IFile.h"
#include "ProcFSRow.h"

/// <summary>
/// Snapshot stavu aktualni tabulky procesu
/// </summary>
struct ProcessTableSnapshot final : public IFile {

	static constexpr size_t MaxProcessNameLen = 24; // proces muze mit maximalni delku nazvu 8 znaku

	/// <summary>
	/// Buffer s daty. Obsahuje n radku s informaci o procesu
	///
	///	Radky se skladaji vzdy z:
	///	uint16_t - pid, uint64_t - pointer na null-terminated string v process_name, uint32_t - pocet vlaken a
	///	uint8_t - stav
	/// </summary>
	std::vector<char> bytes;

	/// <summary>
	/// Aktualni index pro cteni v bufferu
	/// </summary>
	size_t idx = 0;

	explicit ProcessTableSnapshot(std::vector<std::shared_ptr<ProcFSRow>> procfs_rows);

	kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) override;

	kiv_os::NOS_Error Close() override;

	kiv_os::NOS_Error Seek(size_t position, kiv_os::NFile_Seek seek_type, kiv_os::NFile_Seek seek_operation,
	                       size_t& res_pos) override;

};
