#include "shell.h"

#include <array>
#include <memory>
#include <string>



size_t __stdcall shell(const kiv_hal::TRegisters& regs)
{
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	constexpr size_t buffer_size = 256;
	auto buffer = std::array<char, buffer_size>(); // buffer pro nacitani user inputu
	size_t counter;
	
	// constexpr auto intro = "Vitejte v kostre semestralni prace z KIV/OS.\n"
	// 	"Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
	// kiv_os_rtl::Write_File(std_out, intro, strlen(intro), counter);

	const auto shellInterpreter = std::make_unique<ShellInterpreter>(regs, std_in, std_out);
	constexpr auto prompt = "C:\\>"; // todo: filesystem integration
	do
	{
		kiv_os_rtl::Write_File(std_out, prompt, strlen(prompt), counter);

		if (kiv_os_rtl::Read_File(std_in, buffer.data(), buffer_size, counter))
		{
			if (counter == buffer_size) { counter--; }
			buffer[counter] = 0; //udelame z precteneho vstup null-terminated retezec

			constexpr auto new_line = "\n";
			kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
			shellInterpreter->parseLine(buffer.data(), counter);
			// kiv_os_rtl::Write_File(std_out, buffer.data(), strlen(buffer.data()), counter); //a vypiseme ho
			kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
		}
		else
		{
			break; //EOF
		}
	}
	while (buffer.data() != EXIT_COMMAND);

	return 0;
}
