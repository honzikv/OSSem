#pragma once
#include <memory>
#include <array>
#include <string>
#include <mutex>

#include "kernel.h"
#include "Thread.h"
#include "Process.h"
#include "../api/api.h"
#include "handles.h"
#include "Utils/Logging.h"

class RunnableManager {
public:
	// Konstanty pro rozsahy pidu a tidu

	static constexpr uint16_t PID_RANGE_START = 0;
	static constexpr uint16_t PID_RANGE_END = 4095;
	static constexpr uint16_t TID_RANGE_START = 4096;
	static constexpr uint16_t TID_RANGE_END = 8144;
	static constexpr uint16_t NO_FREE_ID = -1;

	/// <summary>
	/// Vychozi pracovni adresar, pokud neni zadny 
	/// </summary>
	inline static const std::string DEFAULT_PROCESS_WORKDIR = "C:\\";

	/// <summary>
	/// Singleton ziskani objektu. Provede lazy inicializaci a vrati referenci
	/// </summary>
	/// <returns>Referenci na singleton instanci teto tridy</returns>
	static RunnableManager& get() {
		static RunnableManager instance;
		return instance;
	}

private:
	/// <summary>
	/// Tabulka procesu
	/// </summary>
	std::array<std::shared_ptr<Process>, PID_RANGE_END - PID_RANGE_START> process_table;

	/// <summary>
	/// Tabulka vlaken
	/// </summary>
	std::array<std::shared_ptr<Thread>, TID_RANGE_END - TID_RANGE_START> thread_table;

	/// <summary>
	/// TID -> Windows HANDLE
	/// </summary>
	std::unordered_map<std::thread::id, kiv_os::THandle> native_tid_to_kiv_handle;

	/// <summary>
	/// Ziska prvni volny pid, pokud neni vrati NO_FREE_ID
	/// </summary>
	/// <returns>Volny pid, nebo NO_FREE_ID, pokud zadny neni</returns>
	kiv_os::THandle Get_Free_Pid() const;

	/// <summary>
	/// Ziska prvni volny tid, pokud neni vrati NO_FREE_ID
	/// </summary>
	/// <returns>Volny tid, nebo NO_FREE_ID, pokud zadny neni</returns>
	kiv_os::THandle Get_Free_Tid() const;

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
	/// Getter pro vlakno procesu podle tidu
	/// </summary>
	/// <param name="tid">tid</param>
	/// <returns>Pointer na vlakno</returns>
	std::shared_ptr<Thread> Get_Thread(const kiv_os::THandle tid);

	void Add_Thread(std::shared_ptr<Thread> thread, kiv_os::THandle tid );

	/// <summary>
	/// Slouzi k zamykani kriticke sekce
	/// </summary>
	std::recursive_mutex mutex;

	RunnableManager() = default;
	~RunnableManager() = default;
	RunnableManager(const RunnableManager&) = delete;
	RunnableManager& operator=(const RunnableManager&) = delete;

public:
	/// <summary>
	/// Zpracuje pozadavek
	/// </summary>
	/// <param name="regs">registry</param>
	/// <returns>vysledek operace</returns>
	kiv_os::NOS_Error Process_Syscall(kiv_hal::TRegisters& regs);

	/// <summary>
	/// Provede klonovani procesu nebo vlakna
	/// </summary>
	/// <param name="regs">kontext</param>
	/// <returns>Success pokud vse probehlo v poradku, jinak chybu</returns>
	kiv_os::NOS_Error Perform_Clone(kiv_hal::TRegisters& regs);

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle Find_Parent_Pid();

	void Dispatch_Process(std::shared_ptr<Process> process);

	kiv_os::NOS_Error Create_Process(kiv_hal::TRegisters& regs);

	// TODO impl this
	kiv_os::NOS_Error Create_Thread(kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error Perform_Wait_For(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error Perform_Read_Exit_Code(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error Perform_Process_Exit(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error Perform_Shutdown(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error Perform_Register_Signal_Handler(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
};
