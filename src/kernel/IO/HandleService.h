#pragma once
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "../../api/api.h"
#include <Windows.h>

#include "ConsoleIn.h"
#include "ConsoleOut.h"

/// <summary>
/// Singleton pro vytvareni handlu
/// </summary>
class HandleService {

public:
	/// <summary>
	/// Z nejakeho duvodu numeric limits ve windows nefunguji ...
	/// </summary>
	static constexpr uint16_t UINT16_T_MAX = 0xffff;

	static HandleService& Get() {
		static HandleService instance;
		return instance;
	}

private:
	HandleService() = default;
	~HandleService() = default;
	HandleService(const HandleService&) = delete;
	HandleService& operator=(const HandleService&) = delete;

	/// <summary>
	/// Potencialni id noveho handlu
	/// </summary>
	uint16_t new_handle_id = 0;

	std::unordered_map<uint16_t, HANDLE> handles;

	/// <summary>
	/// Mutex pro pristup do tabulky
	/// </summary>
	std::mutex mutex;

public:
	kiv_os::THandle CreateHandle(HANDLE native_handle);

	/// <summary>
	/// Vytvori novy empty handle - pouziva se pro file descriptory
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle CreateEmptyHandle();

	/// <summary>
	/// Vrati handle nebo HANDLE_INVALID_VALUE, pokud se handle nepodarilo najit
	/// </summary>
	/// <param name="handle">interni handle</param>
	/// <returns></returns>
	HANDLE GetHandle(kiv_os::THandle handle);

	/// <summary>
	/// Odstrani handle z OS
	/// </summary>
	/// <param name="handle"></param>
	void RemoveHandle(kiv_os::THandle handle);


};
