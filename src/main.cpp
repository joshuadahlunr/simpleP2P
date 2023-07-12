#include <argparse/argparse.hpp>
#include <iostream>
#include <thread>

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



struct Args : public argparse::Args {
	int& sourcePort = kwarg("p,sp", "Source port number").set_default(0);
	std::string& dest = kwarg("d,destination", "Destination multiaddr string").set_default("");
	bool& debug = flag("debug", "Debug generates the same node ID on every execution");
};

int main(int argc, char* argv[]) {
	Args args = argparse::parse<Args>(argc, argv);

	setMessageCallback(print);

	setPeerConnectedCallback(peerJoined);
	setPeerDisconnectedCallback(peerLeft);

	setTopicSubscribedCallback(topicSubscribed);
	setTopicUnsubscribedCallback(topicUnsubscribed);

	std::string_view topic = "chat/debug/v1.0.3";
	std::string_view listen = "/ip4/0.0.0.0/udp/0/quic-v1";
	auto id = initialize({topic.data(), (long)topic.size()}, {listen.data(), (long)listen.size()});

	std::cout << "listening to topic: " << id << std::endl;

	std::string line;
	while(true){
		std::cout << "> " << std::flush;
		std::getline(std::cin, line);

		bool success = broadcastMessage({line.data(), (long)line.size()}, id);
	}

	std::cout << "Will this ever run?" << std::endl;
}