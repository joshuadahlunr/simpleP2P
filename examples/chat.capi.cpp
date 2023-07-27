#include <argparse/argparse.hpp>
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>

#include "simplep2p.hpp"
using namespace p2p; // We are using the C++ wrapper's utilities for saving and loading to the file... 
// the C++ wrapper puts all of the the C wrapper code into the p2p namespace so we use it to pretend we are just using the C api...


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
	std::string& keyFile = kwarg("k,keyfile", "File path to the key identity").set_default("id.capi.key");
};

int main(int argc, char* argv[]) {
	Args args = argparse::parse<Args>(argc, argv);

	p2p::Key key;
	if (!std::filesystem::exists(args.keyFile)) {
		key = p2p::Key::generate();
		std::ofstream fout(args.keyFile);
		key.save(fout);
	}

	{
		std::ifstream fin(args.keyFile);
		key.load(fin);
	}


	p2p_set_message_callback(print);
	p2p_set_connected_callback(connected);

	p2p_set_peer_connected_callback(peerJoined); // NOTE: these callbacks will only be called for peers directly connected... if you need to know about all peers in the network that will need to be done at a higher level!
	p2p_set_peer_disconnected_callback(peerLeft); // NOTE: these callbacks will only be called for peers directly connected... if you need to know about all peers in the network that will need to be done at a higher level!

	p2p_set_topic_subscribed_callback(topicSubscribed);
	p2p_set_topic_unsubscribed_callback(topicUnsubscribed);

	auto topic = p2p_initialize(p2p_initialize_args_from_strings("/ip4/0.0.0.0/udp/0/quic-v1", "chat/capi/v1.0.0", key, 60, false));

	std::string line;
	while(true){
		std::cout << "> " << std::flush;
		std::getline(std::cin, line);

		bool success = p2p_broadcast_messagen(line.data(), line.size(), topic);
	}
}