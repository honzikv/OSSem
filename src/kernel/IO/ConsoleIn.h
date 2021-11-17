#pragma once
#include "IFile.h"

/// <summary>
/// Cteni z konzole
/// </summary>
class ConsoleIn final : public IFile  {


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
