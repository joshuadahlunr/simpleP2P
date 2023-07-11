/*
*
* The MIT License (MIT)
*
* Copyright (c) 2014 Juan Batiz-Benet
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
* This program demonstrate a simple chat application using p2p communication.
*
*/

package main

/*
#include <stdlib.h>
typedef const char* (*str_callback)(char*);
extern const char* bridge_str_callback(char* s, str_callback f);
*/
import "C"
import "unsafe"

import (
	"bufio"
	"context"
	"crypto/rand"
	"fmt"
	"io"
	"log"
	mrand "math/rand"
	// "os"

	"github.com/libp2p/go-libp2p"
	"github.com/libp2p/go-libp2p/core/crypto"
	"github.com/libp2p/go-libp2p/core/host"
	"github.com/libp2p/go-libp2p/core/network"
	"github.com/libp2p/go-libp2p/core/peer"
	"github.com/libp2p/go-libp2p/core/peerstore"

	"github.com/multiformats/go-multiaddr"
)

func handleStream(s network.Stream) {
	log.Println("Got a new stream!")

	// Create a buffer stream for non-blocking read and write.
	rw := bufio.NewReadWriter(bufio.NewReader(s), bufio.NewWriter(s))

	go readData(rw)
	go writeData(rw)

	// stream 's' will stay open until you close it (or the other side closes it).
}

var printCallback C.str_callback 

//export setPrintCallback
func setPrintCallback(callback C.str_callback) {
	printCallback = callback
}

func readData(rw *bufio.ReadWriter) {
	for {
		str, _ := rw.ReadString('\n')

		if str == "" {
			return
		}
		if str != "\n" {
			cstr := C.CString(str)
			defer C.free(unsafe.Pointer(cstr))

			if C.bridge_str_callback(cstr, printCallback) == nil {
				return
			}
		}

	}
}

var inputCallback C.str_callback 

//export setInputCallback
func setInputCallback(callback C.str_callback) {
	inputCallback = callback
}

//export setCallback
func setCallback(callback C.str_callback) {
	setPrintCallback(callback)
	setInputCallback(callback)
}

func writeData(rw *bufio.ReadWriter) {
	for {
		cstr := C.bridge_str_callback(nil, inputCallback)
		if cstr == nil {
			return
		}

		rw.WriteString(fmt.Sprintf("%s\n", C.GoString(cstr)))
		rw.Flush()
	}
}

//export run
func run(sourcePort int, dest string, debug bool) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// If debug is enabled, use a constant random source to generate the peer ID. Only useful for debugging,
	// off by default. Otherwise, it uses rand.Reader.
	var r io.Reader
	if debug {
		// Use the port number as the randomness source.
		// This will always generate the same host ID on multiple executions, if the same port number is used.
		// Never do this in production code.
		r = mrand.New(mrand.NewSource(int64(sourcePort)))
	} else {
		r = rand.Reader
	}

	h, err := makeHost(sourcePort, r)
	if err != nil {
		log.Println(err)
		return
	}

	if dest == "" {
		startPeer(ctx, h, handleStream)
	} else {
		rw, err := startPeerAndConnect(ctx, h, dest)
		if err != nil {
			log.Println(err)
			return
		}

		// Create a thread to read and write data.
		go writeData(rw)
		go readData(rw)

	}

	// Wait forever
	select {}
}

func makeHost(port int, randomness io.Reader) (host.Host, error) {
	// Creates a new RSA key pair for this host.
	prvKey, _, err := crypto.GenerateKeyPairWithReader(crypto.RSA, 2048, randomness)
	if err != nil {
		log.Println(err)
		return nil, err
	}

	// 0.0.0.0 will listen on any interface device.
	sourceMultiAddr, _ := multiaddr.NewMultiaddr(fmt.Sprintf("/ip4/0.0.0.0/tcp/%d", port))

	// libp2p.New constructs a new libp2p Host.
	// Other options can be added here.
	return libp2p.New(
		libp2p.ListenAddrs(sourceMultiAddr),
		libp2p.Identity(prvKey),
	)
}

func startPeer(ctx context.Context, h host.Host, streamHandler network.StreamHandler) {
	// Set a function as stream handler.
	// This function is called when a peer connects, and starts a stream with this protocol.
	// Only applies on the receiving side.
	h.SetStreamHandler("/chat/1.0.0", streamHandler)

	// Let's get the actual TCP port from our listen multiaddr, in case we're using 0 (default; random available port).
	var port string
	for _, la := range h.Network().ListenAddresses() {
		if p, err := la.ValueForProtocol(multiaddr.P_TCP); err == nil {
			port = p
			break
		}
	}

	if port == "" {
		log.Println("was not able to find actual local port")
		return
	}

	log.Printf("Run './chat -d /ip4/127.0.0.1/tcp/%v/p2p/%s' on another console.\n", port, h.ID().Pretty())
	log.Println("You can replace 127.0.0.1 with public IP as well.")
	log.Println("Waiting for incoming connection")
	log.Println()
}


func startPeerAndConnect(ctx context.Context, h host.Host, destination string) (*bufio.ReadWriter, error) {
	log.Println("This node's multiaddresses:")
	for _, la := range h.Addrs() {
		log.Printf(" - %v\n", la)
	}
	log.Println()

	// Turn the destination into a multiaddr.
	maddr, err := multiaddr.NewMultiaddr(destination)
	if err != nil {
		log.Println(err)
		return nil, err
	}

	// Extract the peer ID from the multiaddr.
	info, err := peer.AddrInfoFromP2pAddr(maddr)
	if err != nil {
		log.Println(err)
		return nil, err
	}

	// Add the destination's peer multiaddress in the peerstore.
	// This will be used during connection and stream creation by libp2p.
	h.Peerstore().AddAddrs(info.ID, info.Addrs, peerstore.PermanentAddrTTL)

	// Start a stream with the destination.
	// Multiaddress of the destination peer is fetched from the peerstore using 'peerId'.
	s, err := h.NewStream(context.Background(), info.ID, "/chat/1.0.0")
	if err != nil {
		log.Println(err)
		return nil, err
	}
	log.Println("Established connection to destination")

	// Create a buffered stream so that read and writes are non-blocking.
	rw := bufio.NewReadWriter(bufio.NewReader(s), bufio.NewWriter(s))

	return rw, nil
}

func main() {}
