#ifndef SIMPLE_P2P_NETWORKING_HPP
#define SIMPLE_P2P_NETWORKING_HPP

#include "delegate.hpp"

#include <string_view>
#include <span>
#include <iostream>
#include <vector>
#include <optional>
#include <chrono>


namespace p2p {
	#include "simplep2p.h"

	/**
	 * @brief Alias equating a PeerID and a string
	 */
	struct PeerID: public std::string {
		using std::string::string;
		using std::string::operator=;
		PeerID(const std::string& o) : std::string(o) {}
		PeerID(std::string&& o) : std::string(std::move(o)) {}
		PeerID(const PeerID&) = default;
		PeerID(PeerID&&) = default;
		PeerID& operator=(const PeerID&) = default;
		PeerID& operator=(PeerID&&) = default;

		using view = std::string_view;
	};

	/**
	 * @brief Constant to indicate that initialization should not be performed automatically.
	 */
	constexpr bool do_not_initialize = false;

	/**
	 * @brief Default listen address used for network initialization.
	 */
	constexpr std::string_view default_listen_address = "/ip4/0.0.0.0/udp/0/quic-v1";

	/**
	 * @brief Default discovery topic used for network initialization.
	 */
	constexpr std::string_view default_discovery_topic = "simpleP2P";

	/**
	 * @class Key
	 * @brief Represents a P2P cryptographic private key.
	 */
	class Key {
		std::vector<std::byte> data;

		/**
		 * @brief Frees the underlying memory of the P2PKey structure.
		 * @param base The P2PKey structure to free the memory for.
		 */
		static void freeBase(P2PKey& base) { free((void*)base.data); base.data = nullptr; base.size = 0; }
	public:
		/**
		 * @brief Default constructor.
		 */
		Key() = default;

		/**
		 * @brief Move constructor.
		 * @param o The Key object to move from.
		 */
		Key(P2PKey&& o) : data((std::byte*)o.data, ((std::byte*)o.data) + o.size) { freeBase(o); }

		/**
		 * @brief Copy constructor.
		 * @param o The Key object to copy from.
		 */
		Key(const Key&) = default;

		/**
		 * @brief Move constructor.
		 * @param o The Key object to move from.
		 */
		Key(Key&&) = default;

		/**
		 * @brief Move assignment operator.
		 * @param o The Key object to move from.
		 * @return Reference to the assigned Key object.
		 */
		Key& operator=(P2PKey&& o) { data = std::vector<std::byte>((std::byte*)o.data, ((std::byte*)o.data) + o.size); freeBase(o); return *this; }

		/**
		 * @brief Copy assignment operator.
		 * @param o The Key object to copy from.
		 * @return Reference to the assigned Key object.
		 */
		Key& operator=(const Key&) = default;

		/**
		 * @brief Move assignment operator.
		 * @param o The Key object to move from.
		 * @return Reference to the assigned Key object.
		 */
		Key& operator=(Key&&) = default;

		/**
		 * @brief Conversion operator to P2PKey.
		 * @return The underlying P2PKey structure.
		 */
		operator P2PKey() { return {(char*)data.data(), (int)data.size()}; }

		/**
		 * @brief Conversion operator to P2PKey (const version).
		 * @return The underlying P2PKey structure.
		 */
		operator P2PKey() const { return {(char*)data.data(), (int)data.size()}; }

		/**
		 * @brief Saves the Key object to an output stream.
		 * @param out The output stream to save to.
		 * @return Reference to the output stream.
		 */
		std::ostream& save(std::ostream& out) const {
			auto size = data.size();
			out.write(reinterpret_cast<char*>(&size), sizeof(size));
			out.write((char*)(data.data()), size);
			return out;
		}

		/**
		 * @brief Loads the Key object from an input stream.
		 * @param in The input stream to load from.
		 * @return Reference to the input stream.
		 */
		std::istream& load(std::istream& in) {
			auto size = data.size();
			in.read(reinterpret_cast<char*>(&size), sizeof(size));
			data.resize(size);
			in.read(reinterpret_cast<char*>(data.data()), size);
			return in;
		}

		/**
		 * @brief Generates a new P2P key.
		 * @return The generated Key object.
		 */
		static Key generate() { return p2p_generate_key(); }
	};

