#pragma once
#include <string>
#include <unordered_map>

#include "Task.h"
#include "../../api/api.h"
#include "../Fat12/path.h"

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
	Path working_dir;

	/// <summary>
	/// Mapping signalu a callbacku, ktery se provede po zavolani
	/// </summary>
	std::unordered_map<kiv_os::NSignal_Id, kiv_os::TThread_Proc> signal_callbacks = {};

	/// <summary>
	/// Jmeno programu
	/// </summary>
	std::string program_name;

	/// <summary>
	/// Seznam vlaken, ktere patri tomuto procesu. Vlakno na 0tem indexu je main
	/// </summary>
	std::vector<kiv_os::THandle> threads = {};

public:
	Process(kiv_os::THandle pid, kiv_os::THandle main_thread_tid, kiv_os::THandle parent_pid,
		kiv_os::THandle std_in, kiv_os::THandle std_out, Path& working_dir, std::string program_name);

	/// <summary>
	/// Prida vlakno do procesu
	/// </summary>
	/// <param name="tid">tid vlakna</param>
	void AddThread(kiv_os::THandle tid);

	// Gettery
	[[nodiscard]] kiv_os::THandle Get_Pid() const;
	[[nodiscard]] kiv_os::THandle Get_Parent_Pid() const;
	[[nodiscard]] kiv_os::THandle Get_Std_in() const;
	[[nodiscard]] kiv_os::THandle Get_Std_Out() const;
	[[nodiscard]] Path& Get_Working_Dir();

	/// <summary>
	/// Nastavi pracovni adresar pro proces
	/// </summary>
	/// <param name="path">cesta</param>
	void Set_Working_Dir(Path& path);

	[[nodiscard]] const std::unordered_map<kiv_os::NSignal_Id, kiv_os::TThread_Proc>& Get_Signal_Callbacks() {
		return signal_callbacks;
	}

	/// <summary>
	/// Nastavi pro dany signal dany callback
	/// </summary>
	/// <param name="signal">Signal, ktery ma callback spoustet</param>
	/// <param name="callback">Callback, ktery se ma provest</param>
	void Set_Signal_Callback(kiv_os::NSignal_Id signal, kiv_os::TThread_Proc callback);

	[[nodiscard]] inline const std::vector<kiv_os::THandle>& Get_Process_Threads() const { return threads; }

	/// <summary>
	/// Vrati, zda-li existuje pro signal dany callback
	/// </summary>
	/// <param name="signal_number"></param>
	/// <returns></returns>
	[[nodiscard]] bool Has_Signal_Callback(int signal_number) const;

	/// <summary>
	/// Provede callback na dany signal
	/// </summary>
	/// <param name="signal_id">Cislo signalu</param>
	void Execute_Signal_Callback(kiv_os::NSignal_Id signal_id);

	[[nodiscard]] const std::string& Get_Program_Name() const;

};
