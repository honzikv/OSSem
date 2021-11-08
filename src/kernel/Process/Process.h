#pragma once
#include <mutex>
#include <string>
#include <unordered_map>

#include "Task.h"
#include "../api/api.h"

/// <summary>
/// Reprezentuje proces (resp. PCB)
/// </summary>
class Process : public Task {

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

public:
	Process(kiv_os::THandle pid, kiv_os::THandle main_thread_tid, kiv_os::THandle parent_pid,
	        kiv_os::THandle std_in, kiv_os::THandle std_out);

	/// <summary>
	/// Prida vlakno do procesu
	/// </summary>
	/// <param name="tid">tid vlakna</param>
	void Add_Thread(kiv_os::THandle tid);

	[[nodiscard]] inline kiv_os::THandle Get_Pid() const { return pid; }
	[[nodiscard]] inline kiv_os::THandle Get_Parent_Pid() const { return parent_pid; }
	[[nodiscard]] inline kiv_os::THandle Get_Std_In() const { return std_in; }
	[[nodiscard]] inline kiv_os::THandle Get_Std_Out() const { return std_out; }
	[[nodiscard]] inline std::string& Get_Working_Dir() { return working_dir; }

	[[nodiscard]] inline const std::unordered_map<kiv_os::NSignal_Id, kiv_os::TThread_Proc>& Get_Signal_Callbacks() const {
		return signal_callbacks;
	}

	[[nodiscard]] inline const std::vector<kiv_os::THandle>& Get_Process_Threads() const { return threads; }
	

};
