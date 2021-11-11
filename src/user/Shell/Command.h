#include <string>
#include <vector>
#include <ostream>
#include "Utils/StringUtils.h"
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
	kiv_os::THandle input_fd = kiv_os::Invalid_Handle;

	/// <summary>
	/// Zda-li je vstupni file descriptor otevreny
	/// </summary>
	bool input_fd_open = false;

	/// <summary>
	/// File descriptor pro cteni vystupu
	/// </summary>
	kiv_os::THandle output_fd = kiv_os::Invalid_Handle;

	/// <summary>
	/// Zda-li je vystupni file descriptor otevreny
	/// </summary>
	bool output_fd_open = false;

	kiv_os::THandle pid = kiv_os::Invalid_Handle;
public:
	/// <summary>
	/// Nastavi file descriptory pro cteni a zapis
	/// </summary>
	void SetPipeFileDescriptors(kiv_os::THandle fd_in, kiv_os::THandle fd_out);

	/// <summary>
	/// Vrati file descriptor pro vstup
	/// </summary>
	/// <returns>File descriptor pro vstup</returns>
	kiv_os::THandle GetInputFileDescriptor() const;

	/// <summary>
	/// Vrati file descriptor pro vystup
	/// </summary>
	/// <returns>File descriptor pro vystup</returns>
	kiv_os::THandle GetOutputFileDescriptor() const;

	/// <summary>
	/// Vrati, zda-li je vstupni file descriptor otevreny
	/// </summary>
	/// <returns>True, pokud je file descriptor pro vstup otevreny, jinak false</returns>
	bool InputFileDescriptorOpen() const;

	/// <summary>
	/// Vrati, zda-li je vystupni file descriptor otevreny
	/// </summary>
	/// <returns>True, pokud je file descriptor pro vystup otevreny, jinak false</returns>
	bool OutputFileDescriptorOpen() const;

	/// <summary>
	/// Nastavi pid pro dany prikaz
	/// </summary>
	/// <param name="pid">pid prikazu</param>
	void SetPid(kiv_os::THandle pid);

	/// <summary>
	/// Ziska pid pro dany task
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle GetPid() const;

	/// <summary>
	/// To String pro debugovani / testovani
	/// </summary>
	/// <returns></returns>
	[[nodiscard]] std::string ToString() const;

	/// <summary>
	/// Vrati parametry jako string, kdy kazdy parametr je oddeleny mezerou
	/// </summary>
	/// <returns>Retezec s parametry oddelenymi mezerou</returns>
	std::string GetRtlParams() const;

	friend bool operator==(const Command& lhs, const Command& rhs);

	friend bool operator!=(const Command& lhs, const Command& rhs) { return !(lhs == rhs); }

private:

	static RedirectType ResolveRedirectType(const std::string& source_file, const std::string& target_file);
};
