package main

import (
	"fmt"
	"math/rand"
	"sort"
	"sync"
	"time"
)

type Work struct{ fstIndex, lstIndex, eqlCount int }

type Int16Slice []int16

func (data Int16Slice) Len() int           { return len(data) }
func (data Int16Slice) Less(i, j int) bool { return data[i] < data[j] }
func (data Int16Slice) Swap(i, j int)      { data[i], data[j] = data[j], data[i] }
func (data Int16Slice) Part(work *Work) int {
	var value int16
	var eqlCount, fstIndex, lstIndex int = 0, work.fstIndex, work.lstIndex
	pivot := data[fstIndex]
	fstIndex++
	// iterate through comparing with pivot
	for fstIndex <= lstIndex {
		value = data[fstIndex]
		// look for smaller or equal to the pivot
		if value <= pivot {
			// increment the index
			//fmt.Printf("fst %d value %d <= %d\n", fstIndex, value, pivot)
			fstIndex++
		} else {
			// move greater to the right side
			data.Swap(fstIndex, lstIndex)
			//fmt.Printf("lst %d value %d > %d\n", lstIndex, value, pivot)
			lstIndex--
		}
	}
	fstIndex = work.fstIndex
	// iterate through comparing with pivot
	for fstIndex <= lstIndex {
		value = data[fstIndex]
		// look for equal to the pivot
		if value == pivot {
			// move equal to the right side
			data.Swap(fstIndex, lstIndex)
			lstIndex--
			eqlCount++
		} else {
			// increment the less index
			fstIndex++
		}
	}
	//fmt.Printf("part %d, dub count %d\n", fstIndex, eqlCount)
	//count of repeated values
	work.eqlCount = eqlCount
	//index to the pivot
	return fstIndex
}

func (data Int16Slice) workLoop(works chan *Work, counter *sync.WaitGroup) {
	counter.Add(1)
	hasWork := true
	for hasWork {
		select {
		case work := <-works:
			//add gorotine
			if data.doSort(works, work) {
				go data.workLoop(works, counter)
			}
		default:
			hasWork = false
		}
	}
	//fmt.Printf("Gorotine done\n")
	counter.Done()
}

func (data Int16Slice) doSort(works chan<- *Work, work *Work) bool {
	var fstIndex, lstIndex, prtIndex, eqlCount int = work.fstIndex, work.lstIndex, data.Part(work), work.eqlCount
	//fmt.Printf("work (%d - %d) partitioned at %d, %d equals to pivot %d\n", work.fstIndex, work.lstIndex, prtIndex, work.eqlCount, data[prtIndex])
	leSides := (fstIndex < prtIndex-1)
	if leSides {
		//less to the pivot
		work = new(Work)
		work.fstIndex = fstIndex
		work.lstIndex = prtIndex - 1
		works <- work
	}
	fstIndex = prtIndex + eqlCount
	grSides := (fstIndex < lstIndex)
	if grSides {
		//greater to the pivot
		work = new(Work)
		work.fstIndex = fstIndex
		work.lstIndex = lstIndex
		works <- work
	}
	return leSides && grSides
}

func (data Int16Slice) Sort() {
	var counter sync.WaitGroup
	//prepare channels
	works := make(chan *Work, 8)
	defer close(works)
	work := new(Work)
	work.fstIndex = 0
	work.lstIndex = data.Len() - 1
	works <- work
	go data.workLoop(works, &counter)
	//wait 1 ms
	time.Sleep(1e6)
	counter.Wait()
}

func main() {
	rand.Seed(time.Now().UnixNano())
	const n = 5000000
	var data [n]int16
	for i := 0; i < n; i++ {
		data[i] = int16(rand.Intn(256))
	}
	fmt.Printf("\nBefore sorted\n")
	for i := 0; i < 50; i++ {
		fmt.Printf("%d ", data[i])
	}
	slice := Int16Slice(data[:])
	fst := time.Now()
	//~ 75 ms
	slice.Sort()
	//~ 620 ms for syncro sort.Sort(slice)
	lst := time.Now()
	dur := lst.Sub(fst)
	sorted := sort.IsSorted(slice)
	fmt.Printf("\nAfter sorted\n")
	for i := 0; i < 50; i++ {
		fmt.Printf("%d ", data[i])
	}
	fmt.Printf("%d random values %v sorted in %.3f sec\n", n, sorted, dur.Seconds())
}
