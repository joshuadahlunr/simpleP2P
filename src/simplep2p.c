#include "libgolib.h"
#include "simplep2p.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

/**
 * @brief Bridges a void callback function from C to Go.
 *
 * This function bridges a void callback function from C to Go. It checks if the function is NULL and then invokes it.
 *
 * @param f The void callback function to bridge.
 * @return True if everything went well, False if Go should panic
 */
bool bridge_void_callback(void_callback f) {
	if(f == NULL) return true;
	return f();
}

/**
 * @brief Bridges a message callback function from C to Go.
 *
 * This function bridges a message callback function from C to Go. It checks if the function is NULL and then invokes it with the provided message.
 *
 * @param m The message to pass to the callback function.
 * @param f The message callback function to bridge.
 * @return True if everything went well, False if Go should panic
 */
bool bridge_msg_callback(Message* m, msg_callback f) {
	if(f == NULL) return true;
	return f(m);
}

/**
 * @brief Bridges a peer callback function from C to Go.
 *
 * This function bridges a peer callback function from C to Go. It checks if the function is NULL and then invokes it with the provided peer.
 *
 * @param p The peer to pass to the callback function.
 * @param f The peer callback function to bridge.
 * @return True if everything went well, False if Go should panic
 */
bool bridge_peer_callback(char* p, peer_callback f) {
	if(f == NULL) return true;
	return f(p);
}

/**
 * @brief Bridges a topic callback function from C to Go.
 *
 * This function bridges a topic callback function from C to Go. It checks if the function is NULL and then invokes it with the provided topic.
 *
 * @param t The topic to pass to the callback function.
 * @param f The topic callback function to bridge.
 * @return True if everything went well, False if Go should panic
 */
bool bridge_topic_callback(int t, topic_callback f){
	if(f == NULL) return true;
	return f(t);
}

/**
 * @brief Sets the message callback function for P2P network.
 *
 * This function sets the message callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param callback The message callback function to set.
 */
void p2p_set_message_callback(P2PMsgCallback callback) {
	setMessageCallback((msg_callback)callback);
}

/**
 * @brief Sets the peer connected callback function for P2P network.
 *
 * This function sets the peer connected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param callback The peer connected callback function to set.
 */
void p2p_set_peer_connected_callback(P2PPeerCallback callback) {
	setPeerConnectedCallback((peer_callback)callback);
}

/**
 * @brief Sets the peer disconnected callback function for P2P network.
 *
 * This function sets the peer disconnected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param callback The peer disconnected callback function to set.
 */
void p2p_set_peer_disconnected_callback(P2PPeerCallback callback) {
	setPeerDisconnectedCallback(callback);
}

/**
 * @brief Sets the topic subscribed callback function for P2P network.
 *
 * This function sets the topic subscribed callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The topic subscribed callback function to set.
 */
void p2p_set_topic_subscribed_callback(P2PTopicCallback callback) {
	setTopicSubscribedCallback(callback);
}

/**
 * @brief Sets the topic unsubscribed callback function for P2P network.
 *
 * This function sets the topic unsubscribed callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The topic unsubscribed callback function to set.
 */
void p2p_set_topic_unsubscribed_callback(P2PTopicCallback callback) {
	setTopicUnsubscribedCallback(callback);
}

/**
 * @brief Sets the connected callback function for P2P network.
 *
 * This function sets the connected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The connected callback function to set.
 */
void p2p_set_connected_callback(P2PVoidCallback callback) {
	setConnectedCallback(callback);
}

/**
 * @brief Sets the disconnected callback function for P2P network.
 *
 * This function sets the disconnected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The disconnected callback function to set.
 */
void p2p_set_disconnected_callback(P2PVoidCallback callback) {
	setDisconnectedCallback(callback);
}

/**
 * @brief Generates a P2P key.
 *
 * This function generates a P2P key by calling the corresponding Go function and returns the generated key.
 *
 * @note the data pointer is heap allocated and needs to be freed by the caller
 * @return The generated P2P key.
 */
P2PKey p2p_generate_key() {
	struct generateCKey_return result = generateCKey();
	P2PKey key;
	key.data = result.r0;
	key.size = result.r1;
	return key;
}


/**
 * @brief Returns a null P2P key.
 *
 * This function returns a null P2P key with data set to NULL and size set to 0.
 *
 * @return The null P2P key.
 */
P2PKey p2p_null_key() {
	P2PKey key;
	key.data = NULL;
	key.size = 0;
	return key;
}

/**
 * @brief Returns the default initialization arguments for P2P network.
 *
 * This function returns the default initialization arguments for P2P network.
 *
 * @return The default initialization arguments.
 */
P2PInitializationArguments p2p_default_initialize_args() {
	P2PInitializationArguments out;
	out.listenAddress = "/ip4/0.0.0.0/udp/0/quic-v1";
	out.listenAddressSize = strlen(out.listenAddress);
	out.discoveryTopic = "simpleP2P";
	out.discoveryTopicSize = strlen(out.discoveryTopic);
	out.identity = p2p_null_key();
	out.verbose = false;
	return out;
}

