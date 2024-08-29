package ppsync

import (
	"sync/atomic"
)

type PPLock struct {
	state uint32
}

func NewPPLock() *PPLock {
	return &PPLock{}
}

func (lock *PPLock) Lock() {
	for !atomic.CompareAndSwapUint32(&lock.state, 0, 1) {
	}
}

func (lock *PPLock) Unlock() {
	atomic.StoreUint32(&lock.state, 0)
}
