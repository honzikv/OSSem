#include <string>
#include <vector>
#include "../Utils/StringUtils.h"
#include "../../api/api.h"

/// <summary>
/// Typ presmerovani pro prikaz
/// </summary>
enum class RedirectType : uint8_t {
	FromFile,
	// Ze souboru
	ToFile,
	// Do souboru
	Both,
	// Ze souboru i do souboru
	None // Zadny - Default
};

/// <summary>
/// Reprezentuje prikaz v commandline
/// </summary>
struct Command {
	/// <summary>
	/// Parametry prikazu
	/// </summary>
	const std::vector<std::string> params;

	/// <summary>
	/// Jmeno pozadovaneho prikazu
	/// </summary>
	const std::string command_name;

	/// <summary>
	/// Typ presmerovani - ze souboru, do souboru, oboji, zadne
	/// </summary>
	const RedirectType redirect_type;

	/// <summary>
	/// Vstupni soubor (napr. xyz < source.json)
	/// </summary>
	const std::string source_file;

	/// <summary>
	/// Cilovy soubor (napr. xyz > target.json)
	/// </summary>
	const std::string target_file;

	Command(std::string command_name, std::vector<std::string> params, std::string source_file = "",
	        std::string target_file = "");

private:
	/// <summary>
	/// File descriptor pro cteni vstupu
	/// </summary>
	kiv_os::THandle std_in_file_descriptor = kiv_os::Invalid_Handle;

	/// <summary>
	/// Zda-li je vstupni file descriptor otevreny
	/// </summary>
	bool std_in_open = false;

	/// <summary>
	/// File descriptor pro cteni vystupu
	/// </summary>
	kiv_os::THandle std_out_file_descriptor = kiv_os::Invalid_Handle;

	/// <summary>
	/// Zda-li je vystupni file descriptor otevreny
	/// </summary>
	bool std_out_open = false;

	kiv_os::THandle pid = kiv_os::Invalid_Handle;
public:

	/// <summary>
	/// Nastavi file descriptor pro standardni vstup
	/// </summary>
	/// <param name="std_in_file_descriptor"></param>
	void Set_Std_In_File_Descriptor(kiv_os::THandle std_in_file_descriptor);

	/// <summary>
	/// Nastavi file descriptor pro standardni vystup
	/// </summary>
	/// <param name="std_out_file_descriptor"></param>
	void Set_Std_Out_File_Descriptor(kiv_os::THandle std_out_file_descriptor);

	/// <summary>
	/// Nastavi standardni vstup i vystup
	/// </summary>
	/// <param name="std_in_file_descriptor"></param>
	/// <param name="std_out_file_descriptor"></param>
	void Set_Stdio_File_Descriptors(kiv_os::THandle std_in_file_descriptor, kiv_os::THandle std_out_file_descriptor);

	/// <summary>
	/// Vrati file descriptor pro vstup
	/// </summary>
	/// <returns>File descriptor pro vstup</returns>
	kiv_os::THandle Get_Input_File_Descriptor() const;

	/// <summary>
	/// Vrati file descriptor pro vystup
	/// </summary>
	/// <returns>File descriptor pro vystup</returns>
	kiv_os::THandle Get_Output_File_Descriptor() const;

	/// <summary>
	/// Vrati, zda-li je vstupni file descriptor otevreny
	/// </summary>
	/// <returns>True, pokud je file descriptor pro vstup otevreny, jinak false</returns>
	bool Is_Input_File_Descriptor_Open() const;

	/// <summary>
	/// Vrati, zda-li je vystupni file descriptor otevreny
	/// </summary>
	/// <returns>True, pokud je file descriptor pro vystup otevreny, jinak false</returns>
	bool Is_Output_File_Descriptor_Open() const;

	/// <summary>
	/// Nastavi pid pro dany prikaz
	/// </summary>
	/// <param name="pid">pid prikazu</param>
	void Set_Pid(kiv_os::THandle pid);

	/// <summary>
	/// Ziska pid pro dany task
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle Get_Pid() const;

	/// <summary>
	/// To String pro debugovani / testovani
	/// </summary>
	/// <returns></returns>
	[[nodiscard]] std::string To_String() const;

	/// <summary>
	/// Vrati parametry jako string, kdy kazdy parametr je oddeleny mezerou
	/// </summary>
	/// <returns>Retezec s parametry oddelenymi mezerou</returns>
	std::string Get_Rtl_Params() const;

	friend bool operator==(const Command& lhs, const Command& rhs);

	friend bool operator!=(const Command& lhs, const Command& rhs) { return !(lhs == rhs); }

private:

	static RedirectType Resolve_Redirect_Type(const std::string& source_file, const std::string& target_file);
};
