package main

/*
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
	char* from;
	char* data;
	char* seqno;
	char* topic;
	char* signature;
	char* key;
	char* id;
	char* recieved_from;
} Message;
typedef bool (*msg_callback)(Message*);
extern bool bridge_msg_callback(Message* m, msg_callback f);
typedef bool (*void_callback)();
extern bool bridge_void_callback(void_callback f);
typedef bool (*peer_callback)(char*);
extern bool bridge_peer_callback(char* p, peer_callback f);
typedef bool (*topic_callback)(int);
extern bool bridge_topic_callback(int t, topic_callback f);
*/
import "C"
import (
	"context"
	"crypto/rand"
	"fmt"
	"sync"
	"time"
	"unsafe"

	"github.com/libp2p/go-libp2p"
	"github.com/libp2p/go-libp2p/core/crypto"
	"github.com/libp2p/go-libp2p/core/peer"

	dht "github.com/libp2p/go-libp2p-kad-dht"

	pubsub "github.com/libp2p/go-libp2p-pubsub"
	"github.com/libp2p/go-libp2p/core/host"

	drouting "github.com/libp2p/go-libp2p/p2p/discovery/routing"

	dutil "github.com/libp2p/go-libp2p/p2p/discovery/util"
)

var messageCallback C.msg_callback

//export setMessageCallback
func setMessageCallback(callback C.msg_callback) {
	messageCallback = callback
}

var peerConnectedCallback C.peer_callback

//export setPeerConnectedCallback
func setPeerConnectedCallback(callback C.peer_callback) {
	peerConnectedCallback = callback
}

var peerDisconnectedCallback C.peer_callback

//export setPeerDisconnectedCallback
func setPeerDisconnectedCallback(callback C.peer_callback) {
	peerDisconnectedCallback = callback
}

var topicSubscribedCallback C.topic_callback

//export setTopicSubscribedCallback
func setTopicSubscribedCallback(callback C.topic_callback) {
	topicSubscribedCallback = callback
}

var topicUnsubscribedCallback C.topic_callback

//export setTopicUnsubscribedCallback
func setTopicUnsubscribedCallback(callback C.topic_callback) {
	topicUnsubscribedCallback = callback
}

var connectedCallback C.void_callback

//export setConnectedCallback
func setConnectedCallback(callback C.void_callback) {
	connectedCallback = callback
}

var disconnectedCallback C.void_callback

//export setDisconnectedCallback
func setDisconnectedCallback(callback C.void_callback) {
	disconnectedCallback = callback
}

/*



 */

// Topic represents a pubsub topic
type Topic struct {
	name         string
	topic        *pubsub.Topic
	subscription *pubsub.Subscription
}

// State represents the state of the application
type State struct {
	verbose           bool
	connectionTimeout float64
	ctx               context.Context
	cancel            context.CancelFunc
	host              host.Host
	dht               *dht.IpfsDHT
	ps                *pubsub.PubSub
	topics            map[int]Topic // Maps a topicID to the above topic struct
}

var state State

// generateCKey exports a cryptographic key to C
//
//export generateCKey
func generateCKey() (*C.char, C.int) {
	str := string(generateKey())
	return C.CString(str), C.int(len(str))
}

// generateKey generates a new cryptographic key
func generateKey() []byte {
	privKey, _, err := crypto.GenerateECDSAKeyPair(rand.Reader)
	if err != nil {
		panic(err)
	}

	keyBytes, err := crypto.MarshalPrivateKey(privKey)
	if err != nil {
		panic(err)
	}

	return keyBytes
}

// initialize starts up a connection to the p2p network and initializes some library state
//
//export initialize
func initialize(listenAddress string, discoveryTopic string, keyString string, connectionTimeout float64, verbose bool) int {
	state.verbose = verbose
	state.connectionTimeout = connectionTimeout

	ctx, cancel := context.WithCancel(context.Background())
	state.ctx = ctx
	state.cancel = cancel

	key := []byte(keyString)
	if len(key) <= 0 {
		key = generateKey()
	}

	privateKey, err := crypto.UnmarshalPrivateKey(key)
	if err != nil {
		fmt.Println("Failed to unpack identity key!")
		panic(err)
	}

	h, err := libp2p.New(
		libp2p.ListenAddrStrings(listenAddress),
		libp2p.Identity(privateKey))
	if err != nil {
		panic(err)
	}
	state.host = h

	go discoverPeers(state.ctx, state.host, discoveryTopic)

	ps, err := pubsub.NewGossipSub(state.ctx, state.host)
	if err != nil {
		panic(err)
	}
	state.ps = ps
	state.topics = make(map[int]Topic)

	go trackPeers(state.ctx)

	return subscribeToTopic(discoveryTopic)
}

