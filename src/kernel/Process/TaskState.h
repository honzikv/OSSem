#pragma once
#include <cstdint>

/// <summary>
/// Stav procesu/vlakna
/// </summary>
enum class TaskState : uint8_t {
	Ready, // Nove vytvorene vlakno / proces
	Running, // Stav, kdy vlakno / proces aktivne bezi
	Finished, // Vlakno/proces dobehli a bude se odstranat z tabulky
	Blocked // Stav, kdy se ceka na odblokovani jinym vlaknem / procesem
};