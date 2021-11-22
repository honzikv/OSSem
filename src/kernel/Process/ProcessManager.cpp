#include "ProcessManager.h"

#include <csignal>

#include "InitProcess.h"
#include "../kernel.h"
#include "../IO/IOManager.h"

size_t DefaultSignalCallback(const kiv_hal::TRegisters& regs) {
	const auto signal_val = static_cast<int>(regs.rcx.r);
	LogDebug("Default callback call performed");
	return 0;
}

ProcessManager& ProcessManager::Get() {
	static ProcessManager instance;
	return instance;
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
			operation_result = Syscall_Clone(regs);
			break;
		}

		case kiv_os::NOS_Process::Wait_For: {
			operation_result = Syscall_Wait_For(regs);
			break;
		}

		case kiv_os::NOS_Process::Read_Exit_Code: {
			operation_result = Syscall_Read_Exit_Code(regs);
			break;
		}

		case kiv_os::NOS_Process::Exit: {
			operation_result = Syscall_Exit_Task(regs);
			break;
		}

		case kiv_os::NOS_Process::Shutdown: {
			operation_result = Syscall_Shutdown(regs);
			break;
		}

		case kiv_os::NOS_Process::Register_Signal_Handler: {
			operation_result = Syscall_Register_Signal_Handler(regs);
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

kiv_os::NOS_Error ProcessManager::Syscall_Clone(kiv_hal::TRegisters& regs) {
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
	const auto current_thread_handle = std::this_thread::get_id();
	// Pokud nelze tid prevest vratime invalid handle
	if (std_thread_id_to_kiv_handle.count(current_thread_handle) == 0) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak ziskame vlakno z tabulky a pid jeho procesu
	const auto tid = std_thread_id_to_kiv_handle[current_thread_handle];
	return thread_table[tid - TID_RANGE_START]->GetPid();
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::GetCurrentTid() {
	const auto current_thread = std::this_thread::get_id();
	return std_thread_id_to_kiv_handle.count(current_thread) == 0
		       ? kiv_os::Invalid_Handle
		       : std_thread_id_to_kiv_handle[current_thread];
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

	// Provedeme "registraci" stdio pro proces - toto zvysi pocet referenci na dany soubor
	// Jinak by se mohlo pri zavirani handlu stat, ze proces nekomu jinemu zavre soubory
	if (IOManager::Get().Register_Process_Stdio(pid, std_in, std_out) != kiv_os::NOS_Error::Success) {
		return kiv_os::NOS_Error::IO_Error;
	}

	// Vytvorime vlakno procesu, kde se spusti program
	const auto main_thread = std::make_shared<Thread>(program, process_context, tid, pid, program_args);

	// Zjistime, zda-li vlakno, ve kterem se proces vytvari ma nejakeho rodice a nastavime ho (pokud existuje
	// jinak se nastavi invalid value)
	const auto parent_process_pid = FindParentPid();
	auto working_dir = Path(DEFAULT_PROCESS_WORKING_DIR);
	if (parent_process_pid != kiv_os::Invalid_Handle) {
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
	const auto std_thread_id = main_thread->Dispatch();

	// Namapujeme nativni tid na THandle
	std_thread_id_to_kiv_handle[std_thread_id] = tid;
	kiv_handle_to_std_thread_id[tid] = std_thread_id;

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

	const auto thread = std::make_shared<Thread>(program, thread_context, tid, current_process->Get_Pid(),
	                                             program_args, false);

	// Pridame vlakno do tabulky a spustime
	LogDebug("Creating new non-main thread for process with pid: " +
		std::to_string(current_process->Get_Pid()) + " and tid: " + std::to_string(tid));
	AddThread(thread, tid);
	current_process->AddThread(tid); // Pridame vlakno k procesu
	const auto std_thread_id = thread->Dispatch();

	// Namapujeme nativni tid na THandle
	std_thread_id_to_kiv_handle[std_thread_id] = tid;
	kiv_handle_to_std_thread_id[tid] = std_thread_id;
	regs.rax.x = tid; // Vratime zpet tid vlakna

	return kiv_os::NOS_Error::Success;
}


std::shared_ptr<Process> ProcessManager::GetCurrentProcess() {
	auto lock = std::scoped_lock(tasks_mutex);
	const auto current_tid = GetCurrentTid();
	const auto thread = GetThread(current_tid);

	return GetProcess(thread->GetPid());
}

void ProcessManager::RunInitProcess(kiv_os::TThread_Proc init_main) {
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	const auto pid = GetFreePid(); // Init process ma vzdy 0
	const auto tid = GetFreeTid();

	const auto [std_in, std_out] = IOManager::Get().Create_Stdio();
	IOManager::Get().Register_Process_Stdio(pid, std_in, std_out);

	// Proces pro init
	auto path = Path(DEFAULT_PROCESS_WORKING_DIR);
	const auto init_process = std::make_shared<InitProcess>(pid, tid, kiv_os::Invalid_Handle, std_in, std_out, path);

	// Initu predame do registru stdio
	auto init_regs = kiv_hal::TRegisters();
	init_regs.rax.x = std_in;
	init_regs.rbx.x = std_out;

	// Vlakno pro init
	const auto args = "";
	const auto init_main_thread = std::make_shared<Thread>(init_main, init_regs, tid, pid, args);

	// Nastavime callback pro vzbuzeni Initu do mapy s callbacky, aby fungoval shutdown
	suspend_callbacks[tid] = init_callback;

	auto std_thread_id = init_main_thread->Dispatch();
	// Pridame do tabulky
	AddProcess(init_process, pid);
	AddThread(init_main_thread, tid);
	std_thread_id_to_kiv_handle[std_thread_id] = tid;
	kiv_handle_to_std_thread_id[tid] = std_thread_id;
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
	for (uint32_t i = 0; i < handle_array_size; i += 1) {
		const auto handle = handle_array[i];
		const auto handle_type = GetHandleType(handle);

		if (handle_type == HandleType::Process) {
			// Pokud se jedna o proces, tak do nej pridame subscribera do nej
			// Proces po dobehnuti vsechny subscribery probudi
			const auto process = GetProcess(handle);
			if (process == nullptr) { continue; }
			process->AddSubscriber(current_tid);
			continue;
		}

		if (handle_type == HandleType::Thread) {
			// Pokud se jedna o vlakno, udelame to same, ale staci aby dobehlo vlakno
			const auto thread = GetThread(handle);
			if (thread == nullptr) { continue; }
			thread->AddSubscriber(current_tid);
		}
		// Pokud je hodnota HandleType::Invalid, nic nedelame
	}
}

kiv_os::NOS_Error ProcessManager::Syscall_Wait_For(kiv_hal::TRegisters& regs) {
	const auto handle_array = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto handle_count = regs.rcx.e;

	kiv_os::THandle current_tid;
	std::shared_ptr<SuspendCallback> callback;
	{
		auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
		current_tid = GetCurrentTid();
		if (current_tid == kiv_os::Invalid_Handle) {
			return kiv_os::NOS_Error::Invalid_Argument;
		}

		// Pridame vlakno do vsech existujicich procesu/vlaken v handle_array
		AddCurrentThreadAsSubscriber(handle_array, handle_count, current_tid);

		// Nastavime vychozi hodnotu na invalid value, pokud se na neco opravdu cekalo prenastavime
		regs.rax.x = static_cast<decltype(regs.rax.x)>(kiv_os::Invalid_Handle);
		callback = suspend_callbacks[current_tid]; // ziskame callback
	}

	// Muze se stat, ze vsechny prvky z pole uz se ukoncili pred tim, nez se inicializovalo wait for
	// (pripadne, ze se zadali spatne hodnoty)
	if (callback == nullptr) {
		return kiv_os::NOS_Error::Success;
	}

	// Uspime se - pokud neco callback spustilo, tato operace nebude blokovat, jinak cekame dokud nas neco nevzbudi
	callback->Suspend();

	// Ziskame id vlakna/procesu, ktery toto vlakno vzbudil
	regs.rax.x = callback->GetNotifierId();

	// Smazeme zaznam z mapy callbacku
	{
		auto lock = std::scoped_lock(suspend_callbacks_mutex);
		suspend_callbacks.erase(current_tid);
	}

	return kiv_os::NOS_Error::Success;
}

void ProcessManager::RemoveProcessFromTable(const std::shared_ptr<Process> process) {
	// Odstranime vlakna procesu
	for (const auto& tid : process->GetProcessThreads()) {
		if (const auto thread = GetThread(tid); thread != nullptr) {
			RemoveThreadFromTable(thread);
		}
	}
	process_table[process->Get_Pid() - PID_RANGE_START] = nullptr; // odstranime proces z tabulky
	suspend_callbacks.erase(process->Get_Pid());
}

void ProcessManager::RemoveThreadFromTable(const std::shared_ptr<Thread> thread) {
	const auto tid = thread->GetTid();
	// Odstranime vlakno
	thread_table[tid - TID_RANGE_START] = nullptr;
	const auto native_tid = kiv_handle_to_std_thread_id[tid];

	// Smazeme mapping na windows handle a tid
	kiv_handle_to_std_thread_id.erase(tid);
	std_thread_id_to_kiv_handle.erase(native_tid);

	// Smazeme callback
	suspend_callbacks.erase(thread->GetTid());
}

kiv_os::NOS_Error ProcessManager::Syscall_Read_Exit_Code(kiv_hal::TRegisters& regs) {
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
	const auto exit_code = task->GetTaskExitCode();
	regs.rcx.x = static_cast<decltype(regs.rcx.x)>(exit_code);
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::Syscall_Exit_Task(const kiv_hal::TRegisters& regs) {
	auto scoped_lock = std::scoped_lock(tasks_mutex);
	const auto tid = GetCurrentTid();
	const auto exit_code = regs.rcx.x;
	// TODO terminate thread
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::Syscall_Shutdown(const kiv_hal::TRegisters& regs) {
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex, shutdown_mutex);
	if (shutdown_triggered) {
		return kiv_os::NOS_Error::Permission_Denied;
	}

	// Nastavime flag pro shutdown na true
	shutdown_triggered = true;

	// Ziskame aktualni pid, protoze ten se ukonci po zavolani shutdownu
	const auto current_tid = GetCurrentTid();
	const auto this_thread_pid = GetThread(current_tid)->GetPid();

	// Budeme projizdet vsechny pidy a pro obsazene procesy zabijeme pomoci Terminate_Process metody
	for (auto pid = PID_RANGE_START + 1; pid < PID_RANGE_END; pid += 1) {
		// Pokud je proces nullptr a nebo tento nebudeme nic delat
		if (pid == this_thread_pid || GetProcess(pid) == nullptr) {
			continue;
		}

		// Ukoncime proces signalem terminate
		Terminate_Process(pid);
	}

	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::Syscall_Register_Signal_Handler(const kiv_hal::TRegisters& regs) {
	const auto signal = static_cast<kiv_os::NSignal_Id>(regs.rcx.x); // signal
	
	// funkce pro signal  // NOLINT(performance-no-int-to-ptr)
	const auto callback = regs.rdx.r == 0 ? DefaultSignalCallback : reinterpret_cast<kiv_os::TThread_Proc>(regs.rdx.r);

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

void ProcessManager::InitializeSuspendCallback(const kiv_os::THandle subscriber_handle) {
	if (suspend_callbacks.count(subscriber_handle) == 0 || suspend_callbacks[subscriber_handle] == nullptr) {
		suspend_callbacks[subscriber_handle] = std::make_shared<SuspendCallback>();
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ProcessManager::TriggerSuspendCallback(const kiv_os::THandle subscriber_handle, const kiv_os::THandle notifier_handle) {
	if (suspend_callbacks.count(subscriber_handle) == 0 || suspend_callbacks[subscriber_handle] == nullptr) {
		return; // Pokud callback neni v mape vratime se
	}

	// Jinak ho zkusime zavolat
	const auto callback = suspend_callbacks[subscriber_handle];
	// Notify nic nedela pokud se callback uz jednou zavolal
	callback->Notify(notifier_handle);
}

void ProcessManager::Terminate_Process(const kiv_os::THandle pid) {
	// Nic nelockujeme, tato funkce se muze zavolat pouze z shutdownu
	const auto process = GetProcess(pid);

	// Pokud je proces null nebo uz se proces terminoval nic nedelame
	if (process == nullptr || process->GetTaskState() == TaskState::ReadableExitCode) {
		return;
	}

	// V logice OS jsme proces ukoncili pomoci signalu, takze signal provedeme
	process->ExecuteCallback(kiv_os::NSignal_Id::Terminate);
	IOManager::Get().UnregisterProcessStdIO(pid, process->GetStdIn(), process->GetStdOut());

	// process->NotifySubscribers(pid);
}



void ProcessManager::On_Process_Finish(const kiv_os::THandle pid, const uint16_t main_thread_exit_code) {
	// Nic nelockujeme, tato funkce zavola pouze z metody OnThreadFinish, ktera mutexy uz lockne
	const auto process = GetProcess(pid);

	// Pokud je process null nebo uz se OnProcessFinish zavolal vratime se
	if (process == nullptr || process->GetTaskState() == TaskState::ReadableExitCode) {
		return;
	}

	// Ukoncime ostatni non-main vlakna "nasilim"
	// Pro vlakno, ktere se stihne ukoncit vcas se provede OnThreadFinish a tento kod nic neudela
	for (const auto tid : process->GetProcessThreads()) {
		const auto thread = GetThread(tid);
		if (thread == nullptr || thread->IsMainThread()) {
			// hlavni vlakno nemuzeme vypnout, protoze vola tuto metodu
			continue;
		}

		thread->TerminateIfRunning(ForcefullyEndedTaskExitCode);

		// Notifikujeme subscribery vlakna
		thread->NotifySubscribers(thread->GetTid());

		// Nastavime exit code na readable
		thread->SetReadableExitCode();
	}

	// TODO zavreme vsechny otevrene soubory pro proces
	// TODO - zatim uzavreme pouze stdio
	IOManager::Get().UnregisterProcessStdIO(pid, process->GetStdIn(), process->GetStdOut());

	// Provedeme notifikaci cekajich objektu na tento proces
	process->NotifySubscribers(process->Get_Pid());

	// Nastavime exit code
	process->SetExitCode(main_thread_exit_code);

	// Zmenime stav na readable exit code
	process->SetReadableExitCode();
	LogDebug("On Process Finish pid: " + std::to_string(pid));
}

void ProcessManager::OnThreadFinish(const kiv_os::THandle tid) {
	// Protoze manipulujeme s vlaknem strukturu zavreme
	auto tasks_lock = std::scoped_lock(tasks_mutex);

	// Ziskame vlakno
	const auto thread = GetThread(tid);

	// Pokud je vlakno null nebo uz bylo zavolano on finish (coz pozname podle stavu) vratime se
	if (thread == nullptr || thread->GetState() == TaskState::ReadableExitCode) {
		return;
	}

	// Jinak provedeme notifikaci cekajicich objektu na toto vlakno
	auto callbacks_lock = std::scoped_lock(suspend_callbacks_mutex);
	thread->NotifySubscribers(thread->GetTid());

	// Exit code se nastavil sam, staci zmenit stav na readable exit code
	thread->SetReadableExitCode();

	const auto was_main = thread->IsMainThread() ? " thread was main" : "";
	LogDebug("Thread with tid: " + std::to_string(tid) + " and pid: " + std::to_string(thread->GetPid()) + " finished"
		+ was_main);
	// Pokud je vlakno main provedeme OnProcessFinish
	if (thread->IsMainThread()) {
		On_Process_Finish(thread->GetPid(), thread->GetTaskExitCode());
	}
}

void ProcessManager::OnShutdown() {
	// Pouze pockame, dokud nedostaneme pristup k mutexum aby bylo jasne, ze se vsechno ukoncilo korektne
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex, shutdown_mutex);
}
