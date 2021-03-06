
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
//#include <functional>
//#include <execution>

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

	~ShortState() 
	{
		//--count;
		//std::cout << "work fst=" << this->fstIndex << " lst=" << this->lstIndex << " destructed" << std::endl;
	}
};


class ShortSorter
{
private:
	std::queue<ShortState*> works {};
	std::vector<short> vec {};
	short* data;
	size_t len;
	std::mutex mux;
	std::condition_variable worker_cnd, main_cnd;
	bool exit{ false };
	int counter{ 0 };

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

	void SleepWorker()
	{
		//std::cout << std::this_thread::get_id() << " started " << std::endl;
		std::unique_lock<std::mutex> uni(mux, std::defer_lock);
		while (true)
		{
			ShortState *work = NULL;
			//entered workers
			++counter;
			bool loop{ true };
			while (loop)
			{
				uni.lock();
				if (!works.empty())
				{
					work = works.front();
					works.pop();
					loop = false;
				}
				uni.unlock();
				if (loop)
				{
					//all workers here
					if (counter < 8) 
					{
						std::this_thread::sleep_for(std::chrono::microseconds(800));
						//std::this_thread::yield();
					}
					else
					{
						exit = true;
					}
					if (exit) break;
				}
			}
			if (exit) break;
			//released workers
			--counter;
			//work obtained
			DoSort(work);
			delete work;
		}
		//std::cout << std::this_thread::get_id() << " done " << std::endl;
	}

	void Worker()
	{
		//std::cout << std::this_thread::get_id() << " started " << std::endl;
		std::unique_lock<std::mutex> uni(mux);
		while (true)
		{
			//entered workers
			if (!works.empty())
			{
				ShortState *work = works.front();
				works.pop();
				uni.unlock();
				//work obtained
				DoSort(work);
				delete work;
				uni.lock();
			}
			else
			{
				++counter;
				if (counter > 7)
				{
					exit = true;
					worker_cnd.notify_all();
					main_cnd.notify_one();
				}
				else
				{
					worker_cnd.wait(uni, [this] { return UnLock();	});
					--counter;
				}
				if (exit) break;
			}
		}
		//std::cout << std::this_thread::get_id() << " done " << std::endl;
	}

	void DoSort(ShortState *work)
	{
		int fstIndex = work->fstIndex;
		int lstIndex = work->lstIndex;
		int pivotIndex = DoPartByPivot(work);
		// recurse on the left block
		std::lock_guard<std::mutex> lock(mux);
		bool fst = fstIndex < pivotIndex - 1;
		if (fst)
		{
			works.push(new ShortState(fstIndex, pivotIndex - 1));
		}
		int eqlCount = work->eqlCount;
		// recurse on the right block
		bool lst = pivotIndex + eqlCount < lstIndex;
		if (lst)
		{
			works.push(new ShortState(pivotIndex + eqlCount, lstIndex));
		}
		//both sides added
		if (fst && lst)
		{
			worker_cnd.notify_one();
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

	void Sort()
	{
		//190 ms
		//std::sort(std::begin(vec), std::end(vec));
		//prepare tasks
		works.push(new ShortState(0, len - 1));		
		std::thread t0{ [this] { Worker(); } };
		t0.detach();
		std::thread t1{ [this] { Worker(); } };
		t1.detach();
		std::thread t2{ [this] { Worker(); } };
		t2.detach();
		std::thread t3{ [this] { Worker(); } };
		t3.detach();
		std::thread t4{ [this] { Worker(); } };
		t4.detach();
		std::thread t5{ [this] { Worker(); } };
		t5.detach();
		std::thread t6{ [this] { Worker(); } };
		t6.detach();
		std::thread t7{ [this] { Worker(); } };
		t7.detach();
		//80 - 88 ms
		std::unique_lock<std::mutex> uni(mux);
		main_cnd.wait(uni, [this] {return exit; });
		//std::cout << std::this_thread::get_id() << " main done " << std::endl;
	}

	bool UnLock()
	{
		return (exit || !works.empty());
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

using seconds = std::chrono::duration<double>;

int main()
{
	std::cout << "There are " << std::thread::hardware_concurrency() << " cores." << std::endl;
	std::cout << "Enter number to generate integers: ";
	size_t n {};
	std::cin >> n;
	ShortSorter shortSorter{n};
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
	std::cout << "Ok. Enter any char to exit." << std::endl;
	std::cin >> c;
}

