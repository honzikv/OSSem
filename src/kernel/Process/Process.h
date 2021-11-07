#pragma once
#include <string>
#include <unordered_map>

#include "RunnableState.h"
#include "Thread.h"
#include "../api/api.h"



/// <summary>
/// Reprezentuje proces (resp. PCB)
/// </summary>
class Process {

	/// <summary>
	/// Stav procesu
	/// </summary>
	RunnableState process_state = RunnableState::Ready;

	/// <summary>
	/// PID
	/// </summary>
	kiv_os::THandle pid;

	/// <summary>
	/// PID rodice
	/// </summary>
	kiv_os::THandle parent_pid;

	/// <summary>
	/// Standardni vstup
	/// </summary>
	kiv_os::THandle std_in;

	/// <summary>
	/// Standardni vystup
	/// </summary>
	kiv_os::THandle std_out;

	/// <summary>
	/// Slozka, ve ktere proces aktualne operuje
	/// </summary>
	std::string working_dir;

	/// <summary>
	/// Mapping signalu a callbacku, ktery se provede po zavolani
	/// </summary>
	std::unordered_map<kiv_os::NSignal_Id, kiv_os::TThread_Proc> signal_callbacks;

	/// <summary>
	/// Seznam vlaken, ktere patri tomuto procesu. Vlakno na 0tem indexu je main
	/// </summary>
	std::vector<kiv_os::THandle> threads;

	/// <summary>
	/// Seznam handleru, co cekaji na dokonceni tohoto procesu
	/// </summary>
	std::vector<kiv_os::THandle> on_finish_subscribers;

	/// <summary>
	/// Navratovy kod procesu
	/// </summary>
	kiv_os::NOS_Error return_code = kiv_os::NOS_Error::Success;

public:
	Process(kiv_os::THandle pid, kiv_os::THandle main_thread_tid, kiv_os::THandle parent_pid,
	        kiv_os::THandle std_in, kiv_os::THandle std_out);

	void Assign_Thread(kiv_os::THandle tid);

	inline void Set_Running() {
		process_state = RunnableState::Running;
	}

	inline void Set_Finished() {
		process_state = RunnableState::Finished;
	}

	[[nodiscard]] inline kiv_os::THandle Get_Pid() const { return pid; }
	[[nodiscard]] inline kiv_os::THandle Get_Parent_Pid() const { return parent_pid; }
	[[nodiscard]] inline kiv_os::THandle Get_Std_In() const { return std_in; }
	[[nodiscard]] inline kiv_os::THandle Get_Std_Out() const { return std_out; }
	[[nodiscard]] inline std::string& Get_Working_Dir() { return working_dir; }

	[[nodiscard]] inline const std::unordered_map<kiv_os::NSignal_Id, kiv_os::TThread_Proc>& Get_Signal_Callbacks() const {
		return signal_callbacks;
	}

	[[nodiscard]] inline const std::vector<kiv_os::THandle>& Get_Process_Threads() const { return threads; }

	[[nodiscard]] inline const std::vector<kiv_os::THandle>& Get_On_Finish_Subscribers() const {
		return on_finish_subscribers;
	}

	[[nodiscard]] inline kiv_os::NOS_Error Get_Return_Code() const { return return_code; }
};
