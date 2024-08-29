package main

import (
	"encoding/json"
	"fmt"
	"os"
	"proj2/server"
	"strconv"
	"time"
)

func main() {
	var num_goroutines int
	var mode string
	if len(os.Args) < 2 {
		num_goroutines = 1
		mode = "s"
		//fmt.Println("s")
	} else {
		num_goroutines, _ = strconv.Atoi(os.Args[1])
		//fmt.Println(num_goroutines)
		mode = "p"
	}
	dec := json.NewDecoder(os.Stdin)
	enc := json.NewEncoder(os.Stdout)
	config := server.Config{Encoder: enc, Decoder: dec, Mode: mode, ConsumersCount: num_goroutines}
	start := time.Now()
	server.Run(config)
	elapsed := time.Since(start)
	fmt.Println("Execution time:", elapsed)
}
