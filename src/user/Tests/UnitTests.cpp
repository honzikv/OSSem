#include "UnitTests.h"

#include <iostream>
#include "Shell/ShellInterpreter.h"

void compareCommands(Command& actual, Command& expected) {
	if (actual != expected) {
		std::cerr << "Expected: " << expected.toString() << std::endl;
		std::cerr << "Actual: " << actual.toString() << std::endl;
		throw std::exception();
	}
}

auto shellTest_SimpleCommand(ShellInterpreter& shellInterpreter) {
	const auto input = "ls -a -b -c";

	auto expectedCommand = Command("ls", {"-a", "-b", "-c"}, RedirectType::None, {});
	auto commands = shellInterpreter.parseCommands(input);

	if (commands.size() > 1) {
		std::cerr << "Test failed, parsed: " << commands.size() << " commands instead of 1" << std::endl;
		return;
	}

	try {
		compareCommands(commands[0], expectedCommand);
	}
	catch (...) {
		std::cerr << "Test failed" << std::endl;
		return;
	}

	std::cout << "shellTest_SimpleCommand Test succeeded" << std::endl;

}

auto shellTest_NoCommand(ShellInterpreter& shellInterpreter) {
	auto input = "";

	const auto commands = shellInterpreter.parseCommands(input);
	if (!commands.empty()) {
		std::cerr << "Error commands are not empty" << std::endl;
	}

	std::cout << "shellTest_NoCommand Test succeeded" << std::endl;
}

auto shellTest_FileRedirect(ShellInterpreter& shellInterpreter) {
	auto input = "command1 < data.json";

	const auto commands = shellInterpreter.parseCommands(input);

	if (commands.size() != 1) {
		std::cerr << "Incorrect number of parameters parsed. Expected 1 got: " << commands.size() << std::endl;
		return;
	}

	auto expectedCommand = Command("command1", {}, RedirectType::FromFile, "data.json");
	auto actualCommand = commands[0];

	try {
		compareCommands(expectedCommand, actualCommand);
	}
	catch (...) {
		std::cerr << "Test failed." << std::endl;
		return;
	}

	std::cout << "shellTest_FileRedirect Test succeeded." << std::endl;
}

auto shellTest_MultipleCommandsWithFileRedirects(ShellInterpreter& shellInterpreter) {
	auto input = "command1 -a -b | command2 > out.json|command3|command4 |command5 -a -b -c < from.json";

	auto expectedCommands = std::vector<Command>({
		{"command1", {"-a", "-b"}, None, {}},
		{"command2", {}, ToFile, "out.json"},
		{"command3", {}, None, {}},
		{"command4", {}, None, {}},
		{"command5", {"-a", "-b", "-c"}, FromFile, "from.json"},

	});

	auto actualCommands = shellInterpreter.parseCommands(input);

	if (actualCommands.size() != expectedCommands.size()) {
		std::cerr << "Error, incorrect number of items parsed, expected: " << expectedCommands.size() << " got " <<
			actualCommands.size() << std::endl;
		return;
	}

	try {
		for (auto i = 0; i < actualCommands.size(); i += 1) {
			compareCommands(actualCommands[i], expectedCommands[i]);
		}
	}
	catch (std::exception& ex) {
		std::cerr << "Error while performing shellTest_MultipleCommandsWithFileRedirects test. Cause: " << ex.what() << std::endl;
	}

	std::cout << "shellTest_MultipleCommandsWithFileRedirects Test succeeded." << std::endl;
}


void TestRunner::runTests() {
	auto shellInterpreter = ShellInterpreter({}, {}, {});

	shellTest_SimpleCommand(shellInterpreter);
	shellTest_NoCommand(shellInterpreter);
	shellTest_FileRedirect(shellInterpreter);
	shellTest_MultipleCommandsWithFileRedirects(shellInterpreter);
}
