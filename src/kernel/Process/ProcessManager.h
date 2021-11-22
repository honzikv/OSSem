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
#include <winnt.h>

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

	static constexpr uint16_t PID_RANGE_START = 0;
	static constexpr uint16_t PID_RANGE_END = 4096;
	static constexpr uint16_t TID_RANGE_START = 4096;
	static constexpr uint16_t TID_RANGE_END = 8192;
	static constexpr uint16_t NO_FREE_ID = -1;

	/// <summary>
	/// Vychozi pracovni adresar, pokud neni zadny specifikovany
	/// </summary>
	inline static const auto DEFAULT_PROCESS_WORKING_DIR =  Path("C:\\");

	/// <summary>
	/// Singleton ziskani objektu. Provede lazy inicializaci a vrati referenci
	/// </summary>
	/// <returns>Referenci na singleton instanci teto tridy</returns>
	static ProcessManager& Get();

private:
	/// <summary>
	/// Zda-li se provedl shutdown - aby ho nemohlo provest vice prikazu najednou
	/// </summary>
	std::atomic<bool> shutdown_triggered = { false };

	/// <summary>
	/// Callback pro probuzeni init procesu na shutdown
	/// </summary>
	std::shared_ptr<SuspendCallback> init_callback;

	/// <summary>
	/// Tabulka procesu
	/// </summary>
	std::array<std::shared_ptr<Process>, PID_RANGE_END - PID_RANGE_START> process_table = {};

	/// <summary>
	/// Tabulka vlaken
	/// </summary>
	std::array<std::shared_ptr<Thread>, TID_RANGE_END - TID_RANGE_START> thread_table = {};
	
	/// <summary>
	/// Handle -> Tid
	///	Pro mazani
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::thread::id> kiv_handle_to_std_thread_id = {};

	/// <summary>
	/// Thread id -> Handle pro ukonceni vlakna
	/// </summary>
	std::unordered_map<std::thread::id, kiv_os::THandle> std_thread_id_to_kiv_handle = {};

	/// <summary>
	/// Hashmapa s callbacky pro probouzeni vlaken
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::shared_ptr<SuspendCallback>> suspend_callbacks = {};

	/// <summary>
	/// Ziska prvni volny pid, pokud neni vrati NO_FREE_ID
	/// </summary>
	/// <returns>Volny pid, nebo NO_FREE_ID, pokud zadny neni</returns>
	[[nodiscard]] kiv_os::THandle GetFreePid() const;

	/// <summary>
	/// Ziska prvni volny tid, pokud neni vrati NO_FREE_ID
	/// </summary>
	/// <returns>Volny tid, nebo NO_FREE_ID, pokud zadny neni</returns>
	[[nodiscard]] kiv_os::THandle GetFreeTid() const;

	/// <summary>
	/// Getter pro proces podle pidu
	/// </summary>
	/// <param name="pid">pid</param>
	/// <returns>Pointer na proces</returns>
	std::shared_ptr<Process> GetProcess(kiv_os::THandle pid);

	/// <summary>
	/// Getter pro vlakno procesu podle tidu
	/// </summary>
	/// <param name="tid">tid</param>
	/// <returns>Pointer na vlakno</returns>
	std::shared_ptr<Thread> GetThread(kiv_os::THandle tid);

	/// <summary>
	/// Prida proces do tabulky
	/// </summary>
	/// <param name="process">Pointer na proces</param>
	/// <param name="pid">pid procesu</param>
	void AddProcess(std::shared_ptr<Process> process, kiv_os::THandle pid);

	/// <summary>
	/// Prida vlakno do tabulky
	/// </summary>
	/// <param name="thread">Pointer na vlakno</param>
	/// <param name="tid">tid vlakna</param>
	void AddThread(std::shared_ptr<Thread> thread, kiv_os::THandle tid);

	/// <summary>
	/// Zjisti, zda-li task lze notifikovat
	/// </summary>
	/// <param name="task_handle"></param>
	/// <returns></returns>
	bool TaskNotifiable(kiv_os::THandle task_handle);

	/// <summary>
	/// Mutex pro synchronizaci kriticke sekce
	/// </summary>
	std::recursive_mutex mutex = {};

	ProcessManager() = default;
	~ProcessManager() = default;
	ProcessManager(const ProcessManager&) = delete; // NOLINT(modernize-use-equals-delete)
	ProcessManager& operator=(const ProcessManager&) = delete; // NOLINT(modernize-use-equals-delete)

	/// <summary>
	/// Najde PID rodice pro vlakno, ze ktereho se metoda zavolala
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle FindParentPid();

	/// <summary>
	/// Ziska THandle pro aktualne bezici vlakno. Bezici vlakno musi byt umistene v tabulce, jinak
	///	metoda vyhodi runtime exception
	/// </summary>
	/// <returns>Tid aktualne beziciho vlakna</returns>
	kiv_os::THandle GetCurrentTid();

	/// <summary>
	/// Pocet bezicich procesu
	/// </summary>
	size_t running_processes = 0;

	/// <summary>
	/// Pocet bezicich vlaken
	/// </summary>
	size_t running_threads = 0;

