#include "Command.h"
#include <sstream>

Command::Command(std::string command_name, std::vector<std::string> params, std::string source_file,
                 std::string target_file) :
	params(std::move(params)),
	command_name(std::move(command_name)),
	redirect_type(Resolve_Redirect_Type(source_file, target_file)),
	source_file(std::move(source_file)),
	target_file(std::move(target_file)) {}

void Command::Set_Std_In_File_Descriptor(const kiv_os::THandle std_in_file_descriptor) {
	this->std_in_file_descriptor = std_in_file_descriptor;
	std_in_open = true;
}

void Command::Set_Std_Out_File_Descriptor(const kiv_os::THandle std_out_file_descriptor) {
	this->std_out_file_descriptor = std_out_file_descriptor;
	std_out_open = true;
}

void Command::Set_Stdio_File_Descriptors(const kiv_os::THandle std_in_file_descriptor,
                                         const kiv_os::THandle std_out_file_descriptor) {
	Set_Std_In_File_Descriptor(std_in_file_descriptor);
	Set_Std_Out_File_Descriptor(std_out_file_descriptor);
}


kiv_os::THandle Command::Get_Input_File_Descriptor() const {
	return std_in_file_descriptor;
}

kiv_os::THandle Command::Get_Output_File_Descriptor() const {
	return std_out_file_descriptor;
}

bool Command::Is_Input_File_Descriptor_Open() const {
	return std_in_open;
}

bool Command::Is_Output_File_Descriptor_Open() const {
	return std_out_open;
}

void Command::Set_Pid(const kiv_os::THandle pid) {
	this->pid = pid;
}

kiv_os::THandle Command::Get_Pid() const {
	return pid;
}

std::string Command::To_String() const {

	std::string redirectTypeStr;
	switch (redirect_type) {
		case RedirectType::Both:
			redirectTypeStr = "BOTH";
			break;
		case RedirectType::FromFile:
			redirectTypeStr = "FROM_FILE";
			break;
		case RedirectType::None:
			redirectTypeStr = "NONE";
			break;
		case RedirectType::ToFile:
			redirectTypeStr = "TO_FILE";
			break;
	}

	auto stringStream = std::stringstream();
	stringStream << "commandName: " << command_name << ", redirectType: " << redirectTypeStr << ", Sourcefile: " <<
		source_file << ", targetFile: " << target_file << std::endl;
	stringStream << "params: [";
	for (const auto& param : params) {
		stringStream << param << " ";
	}
	stringStream << "]";
	return stringStream.str();
}

std::string Command::Get_Rtl_Params() const {
	return StringUtils::Join_By_Delimiter(params, " ");
}

RedirectType Command::Resolve_Redirect_Type(const std::string& source_file, const std::string& target_file) {
	if (source_file.empty() && target_file.empty()) {
		return RedirectType::None;
	}

	if (!source_file.empty() && !target_file.empty()) {
		return RedirectType::Both;
	}

	return !source_file.empty() ? RedirectType::FromFile : RedirectType::ToFile;
}
