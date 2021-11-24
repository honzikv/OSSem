#include "UnitTests.h"

#include <iostream>
#include "Shell/Shell.h"
#if IS_DEBUG
void CompareCommands(const Command& actual, const Command& expected) {
	if (actual != expected) {
		std::cerr << "Expected: " << expected.To_String() << std::endl;
		std::cerr << "Actual: " << actual.To_String() << std::endl;
		throw std::exception();
	}
}

auto ShellTest_SimpleCommand(const Shell& shellInterpreter) {
	const auto input = "ls -a -b -c";

	const auto expectedCommand = Command("ls", {"-a", "-b", "-c"});
	const auto commands = shellInterpreter.Parse_Commands(input);

	if (commands.size() > 1) {
		std::cerr << "Test failed, parsed: " << commands.size() << " commands instead of 1" << std::endl;
		return;
	}

	try {
		CompareCommands(commands[0], expectedCommand);
	}
	catch (...) {
		std::cerr << "Test failed" << std::endl;
		return;
	}

	std::cout << "shellTest_SimpleCommand Test succeeded" << std::endl;

}

auto ShellTest_NoCommand(Shell& shellInterpreter) {
	const auto input = "";

	const auto commands = shellInterpreter.Parse_Commands(input);
	if (!commands.empty()) {
		std::cerr << "Error commands are not empty" << std::endl;
	}

	std::cout << "shellTest_NoCommand Test succeeded" << std::endl;
}

auto ShellTest_FileRedirect(Shell& shellInterpreter) {
	auto input = "command1 < data.json";

	const auto commands = shellInterpreter.Parse_Commands(input);

	if (commands.size() != 1) {
		std::cerr << "Incorrect number of parameters parsed. Expected 1 got: " << commands.size() << std::endl;
		return;
	}

	auto expectedCommand = Command("command1", {}, "data.json", "");
	auto actualCommand = commands[0];

	try {
		CompareCommands(expectedCommand, actualCommand);
	}
	catch (...) {
		std::cerr << "shellTest_FileRedirect: Test failed during comparison." << std::endl;
		return;
	}

	std::cout << "shellTest_FileRedirect Test succeeded." << std::endl;
}

auto ShellTest_MultipleCommandsWithFileRedirects(Shell& shellInterpreter) {
	const auto input = "command1 -a -b | command2 > out.json|command3|command4 |command5 -a -b -c < from.json";

	const auto expected_commands = std::vector<Command>({
		{"command1", {"-a", "-b"}},
		{"command2", {}, "", "out.json"},
		{"command3", {}},
		{"command4", {}},
		{"command5", {"-a", "-b", "-c"}, "from.json", ""},

	});

	const auto actual_commands = shellInterpreter.Parse_Commands(input);

	if (actual_commands.size() != expected_commands.size()) {
		std::cerr << "Error, incorrect number of items parsed, expected: " << expected_commands.size() << " got " <<
			actual_commands.size() << std::endl;
		return;
	}

	try {
		for (auto i = 0; i < actual_commands.size(); i += 1) {
			CompareCommands(actual_commands[i], expected_commands[i]);
		}
	}
	catch (std::exception& ex) {
		std::cerr << "Error while performing shellTest_MultipleCommandsWithFileRedirects test. Cause: " << ex.what() <<
			std::endl;
	}

	std::cout << "shellTest_MultipleCommandsWithFileRedirects Test succeeded." << std::endl;
}

auto ShellTest_InvalidInput1(const Shell& shell_interpreter) {
	const auto input = "command <| command";
	try {
		// ReSharper disable once CppNoDiscardExpression
		shell_interpreter.Parse_Commands(input);
	}
	catch (ParseException&) {
		std::cout << "shellTest_InvalidInput1 Test succeeded." << std::endl;
		return;
	}
	catch (std::exception& ex) {
		std::cerr << "shellTest_InvalidInput1: Error, exception ParseException was not caught. Instead got: " << ex.
			what() << std::endl;
		return;
	}

	std::cerr << "shellTest_InvalidInput1: Error, no exception was thrown" << std::endl;
}

auto ShellTest_InvalidInput2(const Shell& shell_interpreter) {
	const auto input = "| | | <> <<<.././/.sdf./sd";
	try {
		// ReSharper disable once CppNoDiscardExpression
		shell_interpreter.Parse_Commands(input);
	}
	catch (ParseException&) {
		std::cout << "shellTest_InvalidInput2 Test succeeded." << std::endl;
		return;
	}
	catch (std::exception& ex) {
		std::cerr << "shellTest_InvalidInput2: Error, exception ParseException was not caught. Instead got: " << ex.
			what() << std::endl;
		return;
	}

	std::cerr << "shellTest_InvalidInput2: Error, no exception was thrown" << std::endl;
}

auto ShellTest_InvalidInput3(const Shell& shell_interpreter) {
	const auto input = "||||||:))):L(:)<::}🙌🙌🙌";
	const auto commands = shell_interpreter.Parse_Commands(input);
	if (commands.size() != 1) {
		std::cerr << "shellTest_InvalidInput3: Error, incorrect number of commands parsed. Expected 1, got: " <<
			commands.size() << std::endl;
		return;
	}

	const auto expected_command = Command(":))):L(:)", {}, "::}🙌🙌🙌", "");
	try {
		CompareCommands(expected_command, commands[0]);
	}
	catch (...) {
		std::cerr << "shellTest_InvalidInput3: Test failed during comparison." << std::endl;
		return;
	}
	std::cout << "shellTest_InvalidInput3 Test succeeded." << std::endl;
}

auto ShellTest_InvalidInput4(const Shell& shell_interpreter) {
	const auto input =
		"She travelling acceptance men unpleasant her especially entreaties law. Law forth but end any arise chief arose.";
	const auto commands = shell_interpreter.Parse_Commands(input);

	if (commands.size() != 1) {
		std::cerr << "shellTest_InvalidInput4: Test failed, expected 1 command, instead got: " << commands.size() <<
			std::endl;
	}

	const auto expected_command = Command("She", {
		                                      "travelling", "acceptance", "men", "unpleasant", "her", "especially",
		                                      "entreaties", "law.", "Law", "forth",
		                                      "but", "end", "any", "arise", "chief", "arose."
	                                      }, "", "");

	try {
		CompareCommands(expected_command, commands[0]);
	}
	catch (...) {
		std::cerr << "shellTest_InvalidInput4: Test failed during comparison." << std::endl;
		return;
	}
	std::cout << "shellTest_InvalidInput4 Test succeeded." << std::endl;

}


void TestRunner::Run_Tests() {
	const auto curr_path = "";
	auto shell_interpreter = Shell({}, {}, {}, curr_path);

	ShellTest_SimpleCommand(shell_interpreter);
	ShellTest_NoCommand(shell_interpreter);
	ShellTest_FileRedirect(shell_interpreter);
	ShellTest_MultipleCommandsWithFileRedirects(shell_interpreter);
	ShellTest_InvalidInput1(shell_interpreter);
	ShellTest_InvalidInput2(shell_interpreter);
	ShellTest_InvalidInput3(shell_interpreter);
	ShellTest_InvalidInput4(shell_interpreter);
}
#endif
