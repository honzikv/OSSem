#pragma once
#include <memory>
#include <array>
#include <string>
#include <mutex>

#include "Thread.h"
#include "Process.h"
#include "SuspendCallback.h"
#include "../../api/api.h"
#include "../Utils/Logging.h"
#include "../IO/ProcessTableSnapshot.h"

/// <summary>
/// Typ handle pro wait for funkcionalitu
/// </summary>
enum class HandleType {
	INVALID,
	Thread,
	Process
};

static constexpr uint16_t ForcefullyEndedTaskExitCode = -1;

/// <summary>
/// Trida, ktera se stara o spravu procesu a vlaken
/// </summary>
class ProcessManager {

	// Init proces ma pristup ke vsem fieldum a metodam aby mohl ukoncit procesy
	friend class InitProcess;
	// NOLINT(cppcoreguidelines-special-member-functions)
public:
	// Konstanty pro rozsahy pidu a tidu

	static constexpr uint16_t PidRangeStart = 0;
	static constexpr uint16_t PidRangeEnd = 4096;
	static constexpr uint16_t TidRangeStart = 4096;
	static constexpr uint16_t TidRangeEnd = 8192;
	static constexpr uint16_t NoFreeId = -1;

	/// <summary>
	/// Vychozi pracovni adresar, pokud neni zadny 
	/// </summary>
	inline static const std::string DefaultProcessWorkingDir = "C:\\";

	/// <summary>
	/// Singleton ziskani objektu. Provede lazy inicializaci a vrati referenci
	/// </summary>
	/// <returns>Referenci na singleton instanci teto tridy</returns>
	static ProcessManager& Get();

private:
	/// <summary>
	/// Flag pro shutdown
	/// </summary>
	bool shutdown_triggered = false;

	/// <summary>
	/// Mutex pro synchronizaci pri shutdownu
	/// </summary>
	std::mutex shutdown_mutex = {};

	std::shared_ptr<Semaphore> shutdown_semaphore = std::make_shared<Semaphore>();

	int32_t processes_running = 0;
	int32_t threads_running = 0;

	/// <summary>
	/// Callback pro probuzeni Init procesu pro ukonceni OS
	/// </summary>
	std::shared_ptr<SuspendCallback> init_callback = std::make_shared<SuspendCallback>();

	/// <summary>
	/// Tabulka procesu
	/// </summary>
	std::array<std::shared_ptr<Process>, PidRangeEnd - PidRangeStart> process_table = {};

	/// <summary>
	/// Tabulka vlaken
	/// </summary>
	std::array<std::shared_ptr<Thread>, TidRangeEnd - TidRangeStart> thread_table = {};

	/// <summary>
	/// TID -> Handle
	/// </summary>
	std::unordered_map<std::thread::id, kiv_os::THandle> std_thread_id_to_kiv_handle = {};

	/// <summary>
	/// Handle -> Tid
	///	Pro mazani
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::thread::id> kiv_handle_to_std_thread_id = {};

	/// <summary>
	/// Hashmapa s callbacky pro probouzeni vlaken
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::shared_ptr<SuspendCallback>> suspend_callbacks = {};

	/// <summary>
	/// Ziska prvni volny pid, pokud neni vrati NO_FREE_ID
	/// </summary>
	/// <returns>Volny pid, nebo NO_FREE_ID, pokud zadny neni</returns>
	[[nodiscard]] kiv_os::THandle Get_Free_Pid() const;

	/// <summary>
	/// Ziska prvni volny tid, pokud neni vrati NO_FREE_ID
	/// </summary>
	/// <returns>Volny tid, nebo NO_FREE_ID, pokud zadny neni</returns>
	[[nodiscard]] kiv_os::THandle Get_Free_Tid() const;

	/// <summary>
	/// Getter pro proces podle pidu
	/// </summary>
	/// <param name="pid">pid</param>
	/// <returns>Pointer na proces</returns>
	std::shared_ptr<Process> Get_Process(kiv_os::THandle pid);

	/// <summary>
	/// Prida proces do tabulky
	/// </summary>
	/// <param name="process">Pointer na proces</param>
	/// <param name="pid">pid procesu</param>
	void Add_Process(std::shared_ptr<Process> process, kiv_os::THandle pid);

	/// <summary>
	/// Prida vlakno do tabulky
	/// </summary>
	/// <param name="thread">Pointer na vlakno</param>
	/// <param name="tid">tid vlakna</param>
	void Add_Thread(std::shared_ptr<Thread> thread, kiv_os::THandle tid);

	/// <summary>
	/// Zjisti, zda-li task existuje
	/// </summary>
	/// <param name="task_handle"></param>
	/// <returns></returns>
	bool Is_Task_Notifiable(kiv_os::THandle task_handle);
	

	/// <summary>
	/// Mutex pro callbacky pro vzbuzeni
	/// </summary>
	std::mutex suspend_callbacks_mutex;

	/// <summary>
	/// Mutex pro tasky
	/// </summary>
	std::mutex tasks_mutex;

	ProcessManager() = default;
	~ProcessManager() = default;
	ProcessManager(const ProcessManager&) = delete; // NOLINT(modernize-use-equals-delete)
	ProcessManager& operator=(const ProcessManager&) = delete; // NOLINT(modernize-use-equals-delete)

