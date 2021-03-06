﻿
#include "pch.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <iterator>
#include <thread>
#include <queue>
#include <vector>

struct WorkNode
{
	WorkNode *next;
	int fstIndex, lstIndex, eqlCount;
	WorkNode() {
		next = NULL;	
		fstIndex = lstIndex = eqlCount = 0;
	}
};

class SafeDeque
{
private:
	WorkNode* root;
	std::atomic<WorkNode*> aheap{ NULL };
	std::atomic<WorkNode*> awork{ NULL };
public:
	SafeDeque() {
		//allocate memory on heap
		root = new WorkNode[101];
		aheap.store(&root[0]);
		//chain of heap nodes
		for (int i{ 0 }; i < 100; ++i) {
			root[i].next = &root[i + 1];
		}
	}
	
	~SafeDeque() {
		delete root;
	}

	void push(int fst, int lst) {
		//obtain node from heap
		WorkNode* heap = aheap.load();
		//exlude from heap
		while (!aheap.compare_exchange_weak(heap, heap->next));
		//obtain last work node
		WorkNode* work = awork.load();
		//add node to lifo stack
		while (!awork.compare_exchange_weak(work, heap));
		heap->next = work;
		heap->fstIndex = fst;
		heap->lstIndex = lst;
	}

	WorkNode* pop() {
		//obtain last node from stack
		WorkNode* work = awork.load();
		if (!work) return NULL;
		//trim last node from chain
		while (!awork.compare_exchange_weak(work, work->next) && work);
		return work;
	}
	
	void free(WorkNode* work) {
		//obtain last heap node
		WorkNode* heap = aheap.load();
		//return work node to heap
		while (!aheap.compare_exchange_weak(heap, work));
		//add on head of chain
		work->next = heap;
	}
};

class AtomicSorter
{
private:
	SafeDeque works {};
	std::vector<short> vec {};
	short* data;
	size_t len;
	std::mutex mux{};
	std::condition_variable main_cnd;
	std::atomic_int counter{ 0 };

	void Randomize(size_t n)
	{
		vec.reserve(n);
		//data = new short[n];
		srand((unsigned)time(NULL));
		// Generate random numbers in the half-closed interval
		for (size_t i = 0; i < n; ++i)
		{
			short u = rand() % 256;
			vec.push_back(u);
		}
		data = vec.data();
		len = n;
	};

	void SwapValues(int firstIndex, int secondIndex)
	{
		//if (firstIndex == secondIndex) return;
		short holder = data[firstIndex];
		data[firstIndex] = data[secondIndex];
		data[secondIndex] = holder;
	}

	int DoPartByPivot(WorkNode *work)
	{
		int eqlCount = 0, fstIndex = work->fstIndex, lstIndex = work->lstIndex;
		short value, pivot = data[fstIndex++];
		// iterate through comparing with pivot
		while (fstIndex <= lstIndex)
		{
			value = data[fstIndex];
			// look for smaller or equal to the pivot
			if (value <= pivot)
			{
				// increment the index
				++fstIndex;
			}
			else
			{
				// move greater to the right side
				SwapValues(fstIndex, lstIndex--);
			}
		}
		fstIndex = work->fstIndex;
		// iterate through comparing with pivot
		while (fstIndex <= lstIndex)
		{
			value = data[fstIndex];
			// look for equal to the pivot
			if (value == pivot)
			{
				// move equal to the right side
				SwapValues(fstIndex, lstIndex--);
				++eqlCount;
			}
			else
			{
				// increment the index
				++fstIndex;
			}
		}
		//count of repeated values
		work->eqlCount = eqlCount;
		//values greater or equal to the pivot
		return fstIndex;
	}

	void Worker()
	{
		counter++;
		bool done = false;
		while (!done)
		{
			WorkNode *work = works.pop();
			if (work) {
				//work obtained
				DoSort(work);
				//return to my heap
				works.free(work);
			}
			else
			{
				done = true;
			}
		}
		int count = counter--;
		if (count < 2) main_cnd.notify_one();
		//std::cout << std::this_thread::get_id() << " done " << counter << std::endl;
	}

