#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include "TaskState.h"
#include "../../api/api.h"


/// <summary>
/// Trida, ktera obsahuje spolecnou funkcionalitu pro vlakno i proces.
///	Jedna se o abstrakci vlakna/procesu
/// </summary>
class Task {
protected:
	TaskState task_state = TaskState::Ready;

	/// <summary>
	/// Seznam handlu, ktere se maji po dokonceni notifikovat
	/// </summary>
	std::vector<kiv_os::THandle> on_finish_subscribers;

	/// <summary>
	/// Mutex pro synchronizaci
	/// </summary>
	std::mutex mutex = {};

	/// <summary>
	/// Navratovy kod
	/// </summary>
	uint16_t task_exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Success);


public:
	virtual ~Task() = default;
	/// <summary>
	/// Ukonci vlakno. Tato funkce nic nedela, pokud se vlakno jiz ukoncilo.
	///	Pri ukonceni notifikuje vsechny vlakna / procesy, ktere na nej cekaji
	/// </summary>
	/// <param name="task_id">id tohoto tasku (pid nebo tid)</param>
	///	<param name="terminated_forcefully">Flag, ktery udava, zda-li task dobehl sam (true), nebo byl ukoncen nasilim</param>
	virtual void NotifySubscribers(kiv_os::THandle task_id, bool terminated_forcefully = true);

	/// <summary>
	/// Prida subscriber handle do seznamu
	/// </summary>
	/// <param name="subscriber_handle"></param>
	void AddSubscriber(kiv_os::THandle subscriber_handle);

	/// <summary>
	/// Nastavi tasku exit code
	/// </summary>
	/// <param name="exit_code"></param>
	void SetExitCode(uint16_t exit_code);

	/// <summary>
	/// Nastavi task na bezici
	/// </summary>
	void SetRunning();

	/// <summary>
	/// Nastavi task na finished
	/// </summary>
	void SetFinished();

	/// <summary>
	/// Vrati exit code
	/// </summary>
	/// <returns></returns>
	uint16_t GetExitCode();
};
