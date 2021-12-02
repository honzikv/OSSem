#pragma once
#include "IFile.h"

/// <summary>
/// Cteni z konzole
/// </summary>
class ConsoleIn final : public IFile  {

	/// <summary>
	/// Vrati true, pokud buffer v klavesnici obsahuje control char
	/// </summary>
	/// <param name="control_char"></param>
	/// <returns></returns>
	static bool Has_Control_Char(kiv_hal::NControl_Codes control_char);


public:
	/// <summary>
	/// Cteni z klavesnice
	/// </summary>
	/// <param name="target_buffer"></param>
	/// <param name="buffer_size"></param>
	/// <param name="bytes_read"></param>
	/// <returns></returns>
	kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) override;

	/// <summary>
	/// Close pro konzoli moc smysl nedava, takze se jenom vrati success
	/// </summary>
	/// <returns></returns>
	kiv_os::NOS_Error Close() override;
};
