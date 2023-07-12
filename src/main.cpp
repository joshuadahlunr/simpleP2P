#include <argparse/argparse.hpp>
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>

#include "simplep2p.h"


bool print(P2PMessage* message) {
	if(std::string_view(message->received_from) == std::string_view(p2p_local_id()))
		// std::cout << "from us";
		return true;

	// Green console colour: 	\x1b[32m
	// Reset console colour: 	\x1b[0m
	std::cout << "\x1b[32m" << message->received_from << ": " << message->data << "\n\x1b[0m> " << std::flush;
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
		auto key = p2p_generate_key();
		std::ofstream fout(args.keyFile);
		fout.write((const char*)&key.data, sizeof(key.size));
		fout.write(key.data, key.size);
	}

	std::vector<char> key;
	{
		std::ifstream fin(args.keyFile);
		int size;
		fin.read((char*)&size, sizeof(size));
		key.resize(size);
		fin.read(key.data(), key.size());
	}


	p2p_set_message_callback(print);
	p2p_set_connected_callback(connected);

	p2p_set_peer_connected_callback(peerJoined);
	p2p_set_peer_disconnected_callback(peerLeft);

	p2p_set_topic_subscribed_callback(topicSubscribed);
	p2p_set_topic_unsubscribed_callback(topicUnsubscribed);



	std::string_view topic = "chat/debug/v1.0.0";
	std::string_view listen = "/ip4/0.0.0.0/udp/0/quic-v1";
	auto id = p2p_initialize(p2p_initialize_args_from_strings("/ip4/0.0.0.0/udp/0/quic-v1", "chat/debug/v1.0.0", {key.data(), (int)key.size()}, false));

	std::string line;
	while(true){
		std::cout << "> " << std::flush;
		std::getline(std::cin, line);

		bool success = p2p_broadcast_messagen(line.data(), line.size(), id);
	}
}