	/**
	 * @struct Topic
	 * @brief Represents a P2P topic.
	 */
	struct Topic {
		P2PNetwork network;
		P2PTopic id;

		/**
		 * @brief Checks if the Topic object is valid.
		 * @return True if the topic is valid, false otherwise.
		 */
		bool valid() const { return id >= 0; }
		operator bool() const { return valid(); }

		/**
		 * @brief Gets the name of the topic.
		 * @return The name of the topic.
		 */
		std::string name() const {
			auto raw = p2p_topic_name(network, id);
			std::string out = raw;
			free((char*)raw);
			return out;
		}

		/**
		 * @brief Leaves the topic.
		 * @return True if successfully left the topic, false otherwise.
		 */
		bool leave() { return p2p_leave_topic(network, id); }
	};

	/**
	 * @class Network
	 * @brief Represents the P2P network.
	 */
	class Network {
		static std::map<P2PNetwork, Network*> networks;
		friend class Message;
	public:
		/**
		 * @brief the id of the underlying network the methods on this object manipulate
		 */
		P2PNetwork network = p2p_initial_network();

		/**
		 * @brief the default topic that the network originally joined 
		 */
		Topic defaultTopic;

		// Multicast delegates representing the different network events
		delegate<void(Network&, struct Message&)> on_message;
		delegate<void(Network&, PeerID::view)> on_peer_connected; // Note: only called for directly connected peers... if you need all peers work at a higher level!
		delegate<void(Network&, PeerID::view)> on_peer_disconnected;
		delegate<void(Network&, Topic)> on_topic_subscribed;
		delegate<void(Network&, Topic)> on_topic_unsubscribed;
		delegate<void(Network&)> on_connected;
		delegate<void(Network&)> on_disconnected;

		/**
		 * @brief Constructor that initializes the P2P network connection.
		 * @param listenAddress The multiaddress we should listen for connections on.
		 * @param discoveryTopic The discovery topic for network initialization.
		 * @param identityKey The identity key for network initialization.
		 * @param do_on_connected Callback function to register in on_connected before initializing the connection
		 * @param connectionTimeout The time to wait for a connection before giving up.
		 * @param verbose Flag indicating if the GO library should spew some more verbose messages.
		 */
		Network(
			std::string_view listenAddress = default_listen_address,
			std::string_view discoveryTopic = default_discovery_topic,
			const Key& identityKey = {},
			delegate_function<void(Network&)> do_on_connected = nullptr,
			std::chrono::milliseconds connectionTimeout = std::chrono::seconds(60),
			bool verbose = false
		) {
			if(do_on_connected != nullptr)
				on_connected = do_on_connected;

			initialize(listenAddress, discoveryTopic, identityKey, connectionTimeout, verbose);
		}

		/**
		 * @brief Constructor for not automatically initializing the network.
		 * @param dontInit Placeholder argument to differentiate from the other constructor.
		 */
		Network(bool dontInit) { }

		/**
		 * @brief Destructor.
		 */
		~Network() { shutdown(); networks.erase(network); }

		/**
		 * @brief Copy constructor (deleted).
		 */
		Network(const Network&) = delete;

		/**
		 * @brief Move constructor (deleted).
		 */
		Network(const Network&&) = delete;
		// Network(Network&& o) : network(o.network), defaultTopic(o.defaultTopic), 
		// 	on_message(o.on_message),
		// 	on_peer_connected(o.on_peer_connected),
		// 	on_peer_disconnected(o.on_peer_disconnected),
		// 	on_topic_subscribed(o.on_topic_subscribed),
		// 	on_topic_unsubscribed(o.on_topic_unsubscribed),
		// 	on_connected(o.on_connected),
		// 	on_disconnected(o.on_disconnected) { networks[network] = this; }

