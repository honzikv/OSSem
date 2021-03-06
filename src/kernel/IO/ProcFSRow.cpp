#include "ProcFSRow.h"

ProcFSRow::ProcFSRow(const std::string& program_name, const uint32_t running_threads, const kiv_os::THandle pid,
                     const TaskState task_state): program_name(program_name),
                                                  running_threads(running_threads),
                                                  pid(pid),
                                                  state(static_cast<uint8_t>(task_state)) {}

std::shared_ptr<ProcFSRow> ProcFSRow::Map_Process_To_ProcFSRow(const Process& process) {
	return std::make_shared<ProcFSRow>(process.Get_Program_Name(), static_cast<uint32_t>(process.Get_Process_Threads().size()),
	                                   process.Get_Pid(), process.Get_Task_State());
}
