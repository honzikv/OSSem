#pragma once
#include "../Utils/Semaphore.h"
#include "../../api/api.h"

/// <summary>
/// Callback, ktery se spusti pri vzbuzeni vlakna
/// </summary>
class SuspendCallback {

	/// <summary>
	/// Mutex pro synchronizaci
	/// </summary>
	std::mutex mutex;

	/// <summary>
	/// Semafor pro synchronizaci - vlakno, ktere na callback ceka zavola semafor.acquire().
	/// Semafor je shared_ptr, protoze jinak by se objekt prekopiroval pri ziskani z hashmapy
	/// </summary>
	const std::unique_ptr<Semaphore> semaphore = std::make_unique<Semaphore>();

	/// <summary>
	/// Id entity, ktera vzbudila spici vlakno
	/// </summary>
	kiv_os::THandle notifier_id = kiv_os::Invalid_Handle;

	/// <summary>
	/// Zda-li se callback provedl
	/// </summary>
	std::atomic<bool> triggered = {false};

public:
	/// <summary>
	/// Uspi vlakno pomoci semaforu
	/// </summary>
	void Suspend() const;

	/// <summary>
	/// Vzbudi vlakno a nastavi notifier_id, handle_type a triggered flag
	/// </summary>
	/// <param name="notifier_id">id vlakna/procesu, ktere vzbudilo vlakno</param>
	void Notify(kiv_os::THandle notifier_id);

	bool Triggered();

	/// <summary>
	/// Toto se muze volat az po tom, co se triggered nastavi na true.
	///	Vrati id vlakna/procesu, ktere triggernulo tento callback
	/// </summary>
	/// <returns></returns>
	[[nodiscard]] kiv_os::THandle Get_Notifier_Id() const;
};
