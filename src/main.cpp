#include <argparse/argparse.hpp>
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>

#define SIMPLE_P2P_IMPLEMENTATION
#include "simplep2p.hpp"


void print(p2p::Message& message) {
	if(message.is_local())
		return;

	// Green console colour: 	\x1b[32m
	// Reset console colour: 	\x1b[0m
	std::cout << "\x1b[32m" << message.sender() << ": " << message.data_string() << "\n\x1b[0m> " << std::flush;
}


void peerJoined(std::string_view id) {
	std::cout << id << " connected!" << std::endl;
}

void peerLeft(std::string_view id) {
	std::cout << id << " disconnected!" << std::endl;
}

void topicSubscribed(p2p::Topic topic) {
	std::cout << "subscribed to topic " << topic.id << " aka. " << topic.name() << std::endl;
}

void topicUnsubscribed(p2p::Topic topic) {
	std::cout << "unsubscribed to topic " << topic.id << " aka. " << topic.name() << std::endl;
}

void connected() {
	topicSubscribed(p2p::Network::get_singleton().defaultTopic);
	std::cout << "connected to the network!\n> " << std::flush;
}



struct Args : public argparse::Args {
	std::string& keyFile = kwarg("k,keyfile", "File path to the key identity").set_default("id.key");
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

	p2p::Network net(p2p::default_listen_address, "chat/debug/v1.0.0", key, connected);

	net.on_message += print;

	net.on_peer_connected += peerJoined;
	net.on_peer_disconnected += peerLeft;

	net.on_topic_subscribed += topicSubscribed;
	net.on_topic_unsubscribed += topicUnsubscribed;

	std::string line;
	while(true){
		std::cout << "> " << std::flush;
		std::getline(std::cin, line);

		bool success = net.broadcast_message(line);
	}
}