		/**
		 * @brief Initializes the P2P network connection.
		 * @param listenAddress The multiaddress we should listen for connections on.
		 * @param discoveryTopic The discovery topic for network initialization.
		 * @param identityKey The identity key for network initialization.
		 * @param connectionTimeout The time to wait for a connection before giving up.
		 * @param verbose Flag indicating if the GO library should spew some more verbose messages.
		 */
		void initialize(
			std::string_view listenAddress = default_listen_address,
			std::string_view discoveryTopic = default_discovery_topic,
			const Key& identityKey = {},
			std::chrono::milliseconds connectionTimeout = std::chrono::seconds(60),
			bool verbose = false
		) {
			// Connect the connect delegate to its callback
			override_connected_callback(on_connected_impl);

			// Initialize the GO library!
			network = p2p_initialize({
				.listenAddress = listenAddress.data(),
				.listenAddressSize = (long long)listenAddress.size(),
				.discoveryTopic = discoveryTopic.data(),
				.discoveryTopicSize = (long long)discoveryTopic.size(),
				.identity = identityKey,
				.connectionTimeout = std::chrono::duration_cast<std::chrono::duration<double>>(connectionTimeout).count(),
				.verbose = verbose
			});

			// Connect the delegates to the callbacks
			override_message_callback(on_mesage_impl);
			override_disconnected_callback(on_disconnected_impl);
			override_peer_connected_callback(on_peer_connected_impl);
			override_peer_disconnected_callback(on_peer_disconnected_impl);
			override_topic_subscribed_callback(on_topic_subscribed_impl);
			override_topic_unsubscribed_callback(on_topic_unsubscribed_impl);

			defaultTopic = { network, p2p_default_topic(network) };

			networks[network] = this;
		}

		/**
		 * @brief Shuts down the network connection.
		 */
		void shutdown() { p2p_shutdown(network); }

		/**
		 * @brief Gets the local hashed ID for the P2P network.
		 * @return The local hashed ID.
		 */
		PeerID local_id() const {
			auto raw = p2p_local_id(network);
			std::string out = raw;
			free((char*)raw);
			return out;
		}

		/**
		 * @brief Subscribes to a topic with the provided name.
		 * @param name The name of the topic to subscribe to.
		 * @return The Topic object representing the subscribed topic.
		 */
		Topic subscribe_to_topic(std::string_view name) { return { network, p2p_subscribe_to_topicn(network, name.data(), name.size()) }; }

		/**
		 * @brief Finds a topic by name.
		 * @param name The name of the topic to find.
		 * @return The Topic object representing the found topic.
		 */
		Topic find_topic(std::string_view name) { return { network, p2p_find_topicn(network, name.data(), name.size()) }; }

		/**
		 * @brief Broadcasts a message to a topic.
		 * @param message The message to broadcast.
		 * @param topic The Topic object representing the target topic.
		 * @return True if the message was successfully broadcasted, false otherwise.
		 */
		bool broadcast_message(std::string_view message, Topic topic) const { return p2p_broadcast_messagen(network, message.data(), message.size(), topic.id); }

		/**
		 * @brief Broadcasts a message to the default topic.
		 * @param message The message to broadcast.
		 * @return True if the message was successfully broadcasted, false otherwise.
		 */
		bool broadcast_message(std::string_view message) const { return broadcast_message(message, defaultTopic); }

		/**
		 * @brief Broadcasts a byte-span message to a topic.
		 * @param message The byte-span message to broadcast.
		 * @param topic The Topic object representing the target topic.
		 * @return True if the message was successfully broadcasted, false otherwise.
		 */
		bool broadcast_message(std::span<std::byte> message, Topic topic) const { return p2p_broadcast_messagen(network, (char*)message.data(), message.size(), topic.id); }

		/**
		 * @brief Broadcasts a byte-span message to the default topic.
		 * @param message The byte-span message to broadcast.
		 * @return True if the message was successfully broadcasted, false otherwise.
		 */
		bool broadcast_message(std::span<std::byte> message) const { return broadcast_message(message, defaultTopic); }

	protected:
		/**
		 * @brief Overrides the message callback with the provided function pointer.
		 * @param callback The function pointer to the message callback.
		 */
		void override_message_callback(P2PMsgCallback callback) { p2p_set_message_callback(network, callback); }

		/**
		 * @brief Overrides the peer connected callback with the provided function pointer.
		 * @param callback The function pointer to the peer connected callback.
		 */
		void override_peer_connected_callback(P2PPeerCallback callback) { p2p_set_peer_connected_callback(network, callback); }

		/**
		 * @brief Overrides the peer disconnected callback with the provided function pointer.
		 * @param callback The function pointer to the peer disconnected callback.
		 */
		void override_peer_disconnected_callback(P2PPeerCallback callback) { p2p_set_peer_disconnected_callback(network, callback); }