public:
	/// <summary>
	/// Zpracuje pozadavek
	/// </summary>
	/// <param name="regs">registry</param>
	/// <returns>vysledek operace</returns>
	void ProcessSyscall(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Spusti
	/// </summary>
	/// <param name="subscriber_handle"></param>
	/// <param name="notifier_handle"></param>
	void TriggerSuspendCallback(kiv_os::THandle subscriber_handle, kiv_os::THandle notifier_handle);

	/// <summary>
	/// Vytvori callback pro vzbuzeni vlakna (pokud neexistuje)
	/// </summary>
	/// <param name="subscriber_handle">tid vlakna, ktere se ma vzbudit</param>
	void CreateSuspendCallbackIfNotExists(kiv_os::THandle subscriber_handle);

	/// <summary>
	/// Tuto metodu pouziva hlavni vlakno po uspesnem skonceni sveho lifecyclu - ukonci tim svuj proces
	/// </summary>
	/// <param name="pid">Pid procesu, ktery se ma ukoncit</param>
	/// <param name="exit_code">Exit code procesu</param>
	void OnProcessFinished(kiv_os::THandle pid, uint16_t exit_code);

	/// <summary>
	/// Tuto metodu pouziva vlakno, aby oznamilo process manageru, ze skoncilo
	/// </summary>
	void OnThreadFinished(kiv_os::THandle tid);


private:
	/// <summary>
	/// Spusti init proces
	/// </summary>
	/// <param name="init_main">odkaz na funkci s mainem pro init proces</param>
	void RunInitProcess(kiv_os::TThread_Proc init_main);

	/// <summary>
	/// Ukonci proces
	/// </summary>
	/// <param name="pid"></param>
	/// <param name="terminated_forcefully">Zda-li se proces ukoncil nasilim</param>
	/// <param name="thread_exit_code">Exit code hlavniho vlakna</param>
	void TerminateProcess(kiv_os::THandle pid, bool terminated_forcefully = false, uint16_t thread_exit_code = 0);

	/// <summary>
	/// Ukonci vlakno
	/// </summary>
	/// <param name="tid"></param>
	/// <param name="terminated_forcefully"></param>
	/// <param name="exit_code">Exit code vlakna</param>
	void TerminateThread(kiv_os::THandle tid, bool terminated_forcefully = false, size_t exit_code = 0);

	/// <summary>
	/// Provede klonovani procesu nebo vlakna
	/// </summary>
	/// <param name="regs">kontext</param>
	/// <returns>Success pokud vse probehlo v poradku, jinak chybu</returns>
	kiv_os::NOS_Error PerformClone(kiv_hal::TRegisters& regs);


	/// <summary>
	/// Vytvori proces s hlavnim vlaknem a prida je do tabulek
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error CreateNewProcess(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vytvori nove vlakno pro aktualne bezici proces
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns>Vysledek operace</returns>
	kiv_os::NOS_Error CreateNewThread(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Vrati typ handle pro  dane id
	/// </summary>
	/// <param name="id">id, pro ktere chceme typ handle dostat</param>
	/// <returns>typ handle</returns>
	static HandleType GetHandleType(kiv_os::THandle id);

	/// <summary>
	/// Prida proces pro ostatni handles (proces/vlakno) jako subscribera, ktereho musi handles po dokonceni vzbudit
	/// </summary>
	/// <param name="handle_array">pointer na pole handlu</param>
	/// <param name="handle_array_size">velikost pole</param>
	/// <param name="current_tid">tid aktualniho vlakna</param>
	void AddCurrentThreadAsSubscriber(const kiv_os::THandle* handle_array, uint32_t handle_array_size,
	                                  kiv_os::THandle current_tid);

	/// <summary>
	/// Provede NOS_Process::Wait_For
	/// </summary>
	/// <param name="regs">kontext</param>
	/// <returns>vysledek operace</returns>
	kiv_os::NOS_Error PerformWaitFor(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Odstrani proces z tabulky
	/// </summary>
	/// <param name="process">Reference na proces</param>
	void RemoveProcessFromTable(std::shared_ptr<Process> process);

	/// <summary>
	/// Odstrani vlakno z tabulky a lookup map
	/// </summary>
	/// <param name="thread">Reference na vlakno</param>
	void RemoveThreadFromTable(std::shared_ptr<Thread> thread);

	/// <summary>
	/// Provede cteni exit codu
	/// </summary>
	/// <param name="regs">Registry pro zapsani vysledku</param>
	/// <returns></returns>
	kiv_os::NOS_Error PerformReadExitCode(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error ExitTask(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformShutdown(const kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error PerformRegisterSignalHandler(const kiv_hal::TRegisters& regs);

	/// <summary>
	/// Odesle signal
	/// </summary>
	/// <param name="signal"></param>
	void SendSignal(kiv_os::NSignal_Id signal, std::shared_ptr<Process> process);
	
};
