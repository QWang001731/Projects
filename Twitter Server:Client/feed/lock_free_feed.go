package feed

import (
	"sync/atomic"
	"unsafe"
)

// Feed represents a user's twitter feed
// You will add to this interface the implementations as you complete them.

func NewAtomicMarkableReference(reference unsafe.Pointer, mark uint32) *AtomicMarkableReference {
	return &AtomicMarkableReference{reference: reference, marked: mark}
}

func (ref *AtomicMarkableReference) Get() (unsafe.Pointer, uint32) {
	reference := atomic.LoadPointer(&ref.reference)
	marked := atomic.LoadUint32(&ref.marked)
	return reference, marked
}

func (ref *AtomicMarkableReference) Set(newReference unsafe.Pointer, newMark int32) {
	atomic.StorePointer(&ref.reference, newReference)
	atomic.StoreUint32(&ref.marked, uint32(newMark))
}

func (ref *AtomicMarkableReference) CompareAndSet(expectedReference unsafe.Pointer, newReference unsafe.Pointer, expectedMark uint32, newMark uint32) bool {
	return atomic.CompareAndSwapPointer(&ref.reference, expectedReference, newReference) && atomic.CompareAndSwapUint32(&ref.marked, expectedMark, newMark)
}

func (ref *AtomicMarkableReference) AttemptMark(newMark uint32) bool {
	for {
		curReference := atomic.LoadPointer(&ref.reference)
		curMark := atomic.LoadUint32(&ref.marked)
		if curMark != 0 {
			return false
		}
		if atomic.CompareAndSwapPointer(&ref.reference, curReference, curReference) &&
			atomic.CompareAndSwapUint32(&ref.marked, curMark, newMark) {
			return true
		}
	}
}

type LockFreeFeed interface {
	Add(body string, timestamp float64)
	Remove(timestamp float64) bool
	Contains(timestamp float64) bool
	GetAll() []Postbody
}

type AtomicMarkableReference struct {
	reference unsafe.Pointer
	marked    uint32
}

type LFFeed struct {
	head *AtomicMarkableReference
}

type LockFreePost struct {
	body      string
	timestamp float64
	next      *AtomicMarkableReference
}

func newLockFreePost(body string, timestamp float64, next *AtomicMarkableReference) *LockFreePost {
	return &LockFreePost{body: body, timestamp: timestamp, next: next}
}

func NewLFFeed() LockFreeFeed {
	return &LFFeed{head: NewAtomicMarkableReference(unsafe.Pointer(nil), 0)}

}

func (feed *LFFeed) Contains(timestamp float64) bool {
	curAmo := feed.head
	cur, mark := curAmo.Get()
	for cur != nil {
		node := (*LockFreePost)(cur)
		if node.timestamp == timestamp {
			if mark == 0 {
				return true
			}
			if mark == 1 {
				return false
			}
		}
		cur, mark = node.next.Get()
	}
	return false
}

func (feed *LFFeed) Add(body string, timestamp float64) {
	for {
		curAmo := feed.head
		preAmo := curAmo
		cur, mark := curAmo.Get()
		node := (*LockFreePost)(cur)
		if cur == nil {
			newPost := newLockFreePost(body, timestamp, NewAtomicMarkableReference(nil, 0))
			if feed.head.CompareAndSet(nil, unsafe.Pointer(newPost), 0, 0) {
				return
			}
			continue
		}
		if timestamp > node.timestamp {
			if mark == 1 {
				continue
			}
			newPost := newLockFreePost(body, timestamp, NewAtomicMarkableReference(cur, 0))
			if feed.head.CompareAndSet(cur, unsafe.Pointer(newPost), 0, 0) {
				return
			}
			continue
		}

		for cur != nil && timestamp < node.timestamp {
			preAmo = curAmo
			curAmo = (*LockFreePost)(cur).next
			cur, _ = curAmo.Get()
			node = (*LockFreePost)(cur)
		}

		if cur == nil {
			_, mark1 := curAmo.Get()
			if mark1 == 1 {
				continue
			}
			newPost := newLockFreePost(body, timestamp, NewAtomicMarkableReference(nil, 0))
			if curAmo.CompareAndSet(nil, unsafe.Pointer(newPost), 0, 0) {
				return
			}
			continue
		}
		_, mark1 := curAmo.Get()
		_, mark2 := preAmo.Get()
		if mark1 == 1 || mark2 == 1 {
			continue
		}
		newPost := newLockFreePost(body, timestamp, NewAtomicMarkableReference(cur, 0))
		if curAmo.CompareAndSet(cur, unsafe.Pointer(newPost), 0, 0) {
			return
		}
		continue
	}
}

func (feed *LFFeed) Remove(timestamp float64) bool {
	curAmo := feed.head
	cur, mark := curAmo.Get()
	for cur != nil {
		node := (*LockFreePost)(cur)
		if node.timestamp == timestamp {
			if curAmo.CompareAndSet(cur, cur, mark, 1) {
				nextPost, mark1 := node.next.Get()
				curAmo.CompareAndSet(cur, nextPost, 1, mark1)
				return true
			}
			continue
		}
		curAmo = node.next
		cur, mark = curAmo.Get()
	}
	return false
}

/*
	for {
		head := feed.head
		headPtr, _ := head.Get()
		node := (*LockFreePost)(headPtr)
		if node == nil {
			println("1")
			return false
		}
		curAMO := NewAtomicMarkableReference(headPtr, 0)
		curPtr := headPtr
		node = (*LockFreePost)(curPtr)
		for node != nil && node.timestamp != timestamp {
			curAMO = (*LockFreePost)(curPtr).next
			curPtr, _ := curAMO.Get()
			node = (*LockFreePost)(curPtr)
			println(curAMO)
		}
		if node == nil {
			println("2")
			return false
		}
		nextNodePtr, _ := node.next.Get()
		curPtr, mark := curAMO.Get()
		if curAMO.CompareAndSet(curPtr, curPtr, mark, 1) {
			if curAMO.CompareAndSet(curPtr, nextNodePtr, 1, 0) {
				println("3")
				return true
			} else {
				println("4")
				continue
			}
		}
	}
*/

func (feed *LFFeed) GetAll() []Postbody {
	var feeds []Postbody
	curAmo := feed.head
	for {
		cur, mark := curAmo.Get()
		if cur == nil {
			break
		}
		if mark == 0 {
			body := (*LockFreePost)(cur).body
			timestamp := (*LockFreePost)(cur).timestamp
			newPostbody := Postbody{Body: body, TimeStamp: timestamp}
			feeds = append(feeds, newPostbody)
		}
		curAmo = (*LockFreePost)(cur).next
	}
	return feeds
}
