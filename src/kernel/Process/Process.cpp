#include "Process.h"
#include "ProcessManager.h"


Process::Process(const kiv_os::THandle pid, const kiv_os::THandle main_thread_tid, const kiv_os::THandle parent_pid,
                 const kiv_os::THandle std_in, const kiv_os::THandle std_out, Path& working_dir, std::string program_name):
	pid(pid), parent_pid(parent_pid), std_in(std_in),
	std_out(std_out), working_dir(std::move(working_dir)), program_name(std::move(program_name)) {

	threads.push_back(main_thread_tid);
}

void Process::AddThread(const kiv_os::THandle tid) {
	threads.push_back(tid);
}

kiv_os::THandle Process::Get_Pid() const { return pid; }

kiv_os::THandle Process::Get_Parent_Pid() const { return parent_pid; }

kiv_os::THandle Process::Get_Std_in() const { return std_in; }

kiv_os::THandle Process::Get_Std_Out() const { return std_out; }

Path& Process::Get_Working_Dir() { return working_dir; }

void Process::Set_Working_Dir(Path& path) {
	auto lock = std::scoped_lock(subscribers_mutex);
	working_dir = std::move(path);
}


void Process::Set_Signal_Callback(const kiv_os::NSignal_Id signal, const kiv_os::TThread_Proc callback) {
	signal_callbacks[signal] = callback;
}

bool Process::Has_Signal_Callback(int signal_number) const {
	return signal_callbacks.count(static_cast<kiv_os::NSignal_Id>(signal_number)) != 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Process::Execute_Signal_Callback(kiv_os::NSignal_Id signal_id) {
	Log_Debug("Executing signal callback");
	auto regs = kiv_hal::TRegisters();
	regs.rcx.r = static_cast<decltype(regs.rcx.r)>(signal_id);
	signal_callbacks[signal_id](regs);
}