	void DoSort(WorkNode *work)
	{
		int fstIndex = work->fstIndex;
		int lstIndex = work->lstIndex;
		int pivotIndex = DoPartByPivot(work);
		// recurse on the left block
		bool fst = fstIndex < pivotIndex - 1;
		if (fst)
		{
			works.push(fstIndex, pivotIndex - 1);
			//std::this_thread::sleep_for(std::chrono::microseconds(12));
		}
		int eqlCount = work->eqlCount;
		// recurse on the right block
		bool lst = pivotIndex + eqlCount < lstIndex;
		if (lst)
		{
			works.push(pivotIndex + eqlCount, lstIndex);
			//std::this_thread::sleep_for(std::chrono::microseconds(12));
		}
		//both sides added
		if (fst && lst && (counter.load(std::memory_order_acquire) < 8)) {
			std::thread task{ [this] { Worker(); } };
			task.detach();
		}
	}

public:

	AtomicSorter(size_t n = 64) 
	{
		Randomize(n);
	}

	void Sort()
	{
		//190 ms
		//std::sort(std::begin(vec), std::end(vec));
		//prepare tasks
		works.push(0, len - 1);
		std::thread t0{ [this] { Worker(); } };
		t0.detach();
		//80 - 88 ms
		//std::this_thread::sleep_for(std::chrono::microseconds(50000));		
		std::unique_lock<std::mutex> unilock(mux);
		main_cnd.wait(unilock, [this] {
			int count = counter.load();
			//awake when all workers done
			return count < 1;
		});		
		//std::cout << std::this_thread::get_id() << " main done " << std::endl;
	}

	bool CheckSorted()
	{
		short curr = data[0];
		bool done = true;
		for (size_t i = 1; i < len; i++)
		{
			if (data[i] < curr)
			{
				std::cout << "Break on i=" << i <<", val=" << data[i] << ", curr=" << curr << std::endl;
				done = false;
				break;
			}
			else
				if (curr < data[i]) curr = data[i];
		}
		std::cout << "Curr is " << curr << ", sorted is " << std::boolalpha << done << ", length is " << len << std::endl;
		return done;
	}

	void PrintHead(size_t k = 32)
	{
		if (k > len) k = len;
		//print head
		std::cout << "Head: ";
		//for (auto& item : vec)
		for (size_t i = 0; i < k; i++)
		{
			std::cout << data[i] << " ";
		}
		std::cout << std::endl;
	};

	void PrintTail(size_t k = 32)
	{
		std::cout << "Tail: ";
		if (k > len) k = len;
		auto fst = std::cend(vec);
		auto lst = std::cend(vec);
		fst -= k;
		std::for_each(fst, lst, [](auto& value) { std::cout << value << " "; });
		std::cout << std::endl;
	};

	void InsertToSorted(short value) 
	{
		auto fst = std::cbegin(vec);
		auto lst = std::cend(vec);
		auto insPos = std::lower_bound(fst, lst, value);
		vec.insert(insPos, value);
	};
};

using seconds = std::chrono::duration<double>;

int main()
{
	std::cout << "There are " << std::thread::hardware_concurrency() << " cores." << std::endl;
	std::cout << "Enter number to generate integers: ";
	size_t n {};
	std::cin >> n;
	AtomicSorter shortSorter{n};
	const auto tic(std::chrono::steady_clock::now());
	//sort it 80 - 88 ms
	shortSorter.Sort();
	const auto toc(std::chrono::steady_clock::now());
	seconds diff{ toc - tic };
	std::cout << "Elapsed time is " << 1000 * diff.count() << " ms" << std::endl;
	//check result
	shortSorter.CheckSorted();
	shortSorter.PrintHead();
	//shortSorter.InsertToSorted(254);
	shortSorter.PrintTail();
	char c;
	std::cout << "Ok..Relax. Enter any char to exit." << std::endl;
	std::cin >> c;
}
//not used here
class spinlock_mutex
{
	std::atomic_flag flag{ ATOMIC_FLAG_INIT };
public:
	spinlock_mutex() {}

	void lock()
	{
		while (flag.test_and_set(std::memory_order_acquire));
	}
	void unlock()
	{
		flag.clear(std::memory_order_release);
	}
};
