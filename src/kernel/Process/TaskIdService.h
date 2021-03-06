#pragma once
#include <mutex>
#include <unordered_set>
#include "../../api/api.h"

/// <summary>
/// Typ handle pro wait for funkcionalitu
/// </summary>
enum class HandleType {
	INVALID,
	Thread,
	Process
};

/// <summary>
/// Jednoducha trida, ktera se stara o alokaci a dealokaci identifikatoru pro procesy a vlakna
/// </summary>
class TaskIdService {

public:
	// Rozsahy pro pidy a tidy
	static constexpr uint16_t PidRangeStart = 0;
	static constexpr uint16_t PidRangeEnd = 4096;
	static constexpr uint16_t TidRangeStart = 4096;
	static constexpr uint16_t TidRangeEnd = 8192;

	/// <summary>
	/// Maximalni pocet PIDu
	/// </summary>
	static constexpr uint16_t MaxPids = PidRangeEnd - PidRangeStart;

	/// <summary>
	/// Maximalni pocet TIDu
	/// </summary>
	static constexpr uint16_t MaxTids = TidRangeEnd - TidRangeStart;

private:
	/// <summary>
	/// Mutex pro synchronizaci
	/// </summary>
	std::mutex mutex;

	/// <summary>
	/// Alokovane pidy
	/// </summary>
	std::unordered_set<kiv_os::THandle> allocated_pids;

	/// <summary>
	/// Alokovane tidy
	/// </summary>
	std::unordered_set<kiv_os::THandle> allocated_tids;

	/// <summary>
	/// Posledni volny pid - zacatek pro vyhledavani volneho id
	/// </summary>
	kiv_os::THandle last_free_pid = PidRangeStart;

	/// <summary>
	/// Posledni volny tid
	/// </summary>
	kiv_os::THandle last_free_tid = TidRangeStart;

public:
	[[nodiscard]]
	/// <summary>
	/// Ziska volny pid
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle Get_Free_Pid();

	[[nodiscard]]
	/// <summary>
	/// Ziska volny tid
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle Get_Free_Tid();

	/// <summary>
	/// Dealokuje pid, pokud existuje
	/// </summary>
	/// <param name="pid"></param>
	void Remove_Pid(kiv_os::THandle pid);

	/// <summary>
	/// Dealokuje tid, pokud existuje
	/// </summary>
	/// <param name="tid"></param>
	void Remove_Tid(kiv_os::THandle tid);

	/// <summary>
	/// Ziska typ handle
	/// </summary>
	/// <param name="id">id handlu</param>
	/// <returns></returns>
	static HandleType Get_Handle_Type(kiv_os::THandle id);

	TaskIdService() {  }
};
