#pragma once
#include <memory>
#include <array>
#include <string>
#include <mutex>

#include "handles.h"
#include "Thread.h"
#include "Process.h"
#include "SuspendCallback.h"
#include "../api/api.h"
#include "Utils/Logging.h"

/// <summary>
/// Typ handle pro wait for funkcionalitu
/// </summary>
enum class HandleType {
	INVALID,
	THREAD,
	PROCESS
};

/// <summary>
/// Trida, ktera se stara o spravu procesu a vlaken
/// </summary>
class ProcessManager {
public:
	// Konstanty pro rozsahy pidu a tidu

	static constexpr uint16_t PID_RANGE_START = 0;
	static constexpr uint16_t PID_RANGE_END = 4096;
	static constexpr uint16_t TID_RANGE_START = 4096;
	static constexpr uint16_t TID_RANGE_END = 8192;
	static constexpr uint16_t NO_FREE_ID = -1;

	/// <summary>
	/// Vychozi pracovni adresar, pokud neni zadny 
	/// </summary>
	inline static const std::string DEFAULT_PROCESS_WORKDIR = "C:\\";

	/// <summary>
	/// Singleton ziskani objektu. Provede lazy inicializaci a vrati referenci
	/// </summary>
	/// <returns>Referenci na singleton instanci teto tridy</returns>
	static ProcessManager& Get() {
		static ProcessManager instance;
		return instance;
	}

private:
	/// <summary>
	/// Tabulka procesu
	/// </summary>
	std::array<std::shared_ptr<Process>, PID_RANGE_END - PID_RANGE_START> process_table = {};

	/// <summary>
	/// Tabulka vlaken
	/// </summary>
	std::array<std::shared_ptr<Thread>, TID_RANGE_END - TID_RANGE_START> thread_table = {};

	/// <summary>
	/// TID -> Handle
	/// </summary>
	std::unordered_map<std::thread::id, kiv_os::THandle> native_tid_to_kiv_handle = {};

	/// <summary>
	/// Handle -> Tid
	///	Pro mazani
	/// </summary>
	std::unordered_map<kiv_os::THandle, std::thread::id> kiv_handle_to_native_tid = {};

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
	/// Slouzi k zamykani kriticke sekce pro procesy a vlakna (tabulky)
	/// </summary>
	std::recursive_mutex tasks_mutex;

	/// <summary>
	/// Mutex pro callbacky pro vzbuzeni
	/// </summary>
	std::mutex suspend_callbacks_mutex;

	ProcessManager() = default;
	~ProcessManager() = default;
	ProcessManager(const ProcessManager&) = delete; // NOLINT(modernize-use-equals-delete)
	ProcessManager& operator=(const ProcessManager&) = delete; // NOLINT(modernize-use-equals-delete)

	/// <summary>
	/// Najde PID rodice pro dane vlakno
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle FindParentPid();

	/// <summary>
	/// Ziska THandle pro aktualne bezici vlakno. Bezici vlakno musi byt umistene v tabulce, jinak
	///	metoda vyhodi runtime exception
	/// </summary>
	/// <returns>Tid aktualne beziciho vlakna</returns>
	kiv_os::THandle GetCurrentThreadTid();

public:
	/// <summary>
	/// Zpracuje pozadavek
	/// </summary>
	/// <param name="regs">registry</param>
	/// <returns>vysledek operace</returns>
	kiv_os::NOS_Error ProcessSyscall(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Provede klonovani procesu nebo vlakna
	/// </summary>
	/// <param name="regs">kontext</param>
	/// <returns>Success pokud vse probehlo v poradku, jinak chybu</returns>
	kiv_os::NOS_Error PerformClone(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Getter pro vlakno procesu podle tidu
	/// </summary>
	/// <param name="tid">tid</param>
	/// <returns>Pointer na vlakno</returns>
	std::shared_ptr<Thread> GetThread(const kiv_os::THandle tid);

	kiv_os::NOS_Error CreateProcess(kiv_hal::TRegisters& regs);

	kiv_os::NOS_Error CreateThread(kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	/// <summary>
	/// Spusti
	/// </summary>
	/// <param name="subscriber_handle"></param>
	/// <param name="notifier_handle"></param>
	void TriggerSuspendCallback(kiv_os::THandle subscriber_handle, kiv_os::THandle notifier_handle);

	/// <summary>
	/// Prevede proces ze stavu Running do stavu Finished
	/// </summary>
	/// <param name="pid"></param>
	void FinishProcess(kiv_os::THandle pid);

	/// <summary>
	/// Vytvori Init proces. Tato metoda se musi zavolat v mainu, jinak nebude kernel blokovat, dokud
	///	se neukonci shell.
	/// </summary>
	void CreateInitProcess();

	/// <summary>
	/// Vytvori callback pro vzbuzeni vlakna (pokud neexistuje)
	/// </summary>
	/// <param name="subscriber_handle">tid vlakna, ktere se ma vzbudit</param>
	void InitializeSuspendCallback(kiv_os::THandle subscriber_handle);

	/// <summary>
	/// Odstrani callback pro vzbuzeni
	/// </summary>
	/// <param name="subscriber_handle"></param>
	void RemoveSuspendCallback(kiv_os::THandle subscriber_handle);

	/// <summary>
	/// Vrati typ handle pro  dane id
	/// </summary>
	/// <param name="id">id, pro ktere chceme typ handle dostat</param>
	/// <returns>typ handle</returns>
	static HandleType GetHandleType(const kiv_os::THandle id);

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

	kiv_os::NOS_Error PerformReadExitCode(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error PerformProcessExit(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error PerformShutdown(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error PerformRegisterSignalHandler(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
};
