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
	std::atomic<TaskState> task_state = TaskState::Ready;

	/// <summary>
	/// Seznam handlu, ktere se maji po dokonceni notifikovat
	/// </summary>
	std::vector<kiv_os::THandle> on_finish_subscribers = {};

	/// <summary>
	/// Mutex pro synchronizaci
	/// </summary>
	std::mutex subscribers_mutex = {};

	/// <summary>
	/// Navratovy kod
	/// </summary>
	uint16_t task_exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Success);


public:
	virtual ~Task() = default;

	[[nodiscard]] const std::atomic<TaskState>& Get_Task_State() const;
	[[nodiscard]] const std::vector<kiv_os::THandle>& Get_On_Finish_Subscribers() const;
	[[nodiscard]] uint16_t Get_Task_Exit_Code() const;

	/// <summary>
	/// Prida subscriber handle do seznamu.
	///	Cokoliv, co tuto metodu vola musi nejprve zamknout lock pro suspend_callbacks
	/// </summary>
	/// <param name="subscriber_handle"></param>
	void Add_Subscriber(kiv_os::THandle subscriber_handle);

	/// <summary>
	/// Nastavi tasku exit code
	/// </summary>
	/// <param name="exit_code"></param>
	void Set_Exit_Code(uint16_t exit_code);

	/// <summary>
	/// Notifikuje subscribery o ukonceni tasku
	/// </summary>
	/// <param name="this_task_handle">handle pro tento task - pid nebo tid</param>
	virtual void Notify_Subscribers(kiv_os::THandle this_task_handle);

	/// <summary>
	/// Nastavi task na bezici
	/// </summary>
	void Set_Running();

	/// <summary>
	/// Nastavi task na stav finished
	/// </summary>
	void Set_Finished();
	
};
