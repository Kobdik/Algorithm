package main

import (
	"fmt"
	"math/rand"
	"sort"
	"time"
)

type ByteSlice []byte

func (p ByteSlice) Len() int { return len(p) }
func (p ByteSlice) Less(i, j int) bool { return p[i] < p[j] }
func (p ByteSlice) Swap(i, j int) { p[i], p[j] = p[j], p[i] }

func main() {
	fmt.Println("Hello, world.")
	rand.Seed(time.Now().UnixNano())
	var data [4096 * 1024]byte
	n, _ := rand.Read(data[:])
	fst := time.Now()
	sort.Sort(ByteSlice(data[:]))
	lst := time.Now()
	dur := lst.Sub(fst)
	fmt.Printf("Sorted %d random values in %.3f sec\n", n, dur.Seconds())
}