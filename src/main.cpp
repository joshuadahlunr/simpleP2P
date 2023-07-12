#include <argparse/argparse.hpp>
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>

extern "C" {
	#include "libgolib.h"
}

extern "C" {
	bool bridge_void_callback(void_callback f) {
		if(f == nullptr) return true;
		return f();
	}
	bool bridge_msg_callback(Message* m, msg_callback f) {
		if(f == nullptr) return true;
		return f(m);
	}
	bool bridge_peer_callback(char* p, peer_callback f) {
		if(f == nullptr) return true;
		return f(p);
	}
	bool bridge_topic_callback(int t, topic_callback f){
		if(f == nullptr) return true;
		return f(t);
	}
}

bool print(Message* message) {
	if(std::string_view(message->recieved_from) == std::string_view(localID()))
		// std::cout << "from us";
		return true;

	// Green console colour: 	\x1b[32m
	// Reset console colour: 	\x1b[0m
	std::cout << "\x1b[32m" << message->recieved_from << ": " << message->data << "\n\x1b[0m> " << std::flush;
	return true;
}


bool peerJoined(char* id) {
	std::cout << id << " connected!" << std::endl;
	return true;
}

bool peerLeft(char* id) {
	std::cout << id << " disconnected!" << std::endl;
	return true;
}

bool topicSubscribed(int topic) {
	std::cout << "subscribed to topic " << topic << std::endl;
	return true;
}

bool topicUnsubscribed(int topic) {
	std::cout << "unsubscribed to topic " << topic << std::endl;
	return true;
}

bool connected() {
	std::cout << "connected to the network!\n> " << std::flush;
	return true;
}



struct Args : public argparse::Args {
	std::string& keyFile = kwarg("k,keyfile", "File path to the key identity").set_default("id.key");
};

int main(int argc, char* argv[]) {
	Args args = argparse::parse<Args>(argc, argv);

	if (!std::filesystem::exists(args.keyFile)) {
		auto key = generateCKey();
		std::ofstream fout(args.keyFile);
		fout.write((const char*)&key.r1, sizeof(key.r1));
		fout.write(key.r0, key.r1);
	}

	std::vector<char> key;
	{
		std::ifstream fin(args.keyFile);
		int size;
		fin.read((char*)&size, sizeof(size));
		key.resize(size);
		fin.read(key.data(), key.size());
	}


	setMessageCallback(print);
	setConnectedCallback(connected);

	setPeerConnectedCallback(peerJoined);
	setPeerDisconnectedCallback(peerLeft);

	setTopicSubscribedCallback(topicSubscribed);
	setTopicUnsubscribedCallback(topicUnsubscribed);



	std::string_view topic = "chat/debug/v1.0.0";
	std::string_view listen = "/ip4/0.0.0.0/udp/0/quic-v1";
	auto id = initialize({listen.data(), (long)listen.size()}, {topic.data(), (long)topic.size()}, {key.data(), (long)key.size()}, false);

	std::cout << "listening to topic: " << id << std::endl;

	std::string line;
	while(true){
		std::cout << "> " << std::flush;
		std::getline(std::cin, line);

		bool success = broadcastMessage({line.data(), (long)line.size()}, id);
	}

	std::cout << "Will this ever run?" << std::endl;
}