# Simple P2P

Simple P2P is a minimalistic C/++ wrapper around the go implementation of libp2p. It wraps a gossip-sub rendevus based network and exposes a simple three function interface to connect to the network, and then send and recieve messages.

```c
bool print(P2PMessage* message) {
    printf("Sender %s: %s", message->received_from, message->data);
    return true; // Did everything go well? False indicates we need to quit!
}

...

// All of the following values are set automatically by calling p2p_default_initialize_args(), then only any deviations from these values need to be specified
P2PInitializationArguments initInfo;
// A multiaddress storing both network and transport layer information (this is the address used by default)
initInfo.listenAddress = "/ip4/0.0.0.0/udp/0/quic-v1";
initInfo.listenAddressSize = strlen(initInfo.listenAddress);
// This is the name of the network... peers who are also discovering on this topic will be automatically found
initInfo.discoveryTopic = "simpleP2P";
initInfo.discoveryTopicSize = strlen(initInfo.discoveryTopic);
// Key used to define our identity, can be generated using p2p_generate_key(). If a null key is passed to init one will be generated automatically!
initInfo.identity = p2p_null_key();
// Time (in seconds) to wait before giving up and deciding that we can't find any peers;
initInfo.connectionTimeout = 60;
// Weather or not the library should print extra debugging information.
initInfo.verbose = false; 

// This function will block until a connection is established (or the timeout elapses) the networking will occur on a seperate thread which can be stopped by calling p2p_shutdown()
P2PNetwork network = p2p_initialize(initInfo);
// if (network < 0) an error has occured! (this function will block)

// Specifies that print should be called whenever we recieve a message from the network
p2p_set_message_callback(network, print);

...

// Sends a message to every connected peer (including ourself)
p2p_broadcast_message(network, "Hello World!", defaultTopic);
// or p2p_broadcast_messagen(data, size, topic); to avoid a call to strlen
```

Additional functions are provided to handle switching topics once we are connected to a network. Topics can be thought of as different channels/rooms in a chat application. 
Additional callbacks are provided for other events such as network connection and topic subscription. Check the [documentation](https://github.com/joshuadahlunr/simpleP2P/wiki/C-API) for a full list.

**Note that simpleP2P only provides utilities to broadcast messages to an entire topic.** If you want to ensure private communications with one specific peer, this will need to be handled seperately by your application using some form of encryption.

## C++ Wrapper

A C++ wrapper is also provided! The above C example can be reproduced as follows:

```cpp
void print(p2p::Network& network, p2p::Message& message) {
    std::cout << "Sender " << message.sender() << ": " << message.data_string() << std::endl;
}

...

// p2p::Network is a singleton... there can only be one of it and it can't be copied or moved!
// All of the arguments to the C initialization struct are presented and given default values
p2p::Network net(p2p::default_listen_address, "simpleP2P");
net.on_message.subscribe(print); // The += operator can be used as well

...

// NOTE: The network will automatically track the default topic
net.broadcast_message("Hello World!");
```

See the [chat.cpp](https://github.com/joshuadahlunr/simpleP2P/blob/master/examples/chat.cpp) example and [documentation](https://github.com/joshuadahlunr/simpleP2P/wiki/Cpp-API) for more details!

## Building

SimpleP2P has no other C++ library dependencies, it is also setup to automaticlly fetch and build the go library dependencies using cmake. Thus all it needs is
-> CMake >= 3.12
-> C/++ Compiler (supporting C++ 20 [specifically std::span] if the C++ wrapper is used)
-> GO Compiler >= 1.19

SimpleP2P supports being added as subdirectory and will provide the `simplep2p` target your project can link against (includes both the C and C++ APIs).

You can also build the sample chat application by running
```bash
mkdir build
cd build
cmake .. -DBUILD_EXAMPLES=ON #-G Ninja
make # or ninja if your prefer
./chat --help # For a list of its commands
```

**Note that in some cases the go module step will find versioning issues, CMake will identify this as failure and stop the build!** If this happens simply rerun make and the build should finish as normal!

Windows: ![Windows Build Status](https://github.com/joshuadahlunr/simpleP2P/actions/workflows/build-windows.yml/badge.svg?event=push) Linux: ![Linux Build Status](https://github.com/joshuadahlunr/simpleP2P/actions/workflows/build-linux.yml/badge.svg?event=push)
