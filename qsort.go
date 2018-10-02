package main

import (
	"fmt"
	"math/rand"
	"sort"
	"time"
)

type IntSlice []int

func (p IntSlice) Len() int           { return len(p) }
func (p IntSlice) Less(i, j int) bool { return p[i] < p[j] }
func (p IntSlice) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }

func main() {
	rand.Seed(time.Now().UnixNano())
	const n = 5000000
	var data [n]int
	for i := 0; i < 5000; i++ {
		data[i] = rand.Intn(256)
	}
	for i := 0; i < 50; i++ {
		fmt.Printf("%d ", data[i])
	}
	fmt.Printf("\nbefore sort\n")
	slice := IntSlice(data[:])
	fst := time.Now()
	//sort.Ints(data[:])
	sort.Sort(slice)
	lst := time.Now()
	dur := lst.Sub(fst)
	sorted := sort.IsSorted(slice)
	for i := 0; i < 50; i++ {
		fmt.Printf("%d ", data[i])
	}
	fmt.Printf("\nafter sort\n")
	fmt.Printf("%d random values %v sorted in %.3f sec\n", n, sorted, dur.Seconds())
}
