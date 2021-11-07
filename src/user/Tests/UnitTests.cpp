﻿#include "UnitTests.h"

#include <iostream>
#include "Shell/Shell.h"

void compareCommands(Command& actual, Command& expected) {
	if (actual != expected) {
		std::cerr << "Expected: " << expected.toString() << std::endl;
		std::cerr << "Actual: " << actual.toString() << std::endl;
		throw std::exception();
	}
}

auto shellTest_SimpleCommand(Shell& shellInterpreter) {
	const auto input = "ls -a -b -c";

	auto expectedCommand = Command("ls", {"-a", "-b", "-c"});
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

auto shellTest_NoCommand(Shell& shellInterpreter) {
	auto input = "";

	const auto commands = shellInterpreter.parseCommands(input);
	if (!commands.empty()) {
		std::cerr << "Error commands are not empty" << std::endl;
	}

	std::cout << "shellTest_NoCommand Test succeeded" << std::endl;
}

auto shellTest_FileRedirect(Shell& shellInterpreter) {
	auto input = "command1 < data.json";

	const auto commands = shellInterpreter.parseCommands(input);

	if (commands.size() != 1) {
		std::cerr << "Incorrect number of parameters parsed. Expected 1 got: " << commands.size() << std::endl;
		return;
	}

	auto expectedCommand = Command("command1", {}, "data.json", "");
	auto actualCommand = commands[0];

	try {
		compareCommands(expectedCommand, actualCommand);
	}
	catch (...) {
		std::cerr << "shellTest_FileRedirect: Test failed during comparison." << std::endl;
		return;
	}

	std::cout << "shellTest_FileRedirect Test succeeded." << std::endl;
}

auto shellTest_MultipleCommandsWithFileRedirects(Shell& shellInterpreter) {
	auto input = "command1 -a -b | command2 > out.json|command3|command4 |command5 -a -b -c < from.json";

	auto expectedCommands = std::vector<Command>({
		{"command1", {"-a", "-b"}},
		{"command2", {}, "", "out.json"},
		{"command3", {}},
		{"command4", {}},
		{"command5", {"-a", "-b", "-c"}, "from.json", ""},

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
		std::cerr << "Error while performing shellTest_MultipleCommandsWithFileRedirects test. Cause: " << ex.what() <<
			std::endl;
	}

	std::cout << "shellTest_MultipleCommandsWithFileRedirects Test succeeded." << std::endl;
}

auto shellTest_InvalidInput1(Shell& shellInterpreter) {
	const auto input = "command <| command";
	try {
		shellInterpreter.parseCommands(input);
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

auto shellTest_InvalidInput2(Shell& shellInterpreter) {
	const auto input = "| | | <> <<<.././/.sdf./sd";
	try {
		shellInterpreter.parseCommands(input);
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

auto shellTest_InvalidInput3(Shell& shellInterpreter) {
	const auto input = "||||||:))):L(:)<::}🙌🙌🙌";
	auto commands = shellInterpreter.parseCommands(input);
	if (commands.size() != 1) {
		std::cerr << "shellTest_InvalidInput3: Error, incorrect number of commands parsed. Expected 1, got: " <<
			commands.size() << std::endl;
		return;
	}

	auto expectedCommand = Command(":))):L(:)", {}, "", "::}🙌🙌🙌");
	try {
		compareCommands(expectedCommand, commands[0]);
	}
	catch (...) {
		std::cerr << "shellTest_InvalidInput3: Test failed during comparison." << std::endl;
		return;
	}
	std::cout << "shellTest_InvalidInput3 Test succeeded." << std::endl;
}

auto shellTest_InvalidInput4(Shell& shellInterpreter) {
	const auto input =
		"She travelling acceptance men unpleasant her especially entreaties law. Law forth but end any arise chief arose.";
	auto commands = shellInterpreter.parseCommands(input);

	if (commands.size() != 1) {
		std::cerr << "shellTest_InvalidInput4: Test failed, expected 1 command, instead got: " << commands.size() << std::endl;
	}

	auto expectedCommand = Command("She", {
		"travelling", "acceptance", "men", "unpleasant", "her", "especially", "entreaties", "law.", "Law", "forth",
		"but", "end", "any", "arise", "chief", "arose."
		}, "", "");
	
	try {
		compareCommands(expectedCommand, commands[0]);
	}
	catch (...) {
		std::cerr << "shellTest_InvalidInput4: Test failed during comparison." << std::endl;
		return;
	}
	std::cout << "shellTest_InvalidInput4 Test succeeded." << std::endl;

}


void TestRunner::runTests() {
	auto shellInterpreter = Shell({}, {}, {});
	
	shellTest_SimpleCommand(shellInterpreter);
	shellTest_NoCommand(shellInterpreter);
	shellTest_FileRedirect(shellInterpreter);
	shellTest_MultipleCommandsWithFileRedirects(shellInterpreter);
	shellTest_InvalidInput1(shellInterpreter);
	shellTest_InvalidInput2(shellInterpreter);
	shellTest_InvalidInput3(shellInterpreter);
	shellTest_InvalidInput4(shellInterpreter);
}
