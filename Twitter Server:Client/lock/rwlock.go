// Package lock provides an implementation of a read-write lock
// that uses condition variables and mutexes.
package lock

import "sync"

type RWlock struct {
	mu         sync.Mutex
	cond       *sync.Cond
	readers    int
	maxReaders int
}

func NewRWlock(maxReaders int) *RWlock {
	rwLock := RWlock{
		maxReaders: maxReaders,
		readers:    0,
	}
	rwLock.cond = sync.NewCond(&rwLock.mu)
	return &rwLock
}

func (rw *RWlock) RLock() {
	rw.mu.Lock()
	for rw.readers >= rw.maxReaders {
		rw.cond.Wait()
	}
	rw.readers++
	rw.mu.Unlock()
}

func (rw *RWlock) RUnlock() {
	rw.mu.Lock()
	rw.readers--
	rw.cond.Broadcast()
	rw.mu.Unlock()
}

func (rw *RWlock) Lock() {
	rw.mu.Lock()
}

func (rw *RWlock) Unlock() {
	rw.mu.Unlock()
}
