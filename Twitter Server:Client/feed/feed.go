package feed

import (
	"proj2/lock"
)

// Feed represents a user's twitter feed
// You will add to this interface the implementations as you complete them.
type Feed interface {
	Add(body string, timestamp float64)
	Remove(timestamp float64) bool
	Contains(timestamp float64) bool
	GetAll() []Postbody
}

// feed is the internal representation of a user's twitter feed (hidden from outside packages)
// You CAN add to this structure but you cannot remove any of the original fields. You must use
// the original fields in your implementation. You can assume the feed will not have duplicate posts
type feed struct {
	start *post // a pointer to the beginning post
	rw    *lock.RWlock
}

// post is the internal representation of a post on a user's twitter feed (hidden from outside packages)
// You CAN add to this structure but you cannot remove any of the original fields. You must use
// the original fields in your implementation.
type post struct {
	body      string  // the text of the post
	timestamp float64 // Unix timestamp of the post
	next      *post   // the next post in the feed

}

type Postbody struct {
	Body      string
	TimeStamp float64
}

// newPost creates and returns a new post value given its body and timestamp
func newPost(body string, timestamp float64, next *post) *post {
	return &post{body, timestamp, next}
}

// NewFeed creates a empy user feed
func NewFeed() Feed {
	RW := lock.NewRWlock(64)
	return &feed{start: nil, rw: RW}
}

// Add inserts a new post to the feed. The feed is always ordered by the timestamp where
// the most recent timestamp is at the beginning of the feed followed by the second most
// recent timestamp, etc. You may need to insert a new post somewhere in the feed because
// the given timestamp may not be the most recent.

func (f *feed) Add(body string, timestamp float64) {
	f.rw.Lock()
	if f.start == nil {
		f.start = newPost(body, timestamp, nil)
		f.rw.Unlock()
		return
	}
	if timestamp > f.start.timestamp {
		new_start := newPost(body, timestamp, f.start)
		f.start = new_start
		f.rw.Unlock()
		return
	}
	for cur := f.start; cur != nil; {
		next_post := cur.next
		if next_post == nil {
			cur.next = newPost(body, timestamp, nil)
			break
		} else if cur.timestamp >= timestamp && timestamp >= next_post.timestamp {
			next := cur.next
			newPost := newPost(body, timestamp, next)
			cur.next = newPost
			break
		}
		cur = cur.next
	}
	f.rw.Unlock()
}

// Remove deletes the post with the given timestamp. If the timestamp
// is not included in a post of the feed then the feed remains
// unchanged. Return true if the deletion was a success, otherwise return false
func (f *feed) Remove(timestamp float64) bool {
	f.rw.Lock()
	var pre *post
	for cur := f.start; cur != nil; {
		if cur.timestamp == timestamp {
			if cur == f.start {
				f.start = f.start.next
				f.rw.Unlock()
				return true
			} else {
				pre.next = cur.next
				f.rw.Unlock()
				return true
			}
		}
		pre = cur
		cur = cur.next
	}
	f.rw.Unlock()
	return false
}

// Contains determines whether a post with the given timestamp is
// inside a feed. The function returns true if there is a post
// with the timestamp, otherwise, false.
func (f *feed) Contains(timestamp float64) bool {
	f.rw.RLock()
	for cur := f.start; cur != nil; {
		if cur.timestamp == timestamp {
			f.rw.RUnlock()
			return true
		}
		cur = cur.next
	}
	f.rw.RUnlock()
	return false
}

func (f *feed) GetAll() []Postbody {
	var feeds []Postbody
	f.rw.RLock()
	for cur := f.start; cur != nil; cur = cur.next {
		newPostbody := Postbody{Body: cur.body, TimeStamp: cur.timestamp}
		feeds = append(feeds, newPostbody)
	}
	f.rw.RUnlock()
	return feeds
}
