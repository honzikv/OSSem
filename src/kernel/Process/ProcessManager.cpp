#include "ProcessManager.h"

#include <csignal>

#include "Init.h"
#include "../kernel.h"
#include "../IO/IOManager.h"

size_t DefaultSignalCallback(const kiv_hal::TRegisters& regs) {
	const auto signal_val = static_cast<int>(regs.rcx.r);
	LogDebug("Default callback call performed");
	return 0;
}

kiv_os::THandle ProcessManager::GetFreePid() const {
	for (uint16_t i = 0; i < static_cast<uint16_t>(process_table.size()); i += 1) {
		if (process_table[i] == nullptr) {
			return i + PID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

kiv_os::THandle ProcessManager::GetFreeTid() const {
	for (uint16_t i = 0; i < static_cast<uint16_t>(thread_table.size()); i += 1) {
		if (thread_table[i] == nullptr) {
			return i + TID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

std::shared_ptr<Process> ProcessManager::GetProcess(const kiv_os::THandle pid) {
	if (pid >= PID_RANGE_START && pid < PID_RANGE_END) {
		return process_table[pid - PID_RANGE_START];
	}
	return nullptr;
}

void ProcessManager::AddProcess(const std::shared_ptr<Process> process, const kiv_os::THandle pid) {
	process_table[pid] = process;
}

std::shared_ptr<Thread> ProcessManager::GetThread(const kiv_os::THandle tid) {
	if (tid >= TID_RANGE_START && tid < TID_RANGE_END) {
		return thread_table[tid - TID_RANGE_START];
	}
	return nullptr;
}

auto ProcessManager::AddThread(const std::shared_ptr<Thread> thread, const kiv_os::THandle tid) -> void {
	thread_table[tid - TID_RANGE_START] = thread;
}

bool ProcessManager::TaskNotifiable(const kiv_os::THandle task_handle) {
	const auto handle_type = GetHandleType(task_handle);
	if (handle_type == HandleType::Process) {
		return GetProcess(task_handle) != nullptr;
	}

	return handle_type == HandleType::Thread ? GetThread(task_handle) != nullptr : false;
}

void ProcessManager::ProcessSyscall(kiv_hal::TRegisters& regs) {
	// Spustime jednotlive funkce podle operace
	auto operation = regs.rax.l;
	auto operation_result = kiv_os::NOS_Error::Success;
	switch (static_cast<kiv_os::NOS_Process>(operation)) {
		case kiv_os::NOS_Process::Clone: {
			operation_result = PerformClone(regs);
			break;
		}

		case kiv_os::NOS_Process::Wait_For: {
			operation_result = PerformWaitFor(regs);
			break;
		}

		case kiv_os::NOS_Process::Read_Exit_Code: {
			operation_result = PerformReadExitCode(regs);
			break;
		}

		case kiv_os::NOS_Process::Exit: {
			operation_result = ExitTask(regs);
			break;
		}

		case kiv_os::NOS_Process::Shutdown: {
			operation_result = PerformShutdown(regs);
			break;
		}

		case kiv_os::NOS_Process::Register_Signal_Handler: {
			operation_result = PerformRegisterSignalHandler(regs);
			break;
		}
		default: {
			break;
		}
	}

	// Pokud nastane chyba nastavime carry bit na 1 a chybu zapiseme do registru
	if (operation_result != kiv_os::NOS_Error::Success) {
		regs.flags.carry = 1;
		regs.rax.x = static_cast<decltype(regs.rax.x)>(operation_result);
	}
}

kiv_os::NOS_Error ProcessManager::PerformClone(kiv_hal::TRegisters& regs) {
	const auto operationType = static_cast<kiv_os::NClone>(regs.rcx.l);
	if (operationType == kiv_os::NClone::Create_Process) {
		return CreateNewProcess(regs);
	}
	if (operationType == kiv_os::NClone::Create_Thread) {
		return CreateNewThread(regs);
	}

	// Jinak vratime invalid argument
	return kiv_os::NOS_Error::Invalid_Argument;
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::FindParentPid() {
	const auto current_thread_handle = GetCurrentThreadId();
	const auto lock = std::scoped_lock(tasks_mutex);
	// Pokud nelze tid prevest vratime invalid handle
	if (thread_id_to_kiv_handle.count(current_thread_handle) == 0) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak ziskame vlakno z tabulky a pid jeho procesu
	const auto tid = thread_id_to_kiv_handle[current_thread_handle];
	return thread_table[tid - TID_RANGE_START]->GetPid();
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::GetCurrentTid() {
	const auto current_thread = GetCurrentThreadId();
	const auto lock = std::scoped_lock(tasks_mutex);
	return thread_id_to_kiv_handle.count(current_thread) == 0
		       ? kiv_os::Invalid_Handle
		       : thread_id_to_kiv_handle[current_thread];
}

HANDLE ProcessManager::GetNativeThreadHandle(const kiv_os::THandle tid) {
	auto lock = std::scoped_lock(tasks_mutex);
	const auto native_thread_id = kiv_handle_to_native_thread_id[tid];
	return native_thread_id_to_native_handle[native_thread_id];
}


kiv_os::NOS_Error ProcessManager::CreateNewProcess(kiv_hal::TRegisters& regs) {
	// Ziskame jmeno programu a argumenty
	const auto program_name = reinterpret_cast<char*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto program_args = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)

	// Ziskame funkci s programem a pretypujeme ji na TThread_Proc
	const auto program = reinterpret_cast<kiv_os::TThread_Proc>(GetProcAddress(User_Programs, program_name));
	// Pokud program neexistuje vratime Invalid_Argument
	if (!program) {
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// bx.e = (stdin << 16) | stdout
	auto std_in = static_cast<uint16_t>(regs.rbx.e >> 16 & 0xffff);
	// posuneme o 16 bitu a vymaskujeme prvnich 16 lsb
	auto std_out = static_cast<uint16_t>(regs.rbx.e & 0xffff);
	// vymaskujeme prvnich 16 msb a pretypujeme na uint16

	// Provedeme "registraci" stdio pro proces - toto zvysi pocet referenci na dany soubor
	// Jinak by se mohlo pri zavirani handlu stat, ze proces 
	// Pokud nelze stdin a stdout najit vyhodime chybu
	if (IOManager::Get().RegisterProcessStdIO(std_in, std_out) != kiv_os::NOS_Error::Success) {
		return kiv_os::NOS_Error::IO_Error;
	}
	
	// Nastavime stdin a stdout pro proces a predame je hlavnimu vlaknu
	// Argumenty programu se kopiruji do objektu Thread a ten si je nastavi sam
	auto process_context = kiv_hal::TRegisters();
	process_context.rax.x = std_in;
	process_context.rbx.x = std_out;

	// Vytvorime zamek, protoze ziskame pid a tid, ktere nam nesmi nikdo sebrat
	auto lock = std::scoped_lock(tasks_mutex);
	// Nejprve provedeme check zda-li muzeme vytvorit novy proces - musi existovat volny pid a tid
	const auto pid = GetFreePid();
	const auto tid = GetFreeTid();

	if (pid == NO_FREE_ID || tid == NO_FREE_ID) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Vytvorime vlakno procesu, kde se spusti program
	const auto main_thread = std::make_shared<Thread>(program, process_context, tid, pid, program_args);

	// Zjistime, zda-li vlakno, ve kterem se proces vytvari ma nejakeho rodice a nastavime ho (pokud existuje
	// jinak se nastavi invalid value)
	const auto parent_process_pid = FindParentPid();
	auto working_dir = DEFAULT_PROCESS_WORKING_DIR;
	if (parent_process_pid != kiv_os:: Invalid_Handle) {
		const auto parent = GetProcess(parent_process_pid);
		working_dir = parent->GetWorkingDir();
	}
	const auto process = std::make_shared<Process>(pid, tid, parent_process_pid, std_in, std_out, working_dir);

	// Pridame proces a vlakno do tabulky
	AddProcess(process, pid);
	AddThread(main_thread, tid);

	// Pridame defaultni signal handler
	process->SetSignalCallback(kiv_os::NSignal_Id::Terminate, DefaultSignalCallback);

	const auto command = std::string(program_name);
	LogDebug("Creating new process for command: " + command + " with pid: "
		+ std::to_string(pid) + ", tid: "
		+ std::to_string(tid) + " and parent pid: " + std::to_string(parent_process_pid));

	// Spustime vlakno
	const auto [native_handle, native_id] = main_thread->Dispatch();

	// Namapujeme nativni tid na THandle
	thread_id_to_kiv_handle[native_id] = tid;
	kiv_handle_to_native_thread_id[tid] = native_id;
	native_thread_id_to_native_handle[native_id] = native_handle;

	// Spustime proces a predame pid uzivateli
	process->SetRunning();
	regs.rax.x = pid;

	// Vratime success
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::CreateNewThread(kiv_hal::TRegisters& regs) {
	// Ziskame jmeno programu a argumenty
	const auto program_name = reinterpret_cast<char*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto program_args = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)

	// Ziskame funkci s programem a pretypujeme ji na TThread_Proc
	const auto program = reinterpret_cast<kiv_os::TThread_Proc>(GetProcAddress(User_Programs, program_name));
	// Pokud program neexistuje vratime Invalid_Argument
	if (!program) {
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// bx.e = (stdin << 16) | stdout
	const auto std_out = static_cast<uint16_t>(regs.rbx.e & 0xffff);
	// vymaskujeme prvnich 16 msb a pretypujeme na uint16
	const auto std_in = static_cast<uint16_t>(regs.rbx.e >> 16 & 0xffff);
	// posuneme o 16 bitu a vymaskujeme prvnich 16 lsb

	// Vlakno nezajima registrace stdin a stdout protoze handly ani nezavira - to dela proces

	// Vytvorime zamek, protoze ziskame tid, ktery by nikdo nemel sebrat
	auto lock = std::scoped_lock(tasks_mutex);
	auto tid = GetFreeTid();
	if (tid == NO_FREE_ID) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Nastavime stdin a stdout pro proces a predame je hlavnimu vlaknu
	// Argumenty programu se kopiruji do objektu Thread a ten si je nastavi sam
	auto thread_context = kiv_hal::TRegisters();
	thread_context.rax.x = std_in;
	thread_context.rbx.x = std_out;

	// Ziskame aktualni proces
	const auto current_thread = GetThread(GetCurrentTid());
	if (current_thread == nullptr) {
		// toto by nastat nicmene nemelo
		return kiv_os::NOS_Error::File_Not_Found;
	}

	const auto current_process = GetProcess(current_thread->GetPid());
	if (current_process == nullptr) {
		// toto by nastat nicmene nemelo
		return kiv_os::NOS_Error::File_Not_Found;
	}

	const auto thread = std::make_shared<Thread>(program, thread_context, tid, current_process->GetPid(),
	                                             program_args, false);

	// Pridame vlakno do tabulky a spustime
	LogDebug("Creating new non-main thread for process with pid: " +
		std::to_string(current_process->GetPid()) + " and tid: " + std::to_string(tid));
	AddThread(thread, tid);
	current_process->AddThread(tid); // Pridame vlakno k procesu
	const auto [native_handle, native_tid] = thread->Dispatch();

	// Namapujeme nativni tid na THandle
	thread_id_to_kiv_handle[native_tid] = tid;
	kiv_handle_to_native_thread_id[tid] = native_tid;
	native_thread_id_to_native_handle[native_tid] = native_handle;
	regs.rax.x = tid; // Vratime zpet tid vlakna
	
	return kiv_os::NOS_Error::Success;
}

void ProcessManager::TriggerSuspendCallback(const kiv_os::THandle subscriber_handle,
                                            const kiv_os::THandle notifier_handle) {
	auto suspend_callbacks_lock = std::scoped_lock(suspend_callbacks_mutex);
	if (suspend_callbacks.count(subscriber_handle) == 0) {
		return;
	}

	const auto callback = suspend_callbacks[subscriber_handle];
	auto tasks_lock = std::scoped_lock(tasks_mutex);
	if (callback != nullptr && TaskNotifiable(subscriber_handle)) {
		callback->Notify(notifier_handle);
	}
	LogDebug("Handle: " + std::to_string(notifier_handle) + " notified: " + std::to_string(subscriber_handle));
}

void ProcessManager::NotifyProcessFinished(const kiv_os::THandle pid, uint16_t exit_code) {
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	TerminateProcess(pid, false, exit_code);
}


void ProcessManager::NotifyThreadFinished(const kiv_os::THandle tid) {
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	TerminateThread(tid, false);
}

void ProcessManager::TerminateProcess(const kiv_os::THandle pid, const bool terminated_forcefully,
                                      const uint16_t thread_exit_code) {
	LogDebug("Terminating process with pid: " + std::to_string(pid) + " from thread: " + std::to_string(GetCurrentTid()));
	std::shared_ptr<Process> process;
	{
		process = GetProcess(pid);
		if (process == nullptr || process->GetState() == TaskState::Finished || process->GetState() == TaskState::Terminated) {
			return;
		}
		// Pokud byl proces ukoncen nasilim, pak muze bezet i jeho hlavni vlakno
		// tim padem musime vsechna vlakna ukoncit nasilim. V opacnem pripade ukoncime vsechny krome mainu "nasilim"
		for (size_t i = terminated_forcefully ? 0 : 1; i < process->GetProcessThreads().size(); i += 1) {
			TerminateThread(process->GetProcessThreads()[i], true, ForcefullyEndedTaskExitCode);
		}
	}

	// Zavolame vsechny vlakna / procesy cekajici na ukonceni procesu
	process->NotifySubscribers(process->GetPid(), terminated_forcefully);
	process->SetExitCode(terminated_forcefully ? -1 : thread_exit_code);

	if (terminated_forcefully) {
		// Zavolame callback pro signal
		process->ExecuteCallback(kiv_os::NSignal_Id::Terminate);
	}

	// Rekneme IOManageru aby odstranil referenci na handle z naseho procesu
	IOManager::Get().UnregisterProcessStdIO(process->GetStdIn(), process->GetStdOut());
}

void ProcessManager::TerminateThread(const kiv_os::THandle tid, const bool terminated_forcefully,
                                     const size_t exit_code) {
	// Zde lock neni potreba, protoze tato metoda se da zavolat pouze z NotifyThreadFinished a nebo NotifyThreadFinished
	const auto thread = GetThread(tid);
	if (thread == nullptr || thread->GetState() == TaskState::Finished || thread->GetState() == TaskState::Terminated) {
		return;
	}

	// Notifikujeme cekajici tasky na toto vlakno o tom, ze uz dobehlo
	thread->NotifySubscribers(thread->GetTid(), terminated_forcefully);

	// Pokud se vlakno neukoncilo samo ukoncime ho a nastavime mu patricny exit code
	thread->TerminateIfRunning(GetNativeThreadHandle(thread->GetTid()), exit_code);
}

HandleType ProcessManager::GetHandleType(const kiv_os::THandle id) {
	if (id >= PID_RANGE_START && id < PID_RANGE_END) {
		// Pokud je handle mezi 0 - PID_RANGE_END jedna se o proces
		return HandleType::Process;
	}
	// Jinak musi byt handle mezi TID_RANGE_START a TID_RANGE_END
	return id >= TID_RANGE_START && id < TID_RANGE_END ? HandleType::Thread : HandleType::INVALID;
}

void ProcessManager::AddCurrentThreadAsSubscriber(const kiv_os::THandle* handle_array,
                                                  const uint32_t handle_array_size,
                                                  const kiv_os::THandle current_tid) {
	// Pro validaci musime locknout tabulku
	// Validujeme protoze se muze stat, ze procesy mezitim co jsme provadeli metodu
	// dobehli, a byly odstranene z tabulek
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	for (uint32_t i = 0; i < handle_array_size; i += 1) {
		const auto handle = handle_array[i];
		const auto handleType = GetHandleType(handle);

		if (handleType == HandleType::Process) {
			// Pokud se jedna o proces, tak do nej pridame subscribera do nej
			// Proces po dobehnuti vsechny subscribery probudi
			const auto process = GetProcess(handle);
			if (process == nullptr) { continue; }
			process->AddSubscriber(current_tid);
			continue;
		}

		if (handleType == HandleType::Thread) {
			// Pokud se jedna o vlakno, udelame to same, ale staci aby dobehlo vlakno
			const auto thread = GetThread(handle);
			if (thread == nullptr) { continue; }
			thread->AddSubscriber(current_tid);
		}
		// Pokud je hodnota HandleType::Invalid, nic nedelame
	}
}

kiv_os::NOS_Error ProcessManager::PerformWaitFor(kiv_hal::TRegisters& regs) {
	const auto handle_array = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto handle_count = regs.rcx.e;

	kiv_os::THandle current_tid;

	// Thread id aktualne beziciho vlakna
	{
		auto lock = std::scoped_lock(tasks_mutex);
		current_tid = GetCurrentTid();
	}
	if (current_tid == kiv_os::Invalid_Handle) {
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// Pridame vlakno do vsech existujicich procesu/vlaken v handle_array
	AddCurrentThreadAsSubscriber(handle_array, handle_count, current_tid);

	// Nastavime vychozi hodnotu na invalid value. Pokud se na neco opravdu cekalo prenastavi se to
	regs.rax.x = static_cast<decltype(regs.rax.x)>(kiv_os::Invalid_Handle);
	std::shared_ptr<SuspendCallback> callback; // nullptr

	// Musime locknout aby nedoslo k race condition
	{
		auto lock = std::scoped_lock(suspend_callbacks_mutex);
		callback = suspend_callbacks[current_tid]; // ziskame callback
	}

	// Muze se stat, ze vsechny prvky z pole uz se ukoncili pred tim, nez se inicializovalo wait for
	// (pripadne, ze se zadali spatne hodnoty)
	if (callback == nullptr) {
		return kiv_os::NOS_Error::Success;
	}

	// Pokud se callback jeste nespustil uspime vlakno
	if (!callback->Triggered()) {
		callback->Suspend();
	}

	// Ziskame id vlakna/procesu, ktery toto vlakno vzbudil
	regs.rax.x = callback->GetNotifierId();

	// Smazeme zaznam z mapy callbacku
	RemoveSuspendCallback(current_tid);

	return kiv_os::NOS_Error::Success;
}

void ProcessManager::RemoveProcessFromTable(const std::shared_ptr<Process> process) {
	// Odstranime vlakna procesu
	for (const auto& tid : process->GetProcessThreads()) {
		if (const auto thread = GetThread(tid); thread != nullptr) {
			RemoveThreadFromTable(thread);
		}
	}
	process_table[process->GetPid() - PID_RANGE_START] = nullptr; // odstranime proces z tabulky
	suspend_callbacks.erase(process->GetPid());
}

void ProcessManager::RemoveThreadFromTable(const std::shared_ptr<Thread> thread) {
	const auto tid = thread->GetTid();
	// Odstranime vlakno
	thread_table[tid - TID_RANGE_START] = nullptr;
	const auto native_tid = kiv_handle_to_native_thread_id[tid];
	kiv_handle_to_native_thread_id.erase(tid);
	thread_id_to_kiv_handle.erase(native_tid);
	native_thread_id_to_native_handle.erase(native_tid);
	suspend_callbacks.erase(thread->GetTid());
}

kiv_os::NOS_Error ProcessManager::PerformReadExitCode(kiv_hal::TRegisters& regs) {
	// Id handlu, ktery se ma precist
	const auto handle = regs.rdx.x;

	// Zjistime, zda-li se jedna o vlakno nebo proces
	const auto handle_type = GetHandleType(handle);

	// Ziskame vlakno / proces, pro ktery exit code cteme a odstranime jeho zaznamy z tabulky
	std::shared_ptr<Task> task = nullptr;
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	LogDebug("Reading exit code by: " + std::to_string(GetCurrentTid()));
	if (handle_type == HandleType::Process) {
		// NOLINT(bugprone-branch-clone)
		task = GetProcess(handle);
		if (task != nullptr) {
			RemoveProcessFromTable(std::static_pointer_cast<Process>(task)); // pretypovani Task na Process shared ptr
		}
	}
	else if (handle_type == HandleType::Thread) {
		task = GetThread(handle);
		if (task != nullptr) {
			RemoveThreadFromTable(std::static_pointer_cast<Thread>(task)); // pretypovani Task na Thread shared ptr
		}
	}

	// V pripade, ze se task nepodarilo najit, nebo to nebyl proces / vlakno
	if (task == nullptr) {
		regs.flags.carry = 1;
		regs.rcx.r = static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found);
		return kiv_os::NOS_Error::File_Not_Found;
	}

	// Zjistime exit code a vratime se
	const auto exit_code = task->GetExitCode();
	regs.rcx.x = static_cast<decltype(regs.rcx.x)>(exit_code);
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::ExitTask(const kiv_hal::TRegisters& regs) {
	auto scoped_lock = std::scoped_lock(tasks_mutex);
	const auto tid = GetCurrentTid();
	const auto exit_code = regs.rcx.x;
	TerminateThread(tid, true, exit_code);
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::PerformShutdown(const kiv_hal::TRegisters& regs) {
	// Nastavime flag pro shutdown - diky tomu bude init proces vedet zda-li ma ukoncit zbyle procesy
	shutdown_triggered = { true };

	auto scoped_lock = std::scoped_lock(tasks_mutex);
	// Vzbudime Init a ten vypne zbyle procesy
	shutdown_callback->Notify(kiv_os::Invalid_Handle);

	return kiv_os::NOS_Error::Success;
}


kiv_os::NOS_Error ProcessManager::PerformRegisterSignalHandler(const kiv_hal::TRegisters& regs) {
	const auto signal = static_cast<kiv_os::NSignal_Id>(regs.rcx.x); // signal

	const auto callback = regs.rdx.r == 0 ? DefaultSignalCallback : reinterpret_cast<kiv_os::TThread_Proc>(regs.rdx.r); // funkce pro signal  // NOLINT(performance-no-int-to-ptr)

	// zamkneme
	auto lock = std::scoped_lock(tasks_mutex);
	const auto current_tid = GetCurrentTid();
	if (current_tid == kiv_os::Invalid_Handle) {
		return kiv_os::NOS_Error::Permission_Denied;
	}
	
	// Nastavime signal handler
	const auto thread = GetThread(current_tid);
	const auto process = GetProcess(thread->GetPid());
	process->SetSignalCallback(signal, callback);
	return kiv_os::NOS_Error::Success;
}

void ProcessManager::TerminateProcesses(const kiv_os::THandle this_thread_pid) {
	LogDebug("Shutdown performed from tid: " + std::to_string(GetCurrentTid()));
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	for (auto pid = PID_RANGE_START + 1; pid < PID_RANGE_END; pid += 1) {
		// Init proces se terminuje sam a ma vzdy id 0, takze ukoncujeme od pid = 1 a pid, ktery spustil ukonceni se ukonci sam
		if (const auto process = GetProcess(pid); pid != this_thread_pid && process != nullptr) {
			TerminateProcess(pid, true, -1);
			RemoveProcessFromTable(process);
		}
	}

}

void ProcessManager::RunInitProcess(kiv_os::TThread_Proc program) {
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	constexpr auto pid = PID_RANGE_START; // Pid Init procesu bude vzdy na zacatku tabulky (index 0)
	const auto tid = GetFreeTid(); // Tid vezmeme jakykoliv

	// Vytvorime stdio
	const auto [std_in, std_out] = IOManager::Get().CreateStdIO();

	// Vytvorime registry, ktere se predaji vlaknu
	auto init_thread_regs = kiv_hal::TRegisters();

	// Nastavime stdio a predame ho do registru
	init_thread_regs.rax.x = std_in;
	init_thread_regs.rbx.x = std_out;
	init_thread_regs.rcx.x = pid; // navic budeme potrebovat pid pro ukonceni procesu

	// Vytvorime novy proces. Parent procesu bude invalid handle, protoze init proces nema rodice
	const auto process = std::make_shared<InitProcess>(pid, tid, kiv_os::Invalid_Handle, std_in,
	                                                   std_out, DEFAULT_PROCESS_WORKING_DIR);
	// Nastavime funkci, ktera se ma spustit
	const auto args = ""; // zadne argumenty nejsou potreba
	const auto thread = std::make_shared<Thread>(program, init_thread_regs, tid, pid, args);

	// Potrebujeme nejakym zpusobem notifikovat init proces ze se zavolal shutdown
	// Pro to pouzijeme callback stejnym zpusobem jako u wait for
	const auto init_suspend_callback = std::make_shared<SuspendCallback>();

	// Callback musi byt nastaveny tid, protoze vlakno ceka na proces/vlakno
	suspend_callbacks[tid] = init_suspend_callback;

	// Pro shutdown si jeste explicitne ulozime shared pointer na tento callback abysme ho nemuseli v mape
	// slozite hledat
	shutdown_callback = init_suspend_callback;

	
	auto [native_handle, native_tid] = thread->Dispatch(); // spusteni procesu
	// Pridame do tabulky
	AddProcess(process, pid);
	AddThread(thread, tid);

	// Nastavime si mapping z Windows handlu a tidu na interni kiv_os handle
	thread_id_to_kiv_handle[native_tid] = tid;
	kiv_handle_to_native_thread_id[tid] = native_tid;
	native_thread_id_to_native_handle[native_tid] = native_handle;
}


void ProcessManager::InitializeSuspendCallback(const kiv_os::THandle subscriber_handle) {
	if (suspend_callbacks.count(subscriber_handle) == 0 || suspend_callbacks[subscriber_handle] == nullptr) {
		suspend_callbacks[subscriber_handle] = std::make_shared<SuspendCallback>();
	}
}

void ProcessManager::RemoveSuspendCallback(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(suspend_callbacks_mutex);
	if (suspend_callbacks.count(subscriber_handle) > 0) {
		suspend_callbacks.erase(subscriber_handle);
	}
}
