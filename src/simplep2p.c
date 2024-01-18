#include "libsimplep2p_golib.h"
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
bool bridge_void_callback(P2PNetwork n, void_callback f) {
	if(f == NULL) return true;
	return f(n);
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
bool bridge_msg_callback(P2PNetwork n, Message* m, msg_callback f) {
	if(f == NULL) return true;
	return f(n, m);
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
bool bridge_peer_callback(P2PNetwork n, char* p, peer_callback f) {
	if(f == NULL) return true;
	return f(n, p);
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
bool bridge_topic_callback(P2PNetwork n, int t, topic_callback f){
	if(f == NULL) return true;
	return f(n, t);
}

/**
 * @brief Sets the message callback function for P2P network.
 *
 * This function sets the message callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param network The network to manipulate.
 * @param callback The message callback function to set.
 */
void p2p_set_message_callback(P2PNetwork network, P2PMsgCallback callback) {
	setMessageCallback(network, (msg_callback)callback);
}

/**
 * @brief Sets the peer connected callback function for P2P network.
 *
 * This function sets the peer connected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param network The network to manipulate.
 * @param callback The peer connected callback function to set.
 */
void p2p_set_peer_connected_callback(P2PNetwork network, P2PPeerCallback callback) {
	setPeerConnectedCallback(network, (peer_callback)callback);
}

/**
 * @brief Sets the peer disconnected callback function for P2P network.
 *
 * This function sets the peer disconnected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param network The network to manipulate.
 * @param callback The peer disconnected callback function to set.
 */
void p2p_set_peer_disconnected_callback(P2PNetwork network, P2PPeerCallback callback) {
	setPeerDisconnectedCallback(network, callback);
}

/**
 * @brief Sets the topic subscribed callback function for P2P network.
 *
 * This function sets the topic subscribed callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param network The network to manipulate.
 * @param callback The topic subscribed callback function to set.
 */
void p2p_set_topic_subscribed_callback(P2PNetwork network, P2PTopicCallback callback) {
	setTopicSubscribedCallback(network, callback);
}

/**
 * @brief Sets the topic unsubscribed callback function for P2P network.
 *
 * This function sets the topic unsubscribed callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param network The network to manipulate.
 * @param callback The topic unsubscribed callback function to set.
 */
void p2p_set_topic_unsubscribed_callback(P2PNetwork network, P2PTopicCallback callback) {
	setTopicUnsubscribedCallback(network, callback);
}

/**
 * @brief Sets the connected callback function for P2P network.
 *
 * This function sets the connected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param network The network to manipulate.
 * @param callback The connected callback function to set.
 */
void p2p_set_connected_callback(P2PNetwork network, P2PVoidCallback callback) {
	setConnectedCallback(network, callback);
}

/**
 * @brief Sets the disconnected callback function for P2P network.
 *
 * This function sets the disconnected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param network The network to manipulate.
 * @param callback The disconnected callback function to set.
 */
void p2p_set_disconnected_callback(P2PNetwork network, P2PVoidCallback callback) {
	setDisconnectedCallback(network, callback);
}

/**
 * @brief Checks if the given network ID is (still) valid!
 *
 * @param network
 * @return true
 * @return false
 */
bool p2p_network_valid(P2PNetwork network);


/**
 * @brief Returns the provided string encoded as a base64 string
 * @note Sized version
 *
 * @param str The string to encode
 * @param size The length of the string
 * @return P2PString The base64 encoded version of str
 */
P2PString p2p_base64_encoden(const char* str, int size) {
	struct base64Encode_return encoded = base64Encode((char*)str, size);
	P2PString out;
	out.data = encoded.r0;
	out.size = encoded.r1;
	return out;
}

/**
 * @brief Returns the provided string encoded as a base64 string
 *
 * @param str The string to encode
 * @return P2PString The base64 encoded version of str
 */
P2PString p2p_base64_encode(const char* str) {
	return p2p_base64_encoden(str, strlen(str));
}

/**
 * @brief Returns the provided string decoded from a base64 string
 * @note Sized version
 *
 * @param str The string to decode
 * @param size The length of the string
 * @return P2PString The base64 decoded version of str
 */
P2PString p2p_base64_decoden(const char* str, int size) {
	GoString string;
	string.p = str;
	string.n = size;
	struct base64Decode_return encoded = base64Decode(string);
	P2PString out;
	out.data = encoded.r0;
	out.size = encoded.r1;
	return out;
}

/**
 * @brief Returns the provided string decoded from a base64 string
 *
 * @param str The string to decode
 * @return P2PString The base64 decoded version of str
 */
P2PString p2p_base64_decode(const char* str) {
	return p2p_base64_decoden(str, strlen(str));
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
	out.connectionTimeout = 60 /*seconds*/;
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
 * @param connectionTimeout The time (in seconds) to wait for a connection before giving up.
 * @param verbose The verbose flag.
 * @return The initialization arguments.
 */
P2PInitializationArguments p2p_initialize_args_from_strings(const char* listenAddress, const char* discoveryTopic, P2PKey identity, double connectionTimeout, bool verbose) {
	P2PInitializationArguments out;
	out.listenAddress = listenAddress;
	out.listenAddressSize = strlen(out.listenAddress);
	out.discoveryTopic = discoveryTopic;
	out.discoveryTopicSize = strlen(out.discoveryTopic);
	out.identity = identity;
	out.connectionTimeout = connectionTimeout;
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
	return initialize(listenAddress, discoveryTopic, key, args.connectionTimeout, args.verbose);
}

/**
 * @brief Shuts down P2P network.
 *
 * This function shuts down P2P network by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 */
void p2p_shutdown(P2PNetwork network) {
	shutdown(network);
}

/**
 * @brief Returns the local hashed ID for P2P network.
 *
 * This function returns the local hashed ID for P2P network by calling the corresponding Go function.
 *
 * @note This pointer is heap allocated and must be freed by the caller!
 * @param network The network to manipulate.
 * @return The local hashed ID.
 */
char* p2p_local_id(P2PNetwork network) {
	return localID(network);
}

/**
 * @brief Subscribes to a topic with the provided name limiting the string's size.
 *
 * This function subscribes to a topic with the provided name and size by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param name The name of the topic.
 * @param nameSize The size of the name.
 * @return The subscribed P2P Topic ID.
 */
P2PTopic p2p_subscribe_to_topicn(P2PNetwork network, const char* name, int nameSize) {
	GoString s;
	s.p = name;
	s.n = nameSize;
	return subscribeToTopic(network, s);
}

/**
 * @brief Subscribes to a topic with the provided name.
 *
 * This function subscribes to a topic with the provided name by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param name The name of the topic.
 * @return The subscribed P2P topic ID.
 */
P2PTopic p2p_subscribe_to_topic(P2PNetwork network, const char* name) {
	return p2p_subscribe_to_topicn(network, name, strlen(name));
}

/**
 * @brief Finds a topic with the provided name limiting the string's size.
 *
 * This function finds a topic with the provided name and size by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param name The name of the topic.
 * @param nameSize The size of the name.
 * @return The found P2P Topic ID.
 */
P2PTopic p2p_find_topicn(P2PNetwork network, const char* name, int nameSize) {
	GoString s;
	s.p = name;
	s.n = nameSize;
	return findTopic(network, s);
}

/**
 * @brief Finds a topic with the provided name.
 *
 * This function finds a topic with the provided name by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param name The name of the topic.
 * @return The found P2P Topic ID.
 */
P2PTopic p2p_find_topic(P2PNetwork network, const char* name) {
	return p2p_find_topicn(network, name, strlen(name));
}

/**
 * @brief Returns the name of the specified P2P topic.
 *
 * This function returns the name of the specified P2P topic by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @note The returned string is heap generated and must be freed by the caller
 * @param topicID The P2P Topic ID.
 * @return The name of the P2P topic.
 */
char* p2p_topic_name(P2PNetwork network, P2PTopic topicID) {
	return topicString(network, topicID);
}

/**
 * @brief Leaves the specified P2P topic.
 *
 * This function leaves the specified P2P topic by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param topicID The P2P topic ID to leave.
 * @return True if successfully left the topic, false otherwise.
 */
bool p2p_leave_topic(P2PNetwork network, P2PTopic topicID) {
	return leaveTopic(network, topicID);
}

/**
 * @brief Broadcasts a message to the specified P2P topic with the provided message limited to size.
 *
 * This function broadcasts a message to the specified P2P topic with the provided message and size by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param message The message to broadcast.
 * @param messageSize The size of the message.
 * @param topicID The P2P topic ID to broadcast the message to.
 * @return True if the message was successfully broadcasted, false otherwise.
 */
bool p2p_broadcast_messagen(P2PNetwork network, const char* message, int messageSize, P2PTopic topicID) {
	GoString m;
	m.p = message;
	m.n = messageSize;
	return broadcastMessage(network, m, topicID);
}

/**
 * @brief Broadcasts a message to the specified P2P topic with the provided message.
 *
 * This function broadcasts a message to the specified P2P topic with the provided message by calling the corresponding Go function.
 *
 * @param network The network to manipulate.
 * @param message The message to broadcast.
 * @param topicID The P2P topic ID to broadcast the message to.
 * @return True if the message was successfully broadcasted, false otherwise.
 */
bool p2p_broadcast_message(P2PNetwork network, const char* message, P2PTopic topicID) {
	return p2p_broadcast_messagen(network, message, strlen(message), topicID);
}

#ifdef __cplusplus
} // extern "C"
#endif