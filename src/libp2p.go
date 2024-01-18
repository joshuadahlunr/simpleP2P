package main

/*
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
	int network;
	char* from;
	char* data;
	char* seqno;
	char* topic;
	char* signature;
	char* key;
	char* id;
	char* recieved_from;
} Message;
typedef bool (*msg_callback)(int, Message*);
extern bool bridge_msg_callback(int n, Message* m, msg_callback f);
typedef bool (*void_callback)(int);
extern bool bridge_void_callback(int n, void_callback f);
typedef bool (*peer_callback)(int, char*);
extern bool bridge_peer_callback(int n, char* p, peer_callback f);
typedef bool (*topic_callback)(int, int);
extern bool bridge_topic_callback(int n, int t, topic_callback f);
*/
import "C"
import (
	"context"
	"crypto/rand"
	b64 "encoding/base64"
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

var messageCallbacks = make(map[int]C.msg_callback)

//export setMessageCallback
func setMessageCallback(nid int, callback C.msg_callback) {
	messageCallbacks[nid] = callback
}

var peerconnectedCallbacks = make(map[int]C.peer_callback)

//export setPeerConnectedCallback
func setPeerConnectedCallback(nid int, callback C.peer_callback) {
	peerconnectedCallbacks[nid] = callback
}

var peerDisconnectedCallbacks = make(map[int]C.peer_callback)

//export setPeerDisconnectedCallback
func setPeerDisconnectedCallback(nid int, callback C.peer_callback) {
	peerDisconnectedCallbacks[nid] = callback
}

var topicSubscribedCallbacks = make(map[int]C.topic_callback)

//export setTopicSubscribedCallback
func setTopicSubscribedCallback(nid int, callback C.topic_callback) {
	topicSubscribedCallbacks[nid] = callback
}

var topicUnsubscribedCallbacks = make(map[int]C.topic_callback)

//export setTopicUnsubscribedCallback
func setTopicUnsubscribedCallback(nid int, callback C.topic_callback) {
	topicUnsubscribedCallbacks[nid] = callback
}

var connectedCallbacks = make(map[int]C.void_callback)

//export setConnectedCallback
func setConnectedCallback(nid int, callback C.void_callback) {
	connectedCallbacks[nid] = callback
}

var disconnectedCallbacks = make(map[int]C.void_callback)

//export setDisconnectedCallback
func setDisconnectedCallback(nid int, callback C.void_callback) {
	disconnectedCallbacks[nid] = callback
}

/*



 */

// Topic represents a pubsub topic
type Topic struct {
	name         string
	topic        *pubsub.Topic
	subscription *pubsub.Subscription
}

// State represents the State of a network connection
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

var states = make(map[int]State)

//export networksCount
func networksCount() C.int {
	return C.int(len(states))
}

//export networkValid
func networkValid(nid int) bool {
	_, ok := states[nid]
	return ok
}

//export base64Encode
func base64Encode(str *C.char, size C.int) (*C.char, C.int) {
	encoded := b64.URLEncoding.EncodeToString(C.GoBytes(unsafe.Pointer(str), size))
	return C.CString(encoded), C.int(len(encoded))
}

//export base64Decode
func base64Decode(encoded string) (*C.char, C.int) {
	decoded, ok := b64.URLEncoding.DecodeString(encoded)
	if ok != nil {
		return nil, -1
	}
	return (*C.char)(C.CBytes(decoded)), C.int(len(decoded))
}

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

// initialize starts up a connection to the p2p network and initializes some library states
//
//export initialize
func initialize(listenAddress string, discoveryTopic string, keyString string, connectionTimeout float64, fullyConnected bool, verbose bool) int {
	var nid = len(states)
	var localState State
	localState.verbose = verbose
	localState.connectionTimeout = connectionTimeout

	ctx, cancel := context.WithCancel(context.Background())
	localState.ctx = ctx
	localState.cancel = cancel

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
	localState.host = h
	states[nid] = localState

	go discoverPeers(nid, states[nid].ctx, states[nid].host, discoveryTopic)

	if fullyConnected {
		ps, err := pubsub.NewFloodSub(localState.ctx, localState.host)
		if err != nil {
			panic(err)
		}
		localState.ps = ps
	} else {
		ps, err := pubsub.NewGossipSub(localState.ctx, localState.host)
		if err != nil {
			panic(err)
		}
		localState.ps = ps
	}

	localState.topics = make(map[int]Topic)

	states[nid] = localState

	go trackPeers(nid, states[nid].ctx)

	// Make sure we can connect to the discovery topic!
	if topic := subscribeToTopic(nid, discoveryTopic); topic < 0 {
		return topic
	}

	return nid
}

// shutdown shuts down the library
//
//export shutdown
func shutdown(nid int) {
	states[nid].cancel()

	for id := range states[nid].topics {
		leaveTopic(nid, id)
	}
	states[nid].dht.Close()
	states[nid].host.Close()
	if !C.bridge_void_callback(C.int(nid), disconnectedCallbacks[nid]) {
		panic("C error!")
	}

	delete(messageCallbacks, nid)
	delete(peerconnectedCallbacks, nid)
	delete(peerDisconnectedCallbacks, nid)
	delete(topicSubscribedCallbacks, nid)
	delete(topicUnsubscribedCallbacks, nid)
	delete(connectedCallbacks, nid)
	delete(disconnectedCallbacks, nid)
	delete(states, nid)
}

// localID returns the hashed ID of the current node
//
//export localID
func localID(nid int) *C.char {
	return C.CString(string(states[nid].host.ID()))
}

// subscribeToTopic subscribes to a topic and begins listening to messages sent within it
//
//export subscribeToTopic
func subscribeToTopic(nid int, name string) int {
	id := len(states[nid].topics)
	if _, ok := states[nid].topics[id]; ok {
		if states[nid].verbose {
			fmt.Println("Failed to find topic: " + name)
		}
		return -1
	}

	topic, err := states[nid].ps.Join(name)
	if err != nil {
		panic(err)
	}

	sub, err := topic.Subscribe()
	if err != nil {
		panic(err)
	}

	states[nid].topics[id] = Topic{name: name, topic: topic, subscription: sub}

	go reciever(nid, states[nid].ctx, states[nid].topics[id].subscription)
	if !C.bridge_topic_callback(C.int(nid), C.int(id), topicSubscribedCallbacks[nid]) {
		panic("C error!")
	}
	return id
}

// findTopic finds a topicID by name
//
//export findTopic
func findTopic(nid int, name string) int {
	for id, topic := range states[nid].topics {
		if topic.name == name {
			return id
		}
	}

	return -1
}

// topicString returns the name of a topic given its TopicID
//
//export topicString
func topicString(nid int, topicID int) *C.char {
	if t, ok := states[nid].topics[topicID]; ok {
		return C.CString(t.name)
	}
	return nil
}

// leaveTopic leaves a topic and stops listening to its messages
//
//export leaveTopic
func leaveTopic(nid int, id int) bool {
	if _, ok := states[nid].topics[id]; !ok {
		return false
	}
	if states[nid].topics[id].subscription != nil {
		states[nid].topics[id].subscription.Cancel()
	}
	if states[nid].topics[id].topic != nil {
		states[nid].topics[id].topic.Close()
	}
	states[nid].topics[id] = Topic{name: "invalid", topic: nil, subscription: nil} // Leave topic in list (technically a memory leak!) so that we don't have id conflicts!

	if !C.bridge_topic_callback(C.int(nid), C.int(id), topicUnsubscribedCallbacks[nid]) {
		panic("C error!")
	}
	return true
}

// broadcastMessage broadcasts a message to all other peers listening to a topic
//
//export broadcastMessage
func broadcastMessage(nid int, message string, topicID int) bool {
	if t, ok := states[nid].topics[topicID]; !ok || t.topic == nil {
		return false
	}

	if err := states[nid].topics[topicID].topic.Publish(states[nid].ctx, []byte(message)); err != nil && states[nid].verbose {
		fmt.Println("### Publish error:", err)
		return false
	}

	return true
}

// initDHT initializes the DHT used to find peers
func initDHT(nid int, ctx context.Context, h host.Host) *dht.IpfsDHT {
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
			if err := h.Connect(ctx, *peerinfo); err != nil && states[nid].verbose {
				fmt.Println("Bootstrap warning:", err)
			}
		}()
	}
	wg.Wait()

	return kademliaDHT
}