// shutdown shuts down the library
//
//export shutdown
func shutdown() {
	state.cancel()

	for id := range state.topics {
		leaveTopic(id)
	}
	state.dht.Close()
	state.host.Close()
	if !C.bridge_void_callback(disconnectedCallback) {
		panic("C error!")
	}
}

// localID returns the hashes ID of the current node
//
//export localID
func localID() *C.char {
	return C.CString(state.host.ID().Pretty())
}

// subscribeToTopic subscribes to a topic and begins listening to messages sent within it
//
//export subscribeToTopic
func subscribeToTopic(name string) int {
	id := len(state.topics)
	if _, ok := state.topics[id]; ok {
		if state.verbose {
			fmt.Println("Failed to find topic: " + name)
		}
		return -1
	}

	topic, err := state.ps.Join(name)
	if err != nil {
		panic(err)
	}

	sub, err := topic.Subscribe()
	if err != nil {
		panic(err)
	}

	state.topics[id] = Topic{name: name, topic: topic, subscription: sub}

	go reciever(state.ctx, state.topics[id].subscription)
	if !C.bridge_topic_callback(C.int(id), topicSubscribedCallback) {
		panic("C error!")
	}
	return id
}

// findTopic finds a topicID by name
//
//export findTopic
func findTopic(name string) int {
	for id, topic := range state.topics {
		if topic.name == name {
			return id
		}
	}

	return -1
}

// topicString returns the name of a topic given its TopicID
//
//export topicString
func topicString(topicID int) *C.char {
	if t, ok := state.topics[topicID]; ok {
		return C.CString(t.name)
	}
	return nil
}

// leaveTopic leaves a topic and stops listening to its messages
//
//export leaveTopic
func leaveTopic(id int) bool {
	if _, ok := state.topics[id]; !ok {
		return false
	}

	if state.topics[id].subscription != nil {
		state.topics[id].subscription.Cancel()
	}
	if state.topics[id].topic != nil {
		state.topics[id].topic.Close()
	}
	state.topics[id] = Topic{name: "invalid", topic: nil, subscription: nil} // Leave topic in list (technically a memory leak!) so that we don't have id conflicts!
	// delete(state.topics, id)

	if !C.bridge_topic_callback(C.int(id), topicUnsubscribedCallback) {
		panic("C error!")
	}
	return true
}

// broadcastMessage broadcasts a message to all other peers listening to a topic
//
//export broadcastMessage
func broadcastMessage(message string, topicID int) bool {
	if t, ok := state.topics[topicID]; !ok || t.topic == nil {
		return false
	}

	if err := state.topics[topicID].topic.Publish(state.ctx, []byte(message)); err != nil && state.verbose {
		fmt.Println("### Publish error:", err)
		return false
	}

	return true
}

// initDHT initializes the DHT used to find peers
func initDHT(ctx context.Context, h host.Host) *dht.IpfsDHT {
	// Start a DHT, for use in peer discovery. We can't just make a new DHT
	// client because we want each peer to maintain its own local copy of the
	// DHT, so that the bootstrapping node of the DHT can go down without
	// inhibiting future peer discovery.
	kademliaDHT, err := dht.New(ctx, h)
	if err != nil {
		panic(err)
	}
	if err = kademliaDHT.Bootstrap(ctx); err != nil {
		panic(err)
	}
	var wg sync.WaitGroup
	for _, peerAddr := range dht.DefaultBootstrapPeers {
		peerinfo, _ := peer.AddrInfoFromP2pAddr(peerAddr)
		wg.Add(1)
		go func() {
			defer wg.Done()
			if err := h.Connect(ctx, *peerinfo); err != nil && state.verbose {
				fmt.Println("Bootstrap warning:", err)
			}
		}()
	}
	wg.Wait()

	return kademliaDHT
}

