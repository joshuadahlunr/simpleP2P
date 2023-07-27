#ifndef SIMPLE_P2P_NETWORKING_H
#define SIMPLE_P2P_NETWORKING_H

#ifdef __cplusplus
extern "C" {
#else
	#include <stdbool.h>
#endif

/**
 * @struct P2PMessage
 * @brief Structure representing a P2P message.
 *
 * This structure holds the fields of a P2P message, including 'from', 'data', 'seqno', 'topic', 'signature', 'key', 'id', and 'received_from'.
 */
typedef struct {
	char* from;             ///< ???
	char* data;             ///< The content of the message.
	char* seqno;            ///< The sequence number of the message.
	char* topic;            ///< The topic of the message.
	char* signature;        ///< ???
	char* key;              ///< The key of the message.
	char* id;               ///< The ID of the message.
	char* received_from;    ///< The sender of the message.
} P2PMessage;

/**
 * @typedef P2PTopic
 * @brief Alias for the P2P topic.
 */
typedef int P2PTopic;


// Definitions of the callback types used by the callback functions
typedef bool (*P2PMsgCallback)(P2PMessage*);
typedef bool (*P2PVoidCallback)();
typedef bool (*P2PPeerCallback)(char*);
typedef bool (*P2PTopicCallback)(P2PTopic);


/**
 * @struct P2PKey
 * @brief Structure representing a P2P cryptopgraphic private key.
 *
 * This structure holds the data and size of a P2P private key used for identification.
 */
typedef struct {
	char* data;
	int size;
} P2PKey;


/**
 * @brief Generates a P2P key.
 *
 * This function generates a P2P key by calling the corresponding Go function and returns the generated key.
 *
 * @note the data pointer is heap allocated and needs to be freed by the caller
 * @return The generated P2P key.
 */
P2PKey p2p_generate_key();

/**
 * @brief Returns a null P2P key.
 *
 * This function returns a null P2P key with data set to NULL and size set to 0.
 *
 * @return The null P2P key.
 */
P2PKey p2p_null_key();


/**
 * @struct P2PInitializationArguments
 * @brief Structure representing the initialization arguments for P2P networking.
 *
 * This structure holds the initialization arguments for P2P networking, including the listen address, listen address size, discovery topic, discovery topic size, P2P key identity, and verbose flag.
 */
typedef struct {
	const char* listenAddress;          ///< The listen address.
	long long listenAddressSize;        ///< The size of the listen address.
	const char* discoveryTopic;         ///< The discovery topic.
	long long discoveryTopicSize;       ///< The size of the discovery topic.
	P2PKey identity;                    ///< The P2P key identity.
	double connectionTimeout;			///< The time in seconds to try connecting before giving up
	bool verbose;                       ///< The verbose flag.
} P2PInitializationArguments;

/**
 * @brief Returns the default initialization arguments for P2P network.
 *
 * This function returns the default initialization arguments for P2P network.
 *
 * @return The default initialization arguments.
 */
P2PInitializationArguments p2p_default_initialize_args();

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
P2PInitializationArguments p2p_initialize_args_from_strings(const char* listenAddress, const char* discoveryTopic, P2PKey identity, double connectionTimeout, bool verbose);

/**
 * @brief Initializes P2P network with the provided arguments.
 *
 * This function initializes P2P network with the provided arguments by calling the corresponding Go function.
 *
 * @param args The initialization arguments.
 * @return The initial P2P Topic ID.
 */
P2PTopic p2p_initialize(P2PInitializationArguments args);

/**
 * @brief Shuts down P2P network.
 *
 * This function shuts down P2P network by calling the corresponding Go function.
 */
void p2p_shutdown();

/**
 * @brief Returns the local hashed ID for P2P network.
 *
 * This function returns the local hashed ID for P2P network by calling the corresponding Go function.
 *
 * @note This pointer is heap allocated and must be freed by the caller!
 * @return The local hashed ID.
 */
char* p2p_local_id();

/**
 * @brief Subscribes to a topic with the provided name.
 *
 * This function subscribes to a topic with the provided name by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @return The subscribed P2P topic ID.
 */
P2PTopic p2p_subscribe_to_topic(const char* name);

/**
 * @brief Subscribes to a topic with the provided name limiting the string's size.
 *
 * This function subscribes to a topic with the provided name and size by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @param nameSize The size of the name.
 * @return The subscribed P2P Topic ID.
 */
P2PTopic p2p_subscribe_to_topicn(const char* name, int nameSize);

/**
 * @brief Finds a topic with the provided name.
 *
 * This function finds a topic with the provided name by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @return The found P2P Topic ID.
 */
P2PTopic p2p_find_topic(const char* name);

/**
 * @brief Finds a topic with the provided name limiting the string's size.
 *
 * This function finds a topic with the provided name and size by calling the corresponding Go function.
 *
 * @param name The name of the topic.
 * @param nameSize The size of the name.
 * @return The found P2P Topic ID.
 */
P2PTopic p2p_find_topicn(const char* name, int nameSize);

/**
 * @brief Returns the name of the specified P2P topic.
 *
 * This function returns the name of the specified P2P topic by calling the corresponding Go function.
 *
 * @note The returned string is heap generated and must be freed by the caller
 * @param topicID The P2P Topic ID.
 * @return The name of the P2P topic.
 */
char* p2p_topic_name(P2PTopic topicID);

/**
 * @brief Leaves the specified P2P topic.
 *
 * This function leaves the specified P2P topic by calling the corresponding Go function.
 *
 * @param topicID The P2P topic ID to leave.
 * @return True if successfully left the topic, false otherwise.
 */
bool p2p_leave_topic(P2PTopic id);

/**
 * @brief Broadcasts a message to the specified P2P topic with the provided message.
 *
 * This function broadcasts a message to the specified P2P topic with the provided message by calling the corresponding Go function.
 *
 * @param message The message to broadcast.
 * @param topicID The P2P topic ID to broadcast the message to.
 * @return True if the message was successfully broadcasted, false otherwise.
 */
bool p2p_broadcast_message(const char* message, P2PTopic topicID);

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
bool p2p_broadcast_messagen(const char* message, int messageSize, P2PTopic topicID);



// Callback Setters



/**
 * @brief Sets the message callback function for P2P network.
 *
 * This function sets the message callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param callback The message callback function to set.
 */
void p2p_set_message_callback(P2PMsgCallback callback);

/**
 * @brief Sets the peer connected callback function for P2P network.
 *
 * This function sets the peer connected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param callback The peer connected callback function to set.
 */
void p2p_set_peer_connected_callback(P2PPeerCallback callback);

/**
 * @brief Sets the peer disconnected callback function for P2P network.
 *
 * This function sets the peer disconnected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @note the data passed to the callback is freed as soon as this returns... if you need it to stick around longer you must copy it!
 * @param callback The peer disconnected callback function to set.
 */
void p2p_set_peer_disconnected_callback(P2PPeerCallback callback);

/**
 * @brief Sets the topic subscribed callback function for P2P network.
 *
 * This function sets the topic subscribed callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The topic subscribed callback function to set.
 */
void p2p_set_topic_subscribed_callback(P2PTopicCallback callback);

/**
 * @brief Sets the topic unsubscribed callback function for P2P network.
 *
 * This function sets the topic unsubscribed callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The topic unsubscribed callback function to set.
 */
void p2p_set_topic_unsubscribed_callback(P2PTopicCallback callback);

/**
 * @brief Sets the connected callback function for P2P network.
 *
 * This function sets the connected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The connected callback function to set.
 */
void p2p_set_connected_callback(P2PVoidCallback callback);

/**
 * @brief Sets the disconnected callback function for P2P network.
 *
 * This function sets the disconnected callback function for P2P network. It bridges the provided callback function from C to Go.
 *
 * @param callback The disconnected callback function to set.
 */
void p2p_set_disconnected_callback(P2PVoidCallback callback);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SIMPLE_P2P_NETWORKING_H