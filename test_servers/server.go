package main

import (
	"fmt"
	"log"
	"net/http"
	"sync"
)

func startServer(port int, id int) {
	mux := http.NewServeMux()

	// Endpoint 1: High-Speed RPS Testing
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		response := fmt.Sprintf("Hello from Go Backend %d (Port %d)!\n", id, port)
		
		w.Header().Set("Content-Type", "text/plain")
		w.Header().Set("Content-Length", fmt.Sprintf("%d", len(response)))
		w.Header().Set("Connection", "keep-alive")
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(response))
	})

	// Endpoint 2: The 10MB Zero-Copy Integrity Test
	mux.HandleFunc("/largefile", func(w http.ResponseWriter, r *http.Request) {
		size := 10 * 1024 * 1024 // Exactly 10 Megabytes
		
		w.Header().Set("Content-Type", "application/octet-stream")
		w.Header().Set("Content-Length", fmt.Sprintf("%d", size))
		w.Header().Set("Connection", "keep-alive")
		w.WriteHeader(http.StatusOK)

		// Write in 8KB chunks to simulate network buffering
		chunk := make([]byte, 8192)
		for i := range chunk {
			chunk[i] = 'A' 
		}
		
		bytesWritten := 0
		for bytesWritten < size {
			n, err := w.Write(chunk)
			if err != nil {
				break
			}
			bytesWritten += n
		}
	})

	serverAddr := fmt.Sprintf("127.0.0.1:%d", port)
	fmt.Printf("✅ Backend %d listening on http://%s\n", id, serverAddr)
	
	if err := http.ListenAndServe(serverAddr, mux); err != nil {
		log.Fatalf("Server %d failed: %v", id, err)
	}
}

func main() {
	var wg sync.WaitGroup
	
	// These match the ports your C++ Load Balancer expects
	ports := []int{8081, 8082, 8083}

	fmt.Println("🚀 Starting Test Infrastructure...")

	for i, port := range ports {
		wg.Add(1)
		go func(p int, id int) {
			defer wg.Done()
			startServer(p, id)
		}(port, i)
	}

	fmt.Println("-------------------------------------------------")
	fmt.Println("All backends are online. Press Ctrl+C to stop.")
	fmt.Println("-------------------------------------------------")
	
	wg.Wait()
}
