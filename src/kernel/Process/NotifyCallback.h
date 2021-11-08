#pragma once
#include "Utils/Semaphore.h"
#include "../api/api.h"

class NotifyCallback {

	/// <summary>
	/// Semafor pro synchronizaci
	/// </summary>
	Semaphore semaphore;

	/// <summary>
	/// Handle, na ktere se ceka - bud vlakno, nebo proces
	/// </summary>
	kiv_os::THandle notifier;
};