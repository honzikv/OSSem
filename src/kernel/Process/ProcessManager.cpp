#include "ProcessManager.h"

#include "../kernel.h"
#include "../IO/IOManager.h"

size_t __stdcall Default_Signal_Callback(const kiv_hal::TRegisters& regs) {
	return 0;
}

ProcessManager& ProcessManager::Get() {
	static ProcessManager instance;
	return instance;
}

bool ProcessManager::Is_Task_Notifiable(const kiv_os::THandle task_handle) const {
	const auto handle_type = TaskIdService::Get_Handle_Type(task_handle);
	if (handle_type == HandleType::Process) {
		return process_table.count(task_handle) > 0;
	}

	return handle_type == HandleType::Thread ? thread_table.count(task_handle) > 0 : false;
}

void ProcessManager::Syscall(kiv_hal::TRegisters& regs) {
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
		return Create_Process(regs);
	}
	if (operationType == kiv_os::NClone::Create_Thread) {
		return Create_Thread(regs);
	}

	// Jinak vratime invalid argument
	return kiv_os::NOS_Error::Invalid_Argument;
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::Find_Parent_Pid() {
	const auto current_thread_id = std::this_thread::get_id();
	// Pokud nelze tid prevest vratime invalid handle
	if (std_thread_id_to_kiv_handle.count(current_thread_id) == 0) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak ziskame vlakno z tabulky a pid jeho procesu
	const auto tid = std_thread_id_to_kiv_handle[current_thread_id];
	return thread_table[tid]->Get_Pid();
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::Get_Current_Tid() {
	const auto current_thread = std::this_thread::get_id();
	return std_thread_id_to_kiv_handle.count(current_thread) == 0
		       ? kiv_os::Invalid_Handle
		       : std_thread_id_to_kiv_handle[current_thread];
}

kiv_os::NOS_Error ProcessManager::Create_Process(kiv_hal::TRegisters& regs) {
	// Ziskame jmeno programu a argumenty
	const auto program_name = reinterpret_cast<char*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto program_args = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)

	// Ziskame funkci programu, ktery bude proces vykonavat
	const auto program = reinterpret_cast<kiv_os::TThread_Proc>(GetProcAddress(User_Programs, program_name));
	// Pokud program neexistuje vratime Invalid_Argument
	if (!program) {
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// Ziskame stdio pomoci bitovych operaci
	auto std_in = static_cast<uint16_t>(regs.rbx.e >> 16 & 0xffff);
	// posuneme o 16 bitu a vymaskujeme prvnich 16 bitu (lsb)
	auto std_out = static_cast<uint16_t>(regs.rbx.e & 0xffff);
	// vymaskujeme poslednich 16 bitu (msb) a pretypujeme na uint16

	// Nastavime stdio a ulozime je do registru pro hlavni vlakno
	// Argumenty programu se kopiruji do objektu Thread a ten si je nastavi sam
	auto process_context = kiv_hal::TRegisters();
	process_context.rax.x = std_in;
	process_context.rbx.x = std_out;


	// Ziskame volny pid a tid pro proces
	const auto pid = task_id_service->Get_Free_Pid();
	const auto tid = task_id_service->Get_Free_Tid();

	// Pokud pid a tid neni vyhodime OOM
	if (pid == kiv_os::Invalid_Handle || tid == kiv_os::Invalid_Handle) {
		task_id_service->Remove_Pid(pid);
		task_id_service->Remove_Tid(tid);
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Zkusime pro dany pid pridat file descriptory
	if (IOManager::Get().Register_Process_Stdio(pid, std_in, std_out) != kiv_os::NOS_Error::Success) {
		task_id_service->Remove_Pid(pid);
		task_id_service->Remove_Tid(tid);
		return kiv_os::NOS_Error::IO_Error;
	}

	// Vytvorime vlakno procesu, kde se spusti program
	const auto main_thread = std::make_shared<Thread>(program, process_context, tid, pid, program_args);

	// Zjistime, zda-li vlakno, ve kterem se proces vytvari ma nejakeho rodice a nastavime ho (pokud existuje
	// jinak se nastavi invalid value)

	// Nyni musime locknout protoze jinak by doslo k race condition
	auto lock = std::scoped_lock(tasks_mutex);

	const auto parent_process_pid = Find_Parent_Pid();
	auto working_dir = Path(DefaultProcessWorkingDir);
	if (parent_process_pid != kiv_os::Invalid_Handle) {
		const auto parent = process_table[parent_process_pid];
		working_dir = parent->Get_Working_Dir(); // stejne tak chceme zdedit i jeho pracovni adresar
	}

	// Vytvorime proces
	const auto process = std::make_shared<Process>(pid, tid, parent_process_pid, std_in, std_out, working_dir,
	                                               program_name);


	// Pridame proces a vlakno do tabulky
	process_table[pid] = process;
	thread_table[tid] = main_thread;
	processes_running += 1;
	threads_running += 1;

	// Pridame defaultni signal handler
	process->Set_Signal_Callback(kiv_os::NSignal_Id::Terminate, Default_Signal_Callback);

	const auto command = std::string(program_name);
	Log_Debug("Creating new process for command: " + command + " with pid: "
		+ std::to_string(pid) + ", tid: "
		+ std::to_string(tid) + " and parent pid: " + std::to_string(parent_process_pid));

	// Spustime vlakno
	const auto std_thread_id = Thread::Dispatch(main_thread);

	// Namapujeme nativni tid na THandle
	std_thread_id_to_kiv_handle[std_thread_id] = tid;
	kiv_handle_to_std_thread_id[tid] = std_thread_id;

	// Spustime proces a predame pid uzivateli
	process->Set_Running();
	regs.rax.x = pid;

	// Vratime success
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::Create_Thread(kiv_hal::TRegisters& regs) {
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
	auto tid = task_id_service->Get_Free_Tid();
	if (tid == kiv_os::Invalid_Handle) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Nastavime stdin a stdout pro proces a predame je hlavnimu vlaknu
	// Argumenty programu se kopiruji do objektu Thread a ten si je nastavi sam
	auto thread_context = kiv_hal::TRegisters();
	thread_context.rax.x = std_in;
	thread_context.rbx.x = std_out;

	auto lock = std::scoped_lock(tasks_mutex);
	// Ziskame aktualni proces
	const auto current_thread = thread_table[Get_Current_Tid()];
	if (current_thread == nullptr) {
		// toto by nastat nicmene nemelo
		return kiv_os::NOS_Error::File_Not_Found;
	}

	const auto current_process = process_table[current_thread->Get_Pid()];
	if (current_process == nullptr) {
		// toto by nastat nicmene nemelo
		return kiv_os::NOS_Error::File_Not_Found;
	}

	const auto thread = std::make_shared<Thread>(program, thread_context, tid, current_process->Get_Pid(),
	                                             program_args, false);

	// Pridame vlakno do tabulky a spustime
	Log_Debug("Creating new non-main thread for process with pid: " +
		std::to_string(current_process->Get_Pid()) + " and tid: " + std::to_string(tid));
	thread_table[tid] = thread;
	current_process->AddThread(tid); // Pridame vlakno k procesu
	threads_running += 1;
	const auto std_thread_id = Thread::Dispatch(thread);

	// Namapujeme nativni tid na THandle
	std_thread_id_to_kiv_handle[std_thread_id] = tid;
	kiv_handle_to_std_thread_id[tid] = std_thread_id;
	regs.rax.x = tid; // Vratime zpet tid vlakna

	return kiv_os::NOS_Error::Success;
}


std::shared_ptr<Process> ProcessManager::Get_Current_Process() {
	auto lock = std::scoped_lock(tasks_mutex);
	const auto current_tid = Get_Current_Tid();
	const auto thread = thread_table[current_tid];
	if (thread == nullptr) {
		return nullptr;
	}

	return process_table[thread->Get_Pid()];
}

kiv_os::THandle ProcessManager::Get_Current_Pid() {
	return Get_Current_Process()->Get_Pid();
}

void ProcessManager::Run_Init_Process(kiv_os::TThread_Proc init_main) {
	const auto pid = task_id_service->Get_Free_Pid();
	const auto tid = task_id_service->Get_Free_Tid();

	const auto [std_in, std_out] = IOManager::Get().Create_Stdio();
	IOManager::Get().Register_Process_Stdio(pid, std_in, std_out);

	// Proces pro init
	auto path = Path(DefaultProcessWorkingDir);
	const auto init_process = std::make_shared<Process>(pid, tid, kiv_os::Invalid_Handle,
	                                                    std_in, std_out, path, "Init");

	// Initu predame do registru stdio
	auto init_regs = kiv_hal::TRegisters();
	init_regs.rax.x = std_in;
	init_regs.rbx.x = std_out;

	// Vlakno pro init
	const auto args = "";
	const auto init_main_thread = std::make_shared<Thread>(init_main, init_regs, tid, pid, args);

	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	// Nastavime callback pro vzbuzeni Initu do mapy s callbacky, aby fungoval shutdown
	suspend_callbacks[tid] = init_callback;

	auto std_thread_id = init_main_thread->Dispatch(init_main_thread);
	// Pridame do tabulky

	process_table[pid] = init_process;
	thread_table[tid] = init_main_thread;
	std_thread_id_to_kiv_handle[std_thread_id] = tid;
	kiv_handle_to_std_thread_id[tid] = std_thread_id;
	processes_running += 1;
	threads_running += 1;
}


void ProcessManager::Add_Current_Thread_As_Subscriber(const kiv_os::THandle* handle_array,
                                                      const uint32_t handle_array_size,
                                                      const kiv_os::THandle current_tid) {
	for (uint32_t i = 0; i < handle_array_size; i += 1) {
		const auto handle = handle_array[i];
		const auto handle_type = TaskIdService::Get_Handle_Type(handle);

		if (handle_type == HandleType::Process) {
			// Pokud se jedna o proces, tak do nej pridame subscribera do nej
			// Proces po dobehnuti vsechny subscribery probudi
			const auto process = process_table[handle];
			if (process == nullptr) { continue; }
			process->Add_Subscriber(current_tid);
			continue;
		}

		if (handle_type == HandleType::Thread) {
			// Pokud se jedna o vlakno, udelame to same, ale staci aby dobehlo vlakno
			const auto thread = thread_table[handle];
			if (thread == nullptr) { continue; }
			thread->Add_Subscriber(current_tid);
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
		current_tid = Get_Current_Tid();
		if (current_tid == kiv_os::Invalid_Handle) {
			return kiv_os::NOS_Error::Invalid_Argument;
		}

		// Pridame vlakno do vsech existujicich procesu/vlaken v handle_array
		Add_Current_Thread_As_Subscriber(handle_array, handle_count, current_tid);

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
	regs.rax.x = callback->Get_Notifier_Id();

	// Smazeme zaznam z mapy callbacku
	{
		auto lock = std::scoped_lock(suspend_callbacks_mutex);
		suspend_callbacks.erase(current_tid);
	}

	return kiv_os::NOS_Error::Success;
}

void ProcessManager::Remove_Process_From_Table(const std::shared_ptr<Process> process) {
	// Odstranime vlakna procesu
	for (const auto& tid : process->Get_Process_Threads()) {
		if (const auto thread = thread_table[tid]; thread != nullptr) {
			thread_table.erase(thread->Get_Tid());
		}
	}

	process_table.erase(process->Get_Pid());
	suspend_callbacks.erase(process->Get_Pid());
}


kiv_os::NOS_Error ProcessManager::Syscall_Read_Exit_Code(kiv_hal::TRegisters& regs) {
	// Id handlu, ktery se ma precist
	const auto handle = regs.rdx.x;

	// Zjistime, zda-li se jedna o vlakno nebo proces
	const auto handle_type = TaskIdService::Get_Handle_Type(handle);

	// Ziskame vlakno / proces, pro ktery exit code cteme a odstranime jeho zaznamy z tabulky
	std::shared_ptr<Task> task = nullptr;
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);

	Log_Debug("Reading exit code by: " + std::to_string(Get_Current_Tid()));
	if (handle_type == HandleType::Process) {
		// NOLINT(bugprone-branch-clone)
		task = process_table[handle];
		if (task != nullptr) {
			Remove_Process_From_Table(std::static_pointer_cast<Process>(task));
		}
	}
	else if (handle_type == HandleType::Thread) {
		task = thread_table[handle];
		if (task != nullptr) {
			thread_table.erase(std::static_pointer_cast<Thread>(task)->Get_Tid());
		}
	}

	// V pripade, ze se task nepodarilo najit, nebo to nebyl proces / vlakno
	if (task == nullptr) {
		regs.flags.carry = 1;
		return kiv_os::NOS_Error::File_Not_Found;
	}

	// Zjistime exit code a vratime se
	const auto exit_code = task->Get_Task_Exit_Code();
	regs.rcx.x = static_cast<decltype(regs.rcx.x)>(exit_code);
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::Syscall_Exit_Task(const kiv_hal::TRegisters& regs) {
	auto scoped_lock = std::scoped_lock(tasks_mutex);
	const auto tid = Get_Current_Tid();
	const auto exit_code = regs.rcx.x;
	// TODO terminate thread
	return kiv_os::NOS_Error::Success;
}

void ProcessManager::Terminate_Process(const kiv_os::THandle pid) {
	const auto process = process_table[pid];
	if (process == nullptr || process->Get_Task_State() == TaskState::Finished) {
		return;
	}

	// Zavolame signal
	process->Execute_Signal_Callback(kiv_os::NSignal_Id::Terminate);

	// Zavreme file descriptory procesu
	IOManager::Get().Close_Process_File_Descriptors(pid);

	process->Set_Exit_Code(ForcefullyEndedTaskExitCode);
	// process->Set_Finished();
}

kiv_os::NOS_Error ProcessManager::Syscall_Shutdown(const kiv_hal::TRegisters& regs) {
	{
		auto lock = std::scoped_lock(shutdown_mutex);
		if (shutdown_triggered) {
			return kiv_os::NOS_Error::Permission_Denied;
		}

		// Nastavime flag pro shutdown na true
		shutdown_triggered = true;
	}

	// Ziskame aktualni pid, protoze ten se ukonci po zavolani shutdownu
	const auto current_tid = Get_Current_Tid();
	const auto this_thread_pid = thread_table[current_tid]->Get_Pid();
	const auto current_process = process_table[this_thread_pid];

	// Budeme projizdet vsechny pidy a pro obsazene procesy zabijeme pomoci Terminate_Process metody
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);
	for (auto pid = TaskIdService::PidRangeStart + 1; pid < TaskIdService::PidRangeEnd; pid += 1) {
		// Pokud je proces nullptr a nebo tento nebudeme nic delat
		if (pid == this_thread_pid || process_table.count(pid) == 0) {
			continue;
		}

		// Ukoncime proces
		Terminate_Process(pid);
	}

	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::Syscall_Register_Signal_Handler(const kiv_hal::TRegisters& regs) {
	const auto signal = static_cast<kiv_os::NSignal_Id>(regs.rcx.x); // signal

	// funkce pro signal  // NOLINT(performance-no-int-to-ptr)
	kiv_os::TThread_Proc callback;
	if (regs.rdx.r == 0)
		callback = Default_Signal_Callback;
	else
		callback = reinterpret_cast<kiv_os::TThread_Proc>(regs.rdx.r);

	// zamkneme
	auto lock = std::scoped_lock(tasks_mutex);
	const auto current_tid = Get_Current_Tid();
	if (current_tid == kiv_os::Invalid_Handle) {
		return kiv_os::NOS_Error::Permission_Denied;
	}

	// Nastavime signal handler
	const auto thread = thread_table[current_tid];
	const auto process = process_table[thread->Get_Pid()];
	process->Set_Signal_Callback(signal, callback);
	return kiv_os::NOS_Error::Success;
}

void ProcessManager::Initialize_Suspend_Callback(const kiv_os::THandle subscriber_handle) {
	if (suspend_callbacks.count(subscriber_handle) == 0 || suspend_callbacks[subscriber_handle] == nullptr) {
		suspend_callbacks[subscriber_handle] = std::make_shared<SuspendCallback>();
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ProcessManager::Trigger_Suspend_Callback(const kiv_os::THandle subscriber_handle,
                                              const kiv_os::THandle notifier_handle) {
	if (suspend_callbacks.count(subscriber_handle) == 0 || suspend_callbacks[subscriber_handle] == nullptr) {
		return; // Pokud callback neni v mape vratime se
	}

	// Jinak ho zkusime zavolat
	const auto callback = suspend_callbacks[subscriber_handle];
	// Notify nic nedela pokud se callback uz jednou zavolal
	callback->Notify(notifier_handle);
}

void ProcessManager::On_Process_Finish(const kiv_os::THandle pid, const uint16_t main_thread_exit_code,
                                       const bool triggered_by_main_thread) {
	// Nic nelockujeme, tato funkce zavola pouze z metody OnThreadFinish, ktera mutexy uz lockne
	const auto process = process_table[pid];

	// Pokud je process null nebo uz se OnProcessFinish zavolal vratime se
	if (process == nullptr || process->Get_Task_State() == TaskState::Finished) {
		return;
	}

	// Ukoncime ostatni vlakna "nasilim"
	// Pro vlakno, ktere se stihne ukoncit vcas se provede OnThreadFinish a tento kod nic neudela
	for (const auto tid : process->Get_Process_Threads()) {
		const auto thread = thread_table[tid];
		if (thread == nullptr || thread->Is_Main_Thread() && triggered_by_main_thread) {
			// hlavni vlakno nemuzeme vypnout, protoze vola tuto metodu
			continue;
		}

		thread->Terminate_If_Running(ForcefullyEndedTaskExitCode);

		// Notifikujeme subscribery vlakna
		thread->Notify_Subscribers(thread->Get_Tid());

		// Nastavime exit code na readable
		thread->Set_Finished();
		threads_running -= 1;
	}

	// Zavreme file descriptory procesu
	IOManager::Get().Close_Process_File_Descriptors(pid);

	// IOManager::Get().Unregister_Process_Stdio(pid, process->Get_Std_in(), process->Get_Std_Out());

	// Provedeme notifikaci cekajich objektu na tento proces
	process->Notify_Subscribers(process->Get_Pid());

	// Nastavime exit code
	process->Set_Exit_Code(main_thread_exit_code);

	// Zmenime stav na readable exit code
	process->Set_Finished();
	Log_Debug("On Process Finish pid: " + std::to_string(pid));

	processes_running -= 1;
	if (processes_running <= 0 && threads_running <= 0) {
		shutdown_semaphore->Release();
	}
}

void ProcessManager::On_Thread_Finish(const kiv_os::THandle tid, uint16_t thread_exit_code) {
	auto lock = std::scoped_lock(tasks_mutex, suspend_callbacks_mutex);

	// Ziskame vlakno
	const auto thread = thread_table[tid];

	// Pokud je vlakno null nebo uz bylo zavolano on finish (coz pozname podle stavu) vratime se
	if (thread == nullptr) {
		return;
	}

	// Jinak provedeme notifikaci cekajicich objektu na toto vlakno
	thread->Notify_Subscribers(thread->Get_Tid());

	// Nastavime exit code
	thread->Set_Exit_Code(thread_exit_code);

	// Nastavime stav na finished
	thread->Set_Finished();

	// Snizime pocet bezicich vlaken
	threads_running -= 1;

	// Pokud je vlakno main provedeme OnProcessFinish
	if (thread->Is_Main_Thread()) {
		On_Process_Finish(thread->Get_Pid(), thread->Get_Task_Exit_Code(), true);
	}

	if (processes_running <= 0 && threads_running <= 0) {
		// V tento moment dobehli vsechny vlakna, takze muzeme odemknout semafor
		shutdown_semaphore->Release();
	}
}

void ProcessManager::On_Shutdown() const {
	// Pockame, dokud se vsechny procesy nedokonci
	shutdown_semaphore->Acquire();
}

std::shared_ptr<ProcessTableSnapshot> ProcessManager::Get_Process_Table_Snapshot() {
	auto lock = std::scoped_lock(tasks_mutex);

	auto procfs_vec = std::vector<std::shared_ptr<ProcFSRow>>();
	for (uint16_t pid = TaskIdService::PidRangeStart; pid < TaskIdService::PidRangeEnd; pid += 1) {
		if (process_table[pid] == nullptr) {
			continue;
		}

		procfs_vec.push_back(ProcFSRow::Map_Process_To_ProcFSRow(*process_table[pid]));
	}

	return std::make_shared<ProcessTableSnapshot>(procfs_vec);
}
