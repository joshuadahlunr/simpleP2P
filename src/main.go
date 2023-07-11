package main

/*
#include <stdlib.h>
typedef const char* (*str_callback)(char*);
extern const char* bridge_str_callback(char* s, str_callback f);
*/
import "C"
import (
	"context"
	"errors"
	"fmt"
	"sync"
	"unsafe"

	"github.com/libp2p/go-libp2p"

	dht "github.com/libp2p/go-libp2p-kad-dht"

	pubsub "github.com/libp2p/go-libp2p-pubsub"
	"github.com/libp2p/go-libp2p/core/host"
	"github.com/libp2p/go-libp2p/core/peer"

	drouting "github.com/libp2p/go-libp2p/p2p/discovery/routing"

	dutil "github.com/libp2p/go-libp2p/p2p/discovery/util"
)

var printCallback C.str_callback

//export setPrintCallback
func setPrintCallback(callback C.str_callback) {
	printCallback = callback
}

// var inputCallback C.str_callback

// //export setInputCallback
// func setInputCallback(callback C.str_callback) {
// 	inputCallback = callback
// }

// //export setCallback
// func setCallback(callback C.str_callback) {
// 	setPrintCallback(callback)
// 	setInputCallback(callback)
// }

func cWrite(data string) error {
	cstr := C.CString(data)
	defer C.free(unsafe.Pointer(cstr))

	if C.bridge_str_callback(cstr, printCallback) == nil {
		return errors.New("Failed to write data")
	}
	return nil
}

// func cRead() (string, error) {
// 	cstr := C.bridge_str_callback(nil, inputCallback)
// 	if cstr == nil {
// 		return "", errors.New("Failed to read data")
// 	}

// 	return C.GoString(cstr), nil
// }

type Topic struct {
	name         string
	topic        *pubsub.Topic
	subscription *pubsub.Subscription
}

type State struct {
	ctx    context.Context
	cancel context.CancelFunc
	host   host.Host
	dht    *dht.IpfsDHT
	ps     *pubsub.PubSub
	topics map[int]Topic
}

var state State

//export initialize
func initialize(discoveryTopic string, useQUIC bool, port int) int {
	ctx, cancel := context.WithCancel(context.Background())
	state.ctx = ctx
	state.cancel = cancel

	if useQUIC {
		h, err := libp2p.New(libp2p.ListenAddrStrings(fmt.Sprintf("/ip4/0.0.0.0/udp/%d/quic-v1", port)))
		if err != nil {
			panic(err)
		}
		state.host = h
	} else {
		h, err := libp2p.New(libp2p.ListenAddrStrings(fmt.Sprintf("/ip4/0.0.0.0/tcp/%d", port)))
		if err != nil {
			panic(err)
		}
		state.host = h
	}

	go discoverPeers(state.ctx, state.host, discoveryTopic)

	ps, err := pubsub.NewGossipSub(state.ctx, state.host)
	if err != nil {
		panic(err)
	}
	state.ps = ps
	state.topics = make(map[int]Topic)

	return subscribeToTopic(discoveryTopic)
}

//export shutdown
func shutdown() {
	state.cancel()

	for id := range state.topics {
		leaveTopic(id)
	}
	state.dht.Close()
	state.host.Close()
}

//export subscribeToTopic
func subscribeToTopic(name string) int {
	id := len(state.topics)
	if _, ok := state.topics[id]; ok {
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

	// go broadcaster(state.ctx, state.topics[id].topic)
	go reciever(state.ctx, state.topics[id].subscription)
	return id
}

//export findTopic
func findTopic(name string) int {
	for id, topic := range state.topics {
		if topic.name == name {
			return id
		}
	}

	return -1
}

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

	return true
}

// func main() {
// 	flag.Parse()
// 	initialize()

// 	topic := subscribeToTopic(*topicNameFlag)
// 	if topic < 0 {
// 		panic("Failed to join topic! " + *topicNameFlag)
// 	}

// 	time.Sleep(30 * time.Second)

// 	leaveTopic(topic)
// 	fmt.Println("left topic!")

// 	shutdown()
// }

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
			if err := h.Connect(ctx, *peerinfo); err != nil {
				fmt.Println("Bootstrap warning:", err)
			}
		}()
	}
	wg.Wait()

	return kademliaDHT
}

func discoverPeers(ctx context.Context, h host.Host, advertisingTopic string) {
	kademliaDHT := initDHT(ctx, h)
	state.dht = kademliaDHT
	routingDiscovery := drouting.NewRoutingDiscovery(kademliaDHT)
	dutil.Advertise(ctx, routingDiscovery, advertisingTopic)

	// Look for others who have announced and attempt to connect to them
	anyConnected := false
	for !anyConnected {
		fmt.Println("Searching for peers...")
		peerChan, err := routingDiscovery.FindPeers(ctx, advertisingTopic)
		if err != nil {
			panic(err)
		}
		for peer := range peerChan {
			if peer.ID == h.ID() {
				continue // No self connection
			}
			err := h.Connect(ctx, peer)
			if err != nil {
				fmt.Println("Failed connecting to ", peer.ID.Pretty(), ", error:", err)
			} else {
				fmt.Println("Connected to:", peer.ID.Pretty())
				anyConnected = true
			}
		}
	}
	fmt.Println("Peer discovery complete")
}

//export broadcastMessage
func broadcastMessage(message string, topicID int) bool {
	if t, ok := state.topics[topicID]; !ok || t.topic == nil {
		return false
	}

	if err := state.topics[topicID].topic.Publish(state.ctx, []byte(message)); err != nil {
		fmt.Println("### Publish error:", err)
		return false
	}

	return true
}

// func broadcaster(ctx context.Context, topic *pubsub.Topic) {
// 	for {
// 		select {
// 		case <-ctx.Done():
// 			return
// 		default:
// 			s, err := cRead()
// 			if err != nil {
// 				panic(err)
// 			}
// 			if err := topic.Publish(ctx, []byte(s)); err != nil {
// 				fmt.Println("### Publish error:", err)
// 			}
// 		}
// 	}
// }

func reciever(ctx context.Context, sub *pubsub.Subscription) {
	for {
		m, err := sub.Next(ctx)
		if m == nil { // This happens when the context gets canceled!
			return
		}
		if err != nil {
			panic(err)
		}
		cWrite(string(m.Message.Data))
		// fmt.Println(m.ReceivedFrom, ": ", string(m.Message.Data))
	}
}

func main() {}
