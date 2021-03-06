
#include "pch.h"
#include <algorithm>
#include <iostream>
#include <time.h>
#include <thread>
#include <queue>
#include <vector>
#include <functional>
#include <future>
#include <Windows.h>
#include <Winbase.h>

class NanoSec
{
private:
	INT64 StartingTime, EndingTime, ElapsedMicroseconds, Frequency;
public:
	NanoSec() = default;
	void start() 
	{
		QueryPerformanceFrequency((LARGE_INTEGER *)&Frequency);
		QueryPerformanceCounter((LARGE_INTEGER *)&StartingTime);
	};
	void stop() 
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&EndingTime); 
		ElapsedMicroseconds = EndingTime - StartingTime;
		ElapsedMicroseconds *= 1000000;
		ElapsedMicroseconds /= Frequency;
	};
	int freq() { return int(Frequency); };
	int mics() { return int(ElapsedMicroseconds); };
};

class ShortState
{
private:
	//static inline size_t count{};
public:
	int fstIndex, lstIndex, eqlCount;

	ShortState(int fst, int lst)
	{
		fstIndex = fst;
		lstIndex = lst;
		eqlCount = 0;
		//++count;
	}

	//static size_t getInstancesCount() { return count; }

	~ShortState() 
	{
		//--count;
		//std::cout << "work fst=" << this->fstIndex << " lst=" << this->lstIndex << " destructed" << std::endl;
	}
};

//using vect_s = std::vector<short>;

class ShortSorter
{
private:
	std::queue<ShortState*> works {};
	std::vector<short> vec {};
	//std::unique_ptr<vect_s> vptr;
	short* data;
	size_t len;

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

	int DoPartByPivot(ShortState *work)
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
				fstIndex++;
			}
			else
			{
				// move greater to the right side
				SwapValues(fstIndex, lstIndex);
				lstIndex--;
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
				SwapValues(fstIndex, lstIndex);
				lstIndex--;
				eqlCount++;
			}
			else
			{
				// increment the index
				fstIndex++;
			}
		}
		//count of repeated values
		work->eqlCount = eqlCount;
		//values greater or equal to the pivot
		return fstIndex;
	}

	int DoPartByPivotLazy(ShortState *work)
	{
		int fstIndex = work->fstIndex, lstIndex = work->lstIndex;
		short value, pivot = data[fstIndex++];
		int eqlCount = 1, curIndex = fstIndex;
		// iterate through comparing with pivot
		while (curIndex <= lstIndex)
		{
			value = data[curIndex];
			// look for smaller or equal to the pivot
			if (value <= pivot)
			{
				if (value == pivot)
				{
					// move equal to the left side
					SwapValues(curIndex, fstIndex);
					fstIndex++;
					eqlCount++;
				}
				// increment the index
				curIndex++;
			}
			else
			{
				// move greater to the right side
				SwapValues(curIndex, lstIndex--);
			}
		}
		curIndex = work->fstIndex;
		while (curIndex < fstIndex)
		{
			value = data[curIndex];
			// move equal to the right side
			SwapValues(curIndex++, lstIndex--);
		}
		//count of repeated values
		work->eqlCount = eqlCount;
		//value equal to the pivot
		return lstIndex + 1;
	}

	void DoLoopSort(ShortState *work)
	{
		//work begin
		int fstIndex = work->fstIndex;
		int lstIndex = work->lstIndex;
		int pivotIndex = DoPartByPivot(work);
		// recurse on the left block
		if (fstIndex < pivotIndex - 1)
		{
			works.push(new ShortState(fstIndex, pivotIndex - 1));
			//works.Enqueue(new ShortState(fstIndex, pivotIndex - 1));
			//do in current loop
		}
		int eqlCount = work->eqlCount;
		// recurse on the right block
		if (pivotIndex + eqlCount < lstIndex)
		{
			works.push(new ShortState(pivotIndex + eqlCount, lstIndex));
			//works.Enqueue(new ShortState(pivotIndex + eqlCount, lstIndex));
			//TryAddLoop();
		}
		//work done
	}

	void DoLoopWork()
	{
		while (!works.empty())
		{
			ShortState *work = works.front();
			works.pop();
			DoLoopSort(work);
			delete work;
		}
	}

public:
	
	ShortSorter(size_t n = 64) 
	{
		Randomize(n);
		//vptr = std::make_unique<vect_s>();
		//data = v->data;
	}

	~ShortSorter()
	{
		//if (data) delete data;
	}

	void LoopSort()
	{
		//auto fst = std::begin(vec);
		//auto lst = std::end(vec);
		//std::sort(fst, lst);
		works.push(new ShortState(0, len - 1));
		DoLoopWork();
	}

	bool CheckSorted()
	{
		short curr = data[0];
		bool done = true;
		for (size_t i = 1; i < len; i++)
		{
			if (data[i] < curr)
			{
				std::cout << "Break on i=" << i <<", val=" << data[i] << "}, curr={curr}" << std::endl;
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

using SorterPtr = std::shared_ptr<ShortSorter>;

class LoopTask
{
private:
	SorterPtr sorter;
	ShortSorter *sorterer;
public:
	LoopTask() = default;
	LoopTask(SorterPtr& p) { sorter = p; };
	LoopTask(ShortSorter *p) { sorterer = p; };

	int operator()() const
	{
		//sorter->LoopSort();
		sorterer->LoopSort();
		return 42;
	};
};

//background_task f;
//std::thread my_thread(f);
//std::thread my_thread(background_task());

int main()
{
	//INT64 StartingTime, EndingTime, ElapsedMicroseconds, Frequency;
	std::cout << "Enter number to generate integers: ";
	size_t n {};
	std::cin >> n;
	//array
	//std::unique_ptr<short[]> arr = std::make_unique<short[]>(n);
	//short *data = arr.get();
	ShortSorter shortSorter{n};
	SorterPtr ptr = std::make_shared<ShortSorter>(shortSorter);
	//ByteRandDemo(n);
	NanoSec ns{};
	ns.start();
	//sort it
	//shortSorter.LoopSort();
	std::future<int> futInt = std::async(std::launch::async, LoopTask{ &shortSorter });
	//std::sort(vec.begin(), vec.end());
	//count perfomance
	int ret = futInt.get();
	ns.stop();
	std::cout << "Result is " << ret << std::endl;
	std::cout << "Frequency: " << ns.freq() << " elapsed time in microsec " << ns.mics() << std::endl;
	shortSorter.CheckSorted();
	shortSorter.PrintHead();
	//shortSorter.InsertToSorted(254);
	shortSorter.PrintTail();
	//std::cin >> n;
	//ByteRandDemo(n);
	char c;
	std::cout << "Ok. Enter any char to exit." << std::endl;
	std::cin >> c;
}

