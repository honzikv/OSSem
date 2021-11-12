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
	std::atomic<TaskState> task_state = {TaskState::Ready};

	/// <summary>
	/// Seznam handlu, ktere se maji po dokonceni notifikovat
	/// </summary>
	std::vector<kiv_os::THandle> on_finish_subscribers;

	/// <summary>
	/// Mutex pro synchronizaci
	/// </summary>
	std::mutex on_finish_subscribers_mutex = {};

	/// <summary>
	/// Navratovy kod
	/// </summary>
	kiv_os::NOS_Error task_exit_code = kiv_os::NOS_Error::Success;


public:
	/// <summary>
	/// Ukonci vlakno. Tato funkce nic nedela, pokud se vlakno jiz ukoncilo.
	///	Pri ukonceni notifikuje vsechny vlakna / procesy, ktere na nej cekaji
	/// </summary>
	/// <param name="task_id">id tohoto tasku (pid nebo tid)</param>
	void Finish(kiv_os::THandle task_id);

	/// <summary>
	/// Prida subscriber handle do seznamu
	/// </summary>
	/// <param name="subscriber_handle"></param>
	void AddSubscriber(kiv_os::THandle subscriber_handle);

	void SetRunning();

	void SetBlocked();

	void SetFinished();

	inline kiv_os::NOS_Error GetExitCode() {
		if (task_state == TaskState::Finished) {
			return task_exit_code;
		}

		return kiv_os::NOS_Error::Unknown_Error;
	}
};
