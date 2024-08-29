package server

import (
	"encoding/json"
	"fmt"
	"io"
	"proj2/feed"
	"sync"
	"sync/atomic"
	"unsafe"
)

type Config struct {
	Encoder *json.Encoder // Represents the buffer to encode Responses
	Decoder *json.Decoder // Represents the buffer to decode Requests
	Mode    string        // Represents whether the server should execute
	// sequentially or in parallel
	// If Mode == "s"  then run the sequential version
	// If Mode == "p"  then run the parallel version
	// These are the only values for Version
	ConsumersCount int // Represents the number of consumers to spawn
}

type Response struct {
	Success bool
	Id      float64
}

type Feed_response struct {
	Id   int
	Feed []feed.Postbody
}

// Run starts up the twitter server based on the configuration
// information provided and only returns when the server is fully
// shutdown.

type task struct {
	data map[string]interface{}
	next unsafe.Pointer
}

type task_queue struct {
	mu   *sync.Mutex
	cond *sync.Cond
	head unsafe.Pointer
	tail unsafe.Pointer
}

func newTaskQueue() *task_queue {
	q := &task_queue{}
	var mu sync.Mutex
	q.mu = &mu
	cond := sync.NewCond(q.mu)
	dummy := &task{}
	q.head = unsafe.Pointer(dummy)
	q.tail = unsafe.Pointer(dummy)
	q.cond = cond
	return q
}

func (q *task_queue) enqueue(data map[string]interface{}) {
	newTask := &task{data: data}
	var tail, next unsafe.Pointer
	for {
		tail = atomic.LoadPointer(&q.tail)
		next = (*task)(tail).next
		if atomic.CompareAndSwapPointer(&q.tail, tail, unsafe.Pointer(newTask)) && next == nil {
			(*task)(tail).next = unsafe.Pointer(newTask)
			break
		}
	}
	atomic.CompareAndSwapPointer(&q.head, nil, unsafe.Pointer(newTask))
}

func (q *task_queue) dequeue() (map[string]interface{}, bool) {
	var head, next unsafe.Pointer
	for {
		head = atomic.LoadPointer(&q.head)
		if head == nil {
			if atomic.LoadPointer(&q.head) == head && next == nil {
				return nil, false
			}
		}
		next = (*task)(head).next
		if atomic.CompareAndSwapPointer(&q.head, head, unsafe.Pointer(next)) {
			return (*task)(head).data, true
		}
	}
}

func Run(config Config) {
	//myFeed := feed.NewLFFeed()
	myFeed := feed.NewFeed()
	if config.Mode == "s" {
		sequential(config, myFeed)
	} else {
		var wg sync.WaitGroup
		numGroutines := config.ConsumersCount
		tasks := newTaskQueue()
		for i := 0; i < numGroutines; i++ {
			wg.Add(1)
			go consumer(config, &wg, tasks, myFeed)
		}
		producer(config, tasks, numGroutines)
		wg.Wait()
		return
	}
}

func producer(config Config, tasks *task_queue, numGoroutines int) {
	dec := config.Decoder
	for {
		var v map[string]interface{}
		dec.Decode(&v)
		if v["command"] == "DONE" {
			for i := 0; i < numGoroutines; i++ {
				poison := make(map[string]interface{})
				poison["command"] = "POISON"
				tasks.enqueue(poison)
				tasks.cond.Signal()
			}
			break
		}
		tasks.enqueue(v)
		tasks.cond.Signal()
	}
}

func consumer(config Config, wg *sync.WaitGroup, tasks *task_queue, myFeed feed.Feed) {
	enc := config.Encoder
	for {
		tasks.mu.Lock()
		v, ok := tasks.dequeue()
		if !ok {
			tasks.cond.Wait()
		}
		tasks.mu.Unlock()
		if v["command"] == "ADD" {
			body := fmt.Sprintf("%v", v["body"])
			timeStamp, _ := v["timestamp"].(float64)
			id, _ := v["id"].(float64)
			myFeed.Add(body, timeStamp)
			res := Response{Success: true, Id: id}
			enc.Encode(res)
		} else if v["command"] == "REMOVE" {
			timeStamp, _ := v["timestamp"].(float64)
			id := v["id"].(float64)
			result := myFeed.Remove(timeStamp)
			res := Response{Success: result, Id: id}
			enc.Encode(res)
		} else if v["command"] == "CONTAINS" {
			timeStamp, _ := v["timestamp"].(float64)
			id, _ := v["id"].(float64)
			result := myFeed.Contains(timeStamp)
			res := Response{Success: result, Id: id}
			enc.Encode(res)
		} else if v["command"] == "FEED" {
			res := myFeed.GetAll()
			id, _ := v["id"].(float64)
			id_int := int(id)
			res1 := Feed_response{Id: id_int, Feed: res}
			enc.Encode(res1)
		} else if v["command"] == "DONE" {
			continue
		} else if v["command"] == "POISON" {
			wg.Done()
			return
		} else {
			continue
		}
	}
}

func sequential(config Config, myFeed feed.Feed) {
	dec := config.Decoder
	enc := config.Encoder
	for {
		var v map[string]interface{}
		if err := dec.Decode(&v); err == io.EOF {
			continue
		}
		if v["command"] == "ADD" {
			body := fmt.Sprintf("%v", v["body"])
			timeStamp, _ := v["timestamp"].(float64)
			id, _ := v["id"].(float64)
			myFeed.Add(body, timeStamp)
			res := Response{Success: true, Id: id}
			enc.Encode(res)
		} else if v["command"] == "REMOVE" {
			timeStamp, _ := v["timestamp"].(float64)
			id := v["id"].(float64)
			result := myFeed.Remove(timeStamp)
			res := Response{Success: result, Id: id}
			enc.Encode(res)
		} else if v["command"] == "CONTAINS" {
			timeStamp, _ := v["timestamp"].(float64)
			id, _ := v["id"].(float64)
			result := myFeed.Contains(timeStamp)
			res := Response{Success: result, Id: id}
			enc.Encode(res)
		} else if v["command"] == "FEED" {
			res := myFeed.GetAll()
			id, _ := v["id"].(float64)
			id_int := int(id)
			res1 := Feed_response{Id: id_int, Feed: res}
			enc.Encode(res1)
		} else if v["command"] == "DONE" {
			return
		}
	}
}