// discoverPeers discovers peers and establishes connections to them
func discoverPeers(inCTX context.Context, h host.Host, advertisingTopic string) {
	ctx, cancel := context.WithTimeout(inCTX, time.Duration(state.connectionTimeout*float64(time.Second)))
	defer cancel()
	kademliaDHT := initDHT(ctx, h)
	state.dht = kademliaDHT
	routingDiscovery := drouting.NewRoutingDiscovery(kademliaDHT)
	dutil.Advertise(ctx, routingDiscovery, advertisingTopic)

	// Look for others who have announced and attempt to connect to them
	anyConnected := false
	for !anyConnected {
		select {
		case <-ctx.Done():
			panic("Failed to find peers!")
		default:
			fmt.Println("Searching for peers...")
			peerChan, err := routingDiscovery.FindPeers(ctx, advertisingTopic)
			if err != nil {
				panic(err)
			}
			var wg sync.WaitGroup
			for peer := range peerChan {
				wg.Add(1)
				go func() {
					defer wg.Done()
					if peer.ID == h.ID() {
						return // No self connection
					}
					err := h.Connect(ctx, peer)
					if err != nil {
						if state.verbose {
							fmt.Println("Failed connecting to ", peer.ID.Pretty(), ", error:", err)
						}
					} else {
						if state.verbose {
							fmt.Println("Connected to:", peer.ID.Pretty())
						}
						anyConnected = true
					}
				}()
			}
			wg.Wait()
		}
	}

	fmt.Println("Peer discovery complete!")
	C.bridge_void_callback(connectedCallback)
}

// trackPeers tracks connected and disconnected peers and fires events when peers connect or disconnect
func trackPeers(ctx context.Context) {
	connectedPeers := make(map[peer.ID]struct{})

	for {
		select {
		case <-ctx.Done():
			return
		case <-time.After(time.Second):
			peers := make([]peer.ID, 0)
			newPeers := make([]peer.ID, 0)
			disconnectedPeers := make([]peer.ID, 0)

			for _, topic := range state.ps.GetTopics() {
				peers = append(peers, state.ps.ListPeers(topic)...)
			}

			// Check for new peers
			for _, p := range peers {
				if _, ok := connectedPeers[p]; !ok {
					connectedPeers[p] = struct{}{}
					newPeers = append(newPeers, p)
				}
			}

			// Check for disconnected peers
			for p := range connectedPeers {
				if !containsPeer(peers, p) {
					delete(connectedPeers, p)
					disconnectedPeers = append(disconnectedPeers, p)
				}
			}

			for _, peerID := range newPeers {
				c := C.CString(peerID.Pretty())
				defer C.free(unsafe.Pointer(c))
				if !C.bridge_peer_callback(c, peerConnectedCallback) {
					panic("C error!")
				}
			}

			for _, peerID := range disconnectedPeers {
				c := C.CString(peerID.Pretty())
				defer C.free(unsafe.Pointer(c))
				if !C.bridge_peer_callback(c, peerDisconnectedCallback) {
					panic("C error!")
				}
			}
		}
	}
}

// containsPeer checks if a peer exists in a list of peers
func containsPeer(peers []peer.ID, target peer.ID) bool {
	for _, p := range peers {
		if p == target {
			return true
		}
	}
	return false
}

// reciever receives messages from a subscription
func reciever(ctx context.Context, sub *pubsub.Subscription) {
	for {
		m, err := sub.Next(ctx)
		if m == nil { // This happens when the context gets canceled!
			return
		}
		if err != nil {
			panic(err)
		}

		cfrom := C.CString(string(m.Message.From))
		defer C.free(unsafe.Pointer(cfrom))
		cdata := C.CString(string(m.Message.Data))
		defer C.free(unsafe.Pointer(cdata))
		cseqno := C.CString(string(m.Message.Seqno))
		defer C.free(unsafe.Pointer(cseqno))
		ctopic := C.CString(*m.Message.Topic)
		defer C.free(unsafe.Pointer(ctopic))
		csignature := C.CString(string(m.Message.Signature))
		defer C.free(unsafe.Pointer(csignature))
		ckey := C.CString(string(m.Message.Key))
		defer C.free(unsafe.Pointer(ckey))
		cID := C.CString(string(m.ID))
		defer C.free(unsafe.Pointer(cID))
		creceivedFrom := C.CString(m.ReceivedFrom.Pretty())
		defer C.free(unsafe.Pointer(creceivedFrom))

		msg := C.Message{from: cfrom, data: cdata, seqno: cseqno, topic: ctopic, signature: csignature, key: ckey, id: cID, recieved_from: creceivedFrom}
		if !C.bridge_msg_callback(&msg, messageCallback) {
			panic("Failed to pass message to C!")
		}
	}
}

// Dummy needed for CGO to properly work!
func main() {}