/**
 * @brief Returns the initialization arguments for P2P network based on provided strings.
 *
 * This function returns the initialization arguments for P2P network based on provided strings.
 *
 * @param listenAddress The listen address string.
 * @param discoveryTopic The discovery topic string.
 * @param identity The P2P key identity.
 * @param verbose The verbose flag.
 * @return The initialization arguments.
 */
P2PInitializationArguments p2p_initialize_args_from_strings(const char* listenAddress, const char* discoveryTopic, P2PKey identity, bool verbose) {
	P2PInitializationArguments out;
	out.listenAddress = listenAddress;
	out.listenAddressSize = strlen(out.listenAddress);
	out.discoveryTopic = discoveryTopic;
	out.discoveryTopicSize = strlen(out.discoveryTopic);
	out.identity = identity;
	out.verbose = verbose;
	return out;
}

/**
 * @brief Initializes P2P network with the provided arguments.
 *
 * This function initializes P2P network with the provided arguments by calling the corresponding Go function.
 *
 * @param args The initialization arguments.
 * @return The initial P2P Topic ID.
 */
P2PTopic p2p_initialize(P2PInitializationArguments args) {
	GoString listenAddress;
	listenAddress.p = args.listenAddress;
	listenAddress.n = args.listenAddressSize;
	GoString discoveryTopic;
	discoveryTopic.p = args.discoveryTopic;
	discoveryTopic.n = args.discoveryTopicSize;
	GoString key;
	key.p = args.identity.data;
	key.n = args.identity.size;
	return initialize(listenAddress, discoveryTopic, key, args.verbose);
}

/**
 * @brief Shuts down P2P network.
 *
 * This function shuts down P2P network by calling the corresponding Go function.
 */
void p2p_shutdown() {
	shutdown();
}

/**
 * @brief Returns the local hashed ID for P2P network.
 *
 * This function returns the local hashed ID for P2P network by calling the corresponding Go function.
 *
 * @note This pointer is heap allocated and must be freed by the caller!
 * @return The local hashed ID.
 */
char* p2p_local_id() {
	return localID();
}

/**
 * @brief Subscribes to a topic with the provided name limiting the string's size.
 *
 * This function subscribes to a topic with the provided name and size by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @param nameSize The size of the name.
 * @return The subscribed P2P Topic ID.
 */
P2PTopic p2p_subscribe_to_topicn(const char* name, int nameSize) {
	GoString s;
	s.p = name;
	s.n = nameSize;
	return subscribeToTopic(s);
}

/**
 * @brief Subscribes to a topic with the provided name.
 *
 * This function subscribes to a topic with the provided name by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @return The subscribed P2P topic ID.
 */
P2PTopic p2p_subscribe_to_topic(const char* name) {
	return p2p_subscribe_to_topicn(name, strlen(name));
}

/**
 * @brief Finds a topic with the provided name limiting the string's size.
 *
 * This function finds a topic with the provided name and size by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @param nameSize The size of the name.
 * @return The found P2P Topic ID.
 */
P2PTopic p2p_find_topicn(const char* name, int nameSize) {
	GoString s;
	s.p = name;
	s.n = nameSize;
	return findTopic(s);
}

/**
 * @brief Finds a topic with the provided name.
 *
 * This function finds a topic with the provided name by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @return The found P2P Topic ID.
 */
P2PTopic p2p_find_topic(const char* name) {
	return p2p_find_topicn(name, strlen(name));
}

/**
 * @brief Returns the name of the specified P2P topic.
 *
 * This function returns the name of the specified P2P topic by calling the corresponding Go function.
 *
 * @note The returned string is heap generated and must be freed by the caller
 * @param topicID The P2P Topic ID.
 * @return The name of the P2P topic.
 */
char* p2p_topic_name(P2PTopic topicID) {
	return topicString(topicID);
}

/**
 * @brief Leaves the specified P2P topic.
 *
 * This function leaves the specified P2P topic by calling the corresponding Go function.
 *
 * @param topicID The P2P topic ID to leave.
 * @return True if successfully left the topic, false otherwise.
 */
bool p2p_leave_topic(P2PTopic topicID) {
	return leaveTopic(topicID);
}

/**
 * @brief Broadcasts a message to the specified P2P topic with the provided message limited to size.
 *
 * This function broadcasts a message to the specified P2P topic with the provided message and size by calling the corresponding Go function.
 *
 * @param message The message to broadcast.
 * @param messageSize The size of the message.
 * @param topicID The P2P topic ID to broadcast the message to.
 * @return True if the message was successfully broadcasted, false otherwise.
 */
bool p2p_broadcast_messagen(const char* message, int messageSize, P2PTopic topicID) {
	GoString m;
	m.p = message;
	m.n = messageSize;
	return broadcastMessage(m, topicID);
}

/**
 * @brief Broadcasts a message to the specified P2P topic with the provided message.
 *
 * This function broadcasts a message to the specified P2P topic with the provided message by calling the corresponding Go function.
 *
 * @param message The message to broadcast.
 * @param topicID The P2P topic ID to broadcast the message to.
 * @return True if the message was successfully broadcasted, false otherwise.
 */
bool p2p_broadcast_message(const char* message, P2PTopic topicID) {
	return p2p_broadcast_messagen(message, strlen(message), topicID);
}

#ifdef __cplusplus
} // extern "C"
#endif