// discoverPeers discovers peers and establishes connections to them
func discoverPeers(nid int, inCTX context.Context, h host.Host, advertisingTopic string) {
	ctx, cancel := context.WithTimeout(inCTX, time.Duration(states[nid].connectionTimeout*float64(time.Second)))
	defer cancel()
	kademliaDHT := initDHT(nid, ctx, h)
	if localState, ok := states[nid]; ok {
		localState.dht = kademliaDHT
		states[nid] = localState
	}
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
						if states[nid].verbose {
							fmt.Println("Failed connecting to ", string(peer.ID), ", error:", err)
						}
					} else {
						if states[nid].verbose {
							fmt.Println("Connected to:", string(peer.ID))
						}
						anyConnected = true
					}
				}()
			}
			wg.Wait()
		}
	}

	fmt.Println("Peer discovery complete!")
	C.bridge_void_callback(C.int(nid), connectedCallbacks[nid])
}

// trackPeers tracks connected and disconnected peers and fires events when peers connect or disconnect
func trackPeers(nid int, ctx context.Context) {
	connectedPeers := make(map[peer.ID]struct{})

	for {
		select {
		case <-ctx.Done():
			return
		case <-time.After(time.Second):
			peers := make([]peer.ID, 0)
			newPeers := make([]peer.ID, 0)
			disconnectedPeers := make([]peer.ID, 0)

			for _, topic := range states[nid].ps.GetTopics() {
				peers = append(peers, states[nid].ps.ListPeers(topic)...)
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
				c := C.CString(string(peerID))
				defer C.free(unsafe.Pointer(c))
				if !C.bridge_peer_callback(C.int(nid), c, peerconnectedCallbacks[nid]) {
					panic("C error!")
				}
			}

			for _, peerID := range disconnectedPeers {
				c := C.CString(string(peerID))
				defer C.free(unsafe.Pointer(c))
				if !C.bridge_peer_callback(C.int(nid), c, peerDisconnectedCallbacks[nid]) {
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
func reciever(nid int, ctx context.Context, sub *pubsub.Subscription) {
	for {
		m, err := sub.Next(ctx)
		if m == nil { // This happens when the context gets canceled!
			return
		}
		if err != nil {
			panic(err)
		}

		cnid := C.int(nid)
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
		creceivedFrom := C.CString(string(m.ReceivedFrom))
		defer C.free(unsafe.Pointer(creceivedFrom))

		msg := C.Message{network: cnid, from: cfrom, data: cdata, seqno: cseqno, topic: ctopic, signature: csignature, key: ckey, id: cID, recieved_from: creceivedFrom}
		if !C.bridge_msg_callback(cnid, &msg, messageCallbacks[nid]) {
			panic("Failed to pass message to C!")
		}
	}
}

// Dummy needed for CGO to properly work!
func main() {}
