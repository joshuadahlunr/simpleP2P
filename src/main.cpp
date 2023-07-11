#include <argparse/argparse.hpp>
#include <iostream>
#include <thread>

extern "C" {
	#include "libgolib.h"
}

extern "C" {
	const char* bridge_str_callback(char* s, str_callback f) { 
		return f(s);
	}
}

std::string line;
const char* input(char* _) {
	std::cout << "> " << std::flush;
	std::getline(std::cin, line);

	return line.c_str();
}

const char* print(char* message) {
	// Green console colour: 	\x1b[32m
	// Reset console colour: 	\x1b[0m
	std::cout << "\x1b[32m" << message << "\x1b[0m> " << std::flush;
	return "good";
}

// const char* callback(char* message) {
// 	if(message) { // We should print something...
// 		// Green console colour: 	\x1b[32m
// 		// Reset console colour: 	\x1b[0m
// 		std::cout << "\x1b[32m" << message << "\x1b[0m> " << std::flush;
// 		return "good";
	
// 	// Otherwise... we need to read something!
// 	} else {
// 		std::cout << "> " << std::flush;
// 		std::getline(std::cin, line);

// 		return line.c_str();
// 	}
// }



struct Args : public argparse::Args {
	int& sourcePort = kwarg("p,sp", "Source port number").set_default(0);
	std::string& dest = kwarg("d,destination", "Destination multiaddr string").set_default("");
	bool& debug = flag("debug", "Debug generates the same node ID on every execution");
};

int main(int argc, char* argv[]) {
	Args args = argparse::parse<Args>(argc, argv);

	setPrintCallback(print);
	setInputCallback(input);
	// setCallback(callback);

	auto t = std::thread([=] { run(args.sourcePort, {args.dest.data(), (long)args.dest.size()}, args.debug); });

	std::cout << "Will this ever run?" << std::endl;

	t.join();
}