		/**
		 * @brief Overrides the topic subscribed callback with the provided function pointer.
		 * @param callback The function pointer to the topic subscribed callback.
		 */
		void override_topic_subscribed_callback(P2PTopicCallback callback) { p2p_set_topic_subscribed_callback(network, callback); }

		/**
		 * @brief Overrides the topic unsubscribed callback with the provided function pointer.
		 * @param callback The function pointer to the topic unsubscribed callback.
		 */
		void override_topic_unsubscribed_callback(P2PTopicCallback callback) { p2p_set_topic_unsubscribed_callback(network, callback); }

		/**
		 * @brief Overrides the connected callback with the provided function pointer.
		 * @param callback The function pointer to the connected callback.
		 */
		void override_connected_callback(P2PVoidCallback callback) { p2p_set_connected_callback(network, callback); }

		/**
		 * @brief Overrides the disconnected callback with the provided function pointer.
		 * @param callback The function pointer to the disconnected callback.
		 */
		void override_disconnected_callback(P2PVoidCallback callback) { p2p_set_disconnected_callback(network, callback); }

	private:
		static bool on_mesage_impl(P2PNetwork n, P2PMessage* msg) {
			Network& network = *networks[n];
			if(!network.on_message.empty())
				network.on_message(network, *reinterpret_cast<struct Message*>(msg));
			return true; // Go should never panic!
		}

		static bool on_peer_connected_impl(P2PNetwork n, char* peerID) {
			Network& network = *networks[n];
			if(!network.on_peer_connected.empty())
				network.on_peer_connected(network, peerID);
			return true; // Go should never panic!
		}

		static bool on_peer_disconnected_impl(P2PNetwork n, char* peerID) {
			Network& network = *networks[n];
			if(!network.on_peer_disconnected.empty())
				network.on_peer_disconnected(network, peerID);
			return true; // Go should never panic!
		}

		static bool on_topic_subscribed_impl(P2PNetwork n, P2PTopic topicID) {
			Network& network = *networks[n];
			if(!network.on_topic_subscribed.empty())
				network.on_topic_subscribed(network, {network.network, topicID});
			return true; // Go should never panic!
		}

		static bool on_topic_unsubscribed_impl(P2PNetwork n, P2PTopic topicID) {
			Network& network = *networks[n];
			if(!network.on_topic_unsubscribed.empty())
				network.on_topic_unsubscribed(network, {network.network, topicID});
			return true; // Go should never panic!
		}

		static bool on_connected_impl(P2PNetwork n) {
			Network& network = *networks[n];
			if(!network.on_connected.empty())
				network.on_connected(network);
			return true; // Go should never panic!
		}

		static bool on_disconnected_impl(P2PNetwork n) {
			Network& network = *networks[n];
			if(!network.on_disconnected.empty())
				network.on_disconnected(network);
			return true; // Go should never panic!
		}
	};

#ifdef SIMPLE_P2P_IMPLEMENTATION
	std::map<P2PNetwork, Network*> Network::networks;
#endif

	/**
	 * @struct Message
	 * @brief Represents a P2P message.
	 */
	struct Message: private P2PMessage {
		/**
		 * @brief finds the network originating this message.
		 * @return a reference to that network.
		 */
		Network& lookup_network() { return *Network::networks[P2PMessage::network];}

		/**
		 * @brief Gets the sender of the message.
		 * @return The sender's ID.
		 */
		PeerID::view sender() { return received_from; }

		/**
		 * @brief Gets the message data as a string view.
		 * @return The message data as a string view.
		 */
		std::string_view data_string() { return P2PMessage::data; }

		/**
		 * @brief Gets the message data as a byte span.
		 * @return The message data as a byte span.
		 */
		std::span<std::byte> data() { auto view = data_string(); return { (std::byte*)view.data(), view.size() }; }

		/**
		 * @brief Checks if the message was sent by the local node.
		 * @param network (optional) the network this message originated from (avoids a map lookup if provided)
		 * @return True if the message was sent by the local node, false otherwise.
		 */
		bool is_local(const Network& network) { return network.local_id() == sender(); }

		/**
		 * @brief Checks if the message was sent by the local node.
		 * @return True if the message was sent by the local node, false otherwise.
		 */
		bool is_local() { return is_local(lookup_network()); }
		
	};
}

#endif // SIMPLE_P2P_NETWORKING_HPP