	/// <summary>
	/// Najde PID rodice pro dane vlakno
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle Find_Parent_Pid();

	/// <summary>
	/// Ziska THandle pro aktualne bezici vlakno. Bezici vlakno musi byt umistene v tabulce, jinak
	///	metoda vyhodi runtime exception
	/// </summary>
	/// <returns>Tid aktualne beziciho vlakna</returns>
	kiv_os::THandle Get_Current_Tid();

public:
	/// <summary>
	/// Zpracuje pozadavek
	/// </summary>
	/// <param name="regs">registry</param>
	/// <returns>vysledek operace</returns>
	void Syscall(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Spusti
	/// </summary>
	/// <param name="subscriber_handle"></param>
	/// <param name="notifier_handle"></param>
	void Trigger_Suspend_Callback(kiv_os::THandle subscriber_handle, kiv_os::THandle notifier_handle);

	/// <summary>
	/// Vytvori callback pro vzbuzeni vlakna (pokud neexistuje)
	/// </summary>
	/// <param name="subscriber_handle">tid vlakna, ktere se ma vzbudit</param>
	void Initialize_Suspend_Callback(kiv_os::THandle subscriber_handle);

	/// <summary>
	/// Vrati aktualne bezici proces
	/// </summary>
	std::shared_ptr<Process> Get_Current_Process();

	/// <summary>
	/// Vrati aktualni pid
	/// </summary>
	kiv_os::THandle Get_Current_Pid();

	/// <summary>
	/// Vrati vsechny aktualne pouzivane pidy
	/// </summary>
	void Get_All_Process_Pids(std::vector<kiv_os::THandle> pid_list);

private:
	/// <summary>
	/// Spusti init proces
	/// </summary>
	/// <param name="init_main">odkaz na funkci s mainem pro init proces</param>
	void Run_Init_Process(kiv_os::TThread_Proc init_main);

	/// <summary>
	/// Provede klonovani procesu nebo vlakna
	/// </summary>
	/// <param name="regs">kontext</param>
	/// <returns>Success pokud vse probehlo v poradku, jinak chybu</returns>
	kiv_os::NOS_Error Syscall_Clone(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Getter pro vlakno procesu podle tidu
	/// </summary>
	/// <param name="tid">tid</param>
	/// <returns>Pointer na vlakno</returns>
	std::shared_ptr<Thread> Get_Thread(kiv_os::THandle tid);

	/// <summary>
	/// Vytvori proces s hlavnim vlaknem a prida je do tabulek
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Create_Process(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vytvori nove vlakno pro aktualne bezici proces
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error Create_Thread(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vrati typ handle pro  dane id
	/// </summary>
	/// <param name="id">id, pro ktere chceme typ handle dostat</param>
	/// <returns>typ handle</returns>
	static HandleType Get_Handle_Type(kiv_os::THandle id);

	/// <summary>
	/// Prida proces pro ostatni handles (proces/vlakno) jako subscribera, ktereho musi handles po dokonceni vzbudit
	/// </summary>
	/// <param name="handle_array">pointer na pole handlu</param>
	/// <param name="handle_array_size">velikost pole</param>
	/// <param name="current_tid">tid aktualniho vlakna</param>
	void Add_Current_Thread_As_Subscriber(const kiv_os::THandle* handle_array, uint32_t handle_array_size,
	                                  kiv_os::THandle current_tid);

	/// <summary>
	/// Provede NOS_Process::Wait_For
	/// </summary>
	/// <param name="regs">kontext</param>
	/// <returns>vysledek operace</returns>
	kiv_os::NOS_Error Syscall_Wait_For(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Odstrani proces z tabulky
	/// </summary>
	/// <param name="process">Reference na proces</param>
	void Remove_Process_From_Table(std::shared_ptr<Process> process);

	/// <summary>
	/// Odstrani vlakno z tabulky a lookup map
	/// </summary>
	/// <param name="thread">Reference na vlakno</param>
	void Remove_Thread_From_Table(std::shared_ptr<Thread> thread);

	/// <summary>
	/// Provede cteni exit codu
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns></returns>
	kiv_os::NOS_Error Syscall_Read_Exit_Code(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Exit_Task(const kiv_hal::TRegisters& regs);
	
	void Terminate_Process(kiv_os::THandle pid);

	kiv_os::NOS_Error Syscall_Shutdown(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error Syscall_Register_Signal_Handler(const kiv_hal::TRegisters& regs);

	/// <summary>
	///
	///	<param name="pid">pid</param>
	///	<param name="main_thread_exit_code">exit code procesu</param>
	///	<param name="triggered_by_main_thread">zda-li zavolalo metodu hlavni vlakno</param>
	/// </summary>
	void On_Process_Finish(kiv_os::THandle pid, uint16_t main_thread_exit_code, bool triggered_by_main_thread);

public:
	/// <summary>
	/// Dokonceni zivotniho cyklu vlakna - provede zavolani metod a dokonceni
	/// </summary>
	void On_Thread_Finish(kiv_os::THandle tid);

	/// <summary>
	/// Synchronizace main vlakna
	/// </summary>
	void On_Shutdown() const;

	/// <summary>
	/// Vrati aktualni snapshot tabulky procesu pro procfs
	/// </summary>
	std::shared_ptr<ProcessTableSnapshot> Get_Process_Table_Snapshot();
};
