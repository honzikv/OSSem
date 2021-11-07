#pragma once
#include <cstdint>

/// <summary>
/// Stav procesu/vlakna
/// </summary>
enum class RunnableState : uint8_t {
	Ready,
	Running,
	Finished
};