#include "Command.h"
#include <sstream>

Command::Command(std::string command_name, std::vector<std::string> params, std::string source_file,
                 std::string target_file) :
	params(std::move(params)),
	command_name(std::move(command_name)),
	redirect_type(ResolveRedirectType(source_file, target_file)),
	source_file(std::move(source_file)),
	target_file(std::move(target_file)) {}

void Command::SetPipeFileDescriptors(const kiv_os::THandle fd_in, const kiv_os::THandle fd_out) {
	input_fd = fd_in;
	input_fd_open = true;
	output_fd = fd_out;
	output_fd_open = true;
}

kiv_os::THandle Command::GetInputFileDescriptor() const {
	return input_fd;
}

kiv_os::THandle Command::GetOutputFileDescriptor() const {
	return output_fd;
}

bool Command::InputFileDescriptorOpen() const {
	return input_fd_open;
}

bool Command::OutputFileDescriptorOpen() const {
	return output_fd_open;
}

void Command::SetPid(const kiv_os::THandle pid) {
	this->pid = pid;
}

kiv_os::THandle Command::GetPid() const {
	return pid;
}

std::string Command::ToString() const {

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

std::string Command::GetRtlParams() const {
	return StringUtils::JoinByDelimiter(params, " ");
}

RedirectType Command::ResolveRedirectType(const std::string& source_file, const std::string& target_file) {
	if (source_file.empty() && target_file.empty()) {
		return RedirectType::None;
	}

	if (!source_file.empty() && !target_file.empty()) {
		return RedirectType::Both;
	}

	return !source_file.empty() ? RedirectType::FromFile : RedirectType::ToFile;
}
